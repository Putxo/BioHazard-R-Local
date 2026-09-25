#include "lifecycle.hpp"
namespace rev_hud {
namespace {
WidgetKind kind(u32 i) { return static_cast<WidgetKind>(i); }
bool field(Reader r, u32 p, u32 off, u32& out) {
    return p && p <= Invalid - off - 3 && r.word && r.word(r.context, p + off, &out);
}
bool graph_valid(const GuiTree& g) {
    if (!g.unit || !g.resource || !g.root || !g.table || !g.count || g.count > GuiTree::MaxNodes) return false;
    if (g.unit == g.root || g.unit == g.table || g.root == g.table ||
        g.resource == g.unit || g.resource == g.root || g.resource == g.table) return false;
    for (u32 i = 0; i < g.count; ++i) {
        if (!g.nodes[i] || g.nodes[i] == g.resource || g.nodes[i] == g.unit || g.nodes[i] == g.root || g.nodes[i] == g.table) return false;
        for (u32 j = 0; j < i; ++j) if (g.nodes[i] == g.nodes[j]) return false;
    }
    return true;
}
u32 mutable_pointer(const GuiTree& g, u32 i) {
    return i == 0 ? g.unit : i == 1 ? g.root : i == 2 ? g.table : g.nodes[i - 3];
}
}
bool capture_tree(Reader r, u32 unit, WidgetKind k, GuiTree* out) {
    const KindInfo* info = kind_info(k);
    if (!out || !info) return false;
    GuiTree g{}; g.unit = unit;
    u32 vt = 0, header = 0, owner = 0, again = 0;
    if (!field(r, unit, 0, vt) || vt != info->vtable ||
        !field(r, unit, 0xF0, g.resource) || !field(r, unit, 0xF4, g.root) ||
        !field(r, unit, 0xF8, g.table) || !field(r, g.resource, 0x68, header) ||
        !field(r, header, 0x44, g.count) || !g.count || g.count > GuiTree::MaxNodes ||
        !field(r, g.root, 0x6C, owner) || owner != unit) return false;
    for (u32 i = 0; i < g.count; ++i) {
        if (!field(r, g.table, i * 4, g.nodes[i]) ||
            !field(r, g.nodes[i], 0x6C, owner) || owner != unit) return false;
    }
    if (!graph_valid(g)) return false;
    // Detect ordinary mid-capture replacement. This is NOT a lifetime lock or
    // an ABA detector; native lifecycle serialization remains mandatory.
    const u32 offsets[] = {0, 0xF0, 0xF4, 0xF8};
    const u32 expected[] = {vt, g.resource, g.root, g.table};
    for (u32 i = 0; i < 4; ++i)
        if (!field(r, unit, offsets[i], again) || again != expected[i]) return false;
    if (!field(r, g.resource, 0x68, again) || again != header ||
        !field(r, header, 0x44, again) || again != g.count) return false;
    *out = g;
    return true;
}
bool disjoint_trees(const GuiTree& a, const GuiTree& b) {
    if (!graph_valid(a) || !graph_valid(b)) return false;
    for (u32 i = 0; i < a.count + 3; ++i)
        for (u32 j = 0; j < b.count + 3; ++j)
            if (mutable_pointer(a, i) == mutable_pointer(b, j)) return false;
    return true; // resource template is intentionally NOT compared
}
bool Lifecycle::valid_backend() const {
    return backend_.memory.word && backend_.safe_point && backend_.construct &&
        (backend_.initialize || backend_.checked_initialize) &&
        (backend_.destroy || backend_.checked_destroy) &&
        (backend_.phase || backend_.checked_phase) && backend_.write_word;
}
bool Lifecycle::live_routes() const {
    for (u32 i = 0; i < 3; ++i) {
        const Route r = registry_.resolve(units_[i], kind(i));
        if (!units_[i] || r.mode != Mode::Local || r.member != 1 || r.view != 1) return false;
    }
    return true;
}
void Lifecycle::stop() {
    if (state_ == LifeState::Empty) return;
    for (const auto& t : managers_) if (t.valid()) registry_.invalidate_manager(t);
    if (state_ != LifeState::Quarantined) state_ = LifeState::Draining;
}
void Lifecycle::fail(LifeError e) { error_ = e; stop(); }
void Lifecycle::observe(const Session& s) {
    registry_.set_session(s);
    if ((state_ == LifeState::Prepared || state_ == LifeState::Live) && !live_routes()) fail(LifeError::Stale);
}
void Lifecycle::parent_destroying(u32 a) {
    if (a && (parents_[0] == a || parents_[1] == a)) stop();
}
bool Lifecycle::verify_graphs() {
    GuiTree fresh[3]{};
    for (u32 i = 0; i < 3; ++i) {
        if (!capture_tree(backend_.memory, units_[i], kind(i), &fresh[i])) {
            fail(LifeError::Resources); return false;
        }
        for (u32 j = 0; j < 3; ++j) {
            GuiTree stock{};
            if (!capture_tree(backend_.memory, originals_[j], kind(j), &stock)) {
                fail(LifeError::Resources); return false;
            }
            if (!disjoint_trees(fresh[i], stock)) {
                quarantine_[i] = true; fail(LifeError::Alias); return false;
            }
        }
        for (u32 j = 0; j < i; ++j) if (!disjoint_trees(fresh[i], fresh[j])) {
            quarantine_[i] = quarantine_[j] = true; fail(LifeError::Alias); return false;
        }
    }
    for (u32 i = 0; i < 3; ++i) trees_[i] = fresh[i];
    return true;
}
u32 Lifecycle::prepare(const Session& s, const u32 p[2], const u32 originals[3]) {
    if (busy_ || state_ != LifeState::Empty) { error_ = LifeError::Busy; return 0; }
    if (!valid_backend() || !p || !originals || !s.active || !s.epoch || !s.self.address || !s.sub0.address ||
        !s.self.lifetime || !s.sub0.lifetime || s.self.serial < 0 || s.sub0.serial < 0 ||
        s.self.think_mode != 1 || s.sub0.think_mode != 1 ||
        s.self.address == s.sub0.address || s.self.serial == s.sub0.serial || ticket_ == Invalid) {
        error_ = LifeError::BadInput; return 0;
    }
    if (!backend_.safe_point(backend_.memory.context)) { error_ = LifeError::UnsafePoint; return 0; }
    registry_.set_session(s);
    for (auto& tree : trees_) tree = {};
    for (auto& flag : quarantine_) flag = false;
    // Validate borrowed originals before allocating anything.
    for (u32 i = 0; i < 3; ++i) {
        GuiTree g{};
        if (!capture_tree(backend_.memory, originals[i], kind(i), &g)) { error_ = LifeError::Resources; return 0; }
        originals_[i] = originals[i]; stocks_[i] = g;
    }
    state_ = LifeState::Preparing; error_ = LifeError::None; busy_ = true;
    for (u32 i = 0; i < 2; ++i) {
        parents_[i] = p[i]; u32 vt = 0;
        if (!field(backend_.memory, p[i], 0, vt)) { fail(LifeError::BadInput); break; }
        managers_[i] = registry_.open_manager(p[i], vt, static_cast<ManagerKind>(i));
        if (!managers_[i].valid()) { fail(LifeError::Registration); break; }
    }
    for (u32 i = 0; i < 3 && state_ == LifeState::Preparing; ++i) {
        const u32 unit = backend_.construct(backend_.memory.context, kind(i));
        if (!unit) { fail(LifeError::Construct); break; }
        bool alias = false;
        for (u32 j = 0; j < 3; ++j) if (unit == originals_[j] || unit == units_[j]) alias = true;
        for (u32 j = 0; j < 2; ++j) if (unit == parents_[j]) alias = true;
        // Backend contract violation: never initialize or destroy the borrowed
        // address. Its alleged second allocation is not a second owned unit.
        if (alias) { fail(LifeError::Alias); break; }
        units_[i] = unit;
        if (state_ != LifeState::Preparing) break; // stop requested by constructor
        u32 observed_vtable = 0;
        if (!field(backend_.memory, unit, 0, observed_vtable) ||
            observed_vtable != kind_info(kind(i))->vtable) {
            quarantine_[i] = true; fail(LifeError::BadInput); break;
        }
        bool initialized = true;
        if (backend_.checked_initialize)
            initialized = backend_.checked_initialize(backend_.memory.context, unit, kind(i));
        else backend_.initialize(backend_.memory.context, unit, kind(i));
        GuiTree g{};
        const bool captured = capture_tree(backend_.memory, unit, kind(i), &g);
        if (captured) trees_[i] = g;
        if (state_ != LifeState::Preparing) break;
        if (!initialized || !captured) { fail(LifeError::Resources); break; }
        widgets_[i] = registry_.bind_widget(managers_[i == 2 ? 1 : 0], unit,
                         kind_info(kind(i))->vtable, kind(i), 1, s.sub0, 1);
        if (!widgets_[i].valid()) { fail(LifeError::Registration); break; }
    }
    if (state_ == LifeState::Preparing && verify_graphs() && live_routes()) {
        state_ = LifeState::Prepared; ++ticket_; frame_ = phases_ = 0;
    } else if (state_ == LifeState::Preparing) fail(LifeError::Stale);
    busy_ = false;
    if (state_ != LifeState::Prepared) { collect(); return 0; }
    return ticket_;
}
bool Lifecycle::publish(u32 ticket, const Session& s) {
    if (busy_ || state_ != LifeState::Prepared || !ticket || ticket != ticket_) return false;
    if (!backend_.safe_point(backend_.memory.context)) { error_ = LifeError::UnsafePoint; return false; }
    busy_ = true; observe(s);
    if (state_ == LifeState::Prepared && verify_graphs() && live_routes()) state_ = LifeState::Live;
    busy_ = false;
    return state_ == LifeState::Live;
}
bool Lifecycle::can_destroy(u32 i) const {
    u32 observed_vtable = 0;
    if (!field(backend_.memory, units_[i], 0, observed_vtable) ||
        observed_vtable != kind_info(kind(i))->vtable) return false;
    if (!trees_[i].unit) {
        // Fully constructed but resource initialization returned early. Never
        // infer ownership of non-null structures from an unsuccessful capture.
        u32 resource = 0, root = 0, table = 0;
        return field(backend_.memory, units_[i], 0xF0, resource) &&
            field(backend_.memory, units_[i], 0xF4, root) &&
            field(backend_.memory, units_[i], 0xF8, table) && !resource && !root && !table;
    }
    GuiTree now{};
    if (!capture_tree(backend_.memory, units_[i], kind(i), &now)) return false;
    const GuiTree& old = trees_[i];
    if (now.unit != old.unit || now.resource != old.resource || now.root != old.root ||
        now.table != old.table || now.count != old.count) return false;
    for (u32 n = 0; n < now.count; ++n) if (now.nodes[n] != old.nodes[n]) return false;
    for (const auto& stock : stocks_) if (!disjoint_trees(now, stock)) return false;
    return true;
}
bool Lifecycle::collect() {
    if (busy_ || (state_ != LifeState::Draining && state_ != LifeState::Quarantined)) return false;
    if (!backend_.safe_point(backend_.memory.context)) { error_ = LifeError::UnsafePoint; return false; }
    busy_ = true; bool quarantine = false;
    // All registry entries remain tombstones during ALL native destructors.
    for (u32 i = 3; i-- > 0;) {
        if (units_[i] && !can_destroy(i)) quarantine_[i] = true;
        if (quarantine_[i]) { quarantine = true; continue; }
        if (units_[i]) {
            bool destroyed = true;
            if (backend_.checked_destroy)
                destroyed = backend_.checked_destroy(backend_.memory.context, units_[i], kind(i));
            else backend_.destroy(backend_.memory.context, units_[i], kind(i));
            if (!destroyed) { quarantine_[i] = quarantine = true; continue; }
            units_[i] = 0;
        }
    }
    if (quarantine) { state_ = LifeState::Quarantined; busy_ = false; return false; }
    for (auto& t : widgets_) { if (t.valid()) registry_.retire_widget(t); t = {}; }
    for (auto& t : managers_) { if (t.valid()) registry_.release_manager(t); t = {}; }
    for (auto& a : parents_) a = 0;
    state_ = LifeState::Empty; busy_ = false;
    return true;
}
bool Lifecycle::dispatch(u32 ticket, const Session& current, u32 frame, u32 phase, u32 view, u32 context) {
    // Compatibility entry for historical component tests, NOT a native hook.
    return dispatch_scope(ticket, current, frame, phase, view, context, 3);
}
bool Lifecycle::dispatch_manager(u32 ticket, const Session& current, u32 frame,
                                 ManagerKind manager, u32 parent_address,
                                 u32 phase, u32 view, u32 context) {
    const u32 index = static_cast<u32>(manager);
    if (busy_ || !live_ticket(ticket) || index >= 2 || !parent_address ||
        parents_[index] != parent_address || !frame || frame < frame_) return false;
    // Parent lifetime must already be pinned by the driver. A class read is a
    // consistency check, not a substitute for that pin or an ABA detector.
    u32 vt = 0;
    if (!field(backend_.memory, parent_address, 0, vt) || vt != manager_vtable(manager)) {
        fail(LifeError::Stale); return false;
    }
    return dispatch_scope(ticket, current, frame, phase, view, context, u32(1) << index);
}
bool Lifecycle::dispatch_scope(u32 ticket, const Session& current, u32 frame,
                               u32 phase, u32 view, u32 context, u32 manager_mask) {
    // A stale dispatch must not replace the current session or revoke a new
    // generation. Explicit observe() is reserved for the trusted lifecycle driver.
    if (busy_ || state_ != LifeState::Live || !ticket || ticket != ticket_ || !frame || frame < frame_) return false;
    observe(current);
    if (state_ != LifeState::Live) return false;
    if (phase != 8 && phase != 9 && phase != 11) { error_ = LifeError::UnsupportedPhase; return false; }
    if (phase == 11 && view != 1) return false;
    if (frame != frame_) { frame_ = frame; phases_ = 0; }
    const u32 phase_bit = phase == 8 ? 1 : phase == 9 ? 2 : 4;
    const u32 bit = ((manager_mask & 1) ? phase_bit : 0) |
                    ((manager_mask & 2) ? (phase_bit << 3) : 0);
    if (phases_ & bit) return false;
    if (!live_routes()) { fail(LifeError::Stale); return false; }
    busy_ = true; phases_ |= bit;
    for (u32 i = 0; i < 3 && state_ == LifeState::Live; ++i) {
        const u32 owner_bit = kind_info(kind(i))->manager == ManagerKind::Cockpit ? 1 : 2;
        if (!(manager_mask & owner_bit)) continue;
        u32 old = 0, scoped = 0, now = 0;
        if (!field(backend_.memory, units_[i], 0xC, old)) { fail(LifeError::Memory); break; }
        scoped = registry_.flags_for(units_[i], kind(i), old);
        if (!backend_.write_word(backend_.memory.context, units_[i] + 0xC, scoped)) { fail(LifeError::Memory); break; }
        bool dispatched = true;
        if (backend_.checked_phase)
            dispatched = backend_.checked_phase(backend_.memory.context, units_[i], kind(i), phase, context);
        else backend_.phase(backend_.memory.context, units_[i], kind(i), phase, context);
        // stop() cannot free while busy. Preserve unrelated bits changed by the
        // engine, restoring only the original ten-bit view mask.
        if (!field(backend_.memory, units_[i], 0xC, now) ||
            !replace_draw_view(now, draw_view(old), &now) ||
            !backend_.write_word(backend_.memory.context, units_[i] + 0xC, now)) { fail(LifeError::Memory); break; }
        if (!dispatched) { fail(LifeError::BadInput); break; }
    }
    busy_ = false;
    return state_ == LifeState::Live;
}
}
