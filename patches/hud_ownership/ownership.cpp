#include "ownership.hpp"
namespace rev_hud {
namespace {
constexpr KindInfo Kinds[] = {
    {0x04DE52C4, ManagerKind::Cockpit, 9},
    {0x04DE4C3C, ManagerKind::Cockpit, 8},
    {0x04DE810C, ManagerKind::MiniMap, 8},
};
bool usable(const Actor& a) {
    return a.address && a.lifetime && a.serial >= 0 && a.think_mode == 1;
}
bool same_actor(const Actor& a, const Actor& b) {
    return a.address == b.address && a.lifetime == b.lifetime &&
           a.serial == b.serial && a.think_mode == b.think_mode;
}
bool dual_session(const Session& s) {
    return s.active && s.epoch && usable(s.self) && usable(s.sub0) &&
           s.self.address != s.sub0.address && s.self.serial != s.sub0.serial;
}
// Static storage, no constructor-driven engine calls and no allocations.
Registry state;
}
const KindInfo* kind_info(WidgetKind k) {
    const u32 i = static_cast<u32>(k);
    return i < 3 ? &Kinds[i] : nullptr;
}
u32 manager_vtable(ManagerKind k) {
    switch (k) {
        case ManagerKind::Cockpit: return 0x04DE5C14;
        case ManagerKind::MiniMap: return 0x04DE86FC;
    }
    return 0;
}
u32 draw_view(u32 flags) { return (flags >> 16) & 0x3FF; }
bool replace_draw_view(u32 flags, u32 mask, u32* out) {
    if (!out || mask > 0x3FF) return false;
    *out = (flags & ~ViewBits) | (mask << 16);
    return true;
}
bool view_allowed(u32 flags, u32 view) {
    return view < 10 && (draw_view(flags) & (u32(1) << view)) != 0;
}
Registry::ManagerEntry* Registry::find_manager(Token t) {
    if (t.slot >= ManagerCapacity) return nullptr;
    ManagerEntry& m = managers_[t.slot];
    return m.occupied && m.generation == t.generation && t.generation ? &m : nullptr;
}
const Registry::ManagerEntry* Registry::find_manager(Token t) const {
    if (t.slot >= ManagerCapacity) return nullptr;
    const ManagerEntry& m = managers_[t.slot];
    return m.occupied && m.generation == t.generation && t.generation ? &m : nullptr;
}
Token Registry::open_manager(u32 address, u32 vt, ManagerKind kind) {
    const u32 expected = manager_vtable(kind);
    if (!address || !expected || vt != expected) return {};
    for (const auto& m : managers_)
        if (m.occupied && m.address == address) return {};
    for (u32 i = 0; i < ManagerCapacity; ++i) {
        auto& m = managers_[i];
        // Saturated counters retire their slot instead of accepting old tokens.
        if (m.occupied || m.generation == Invalid) continue;
        ++m.generation;
        m.address = address; m.kind = kind; m.occupied = m.alive = true;
        return {i, m.generation};
    }
    return {};
}
bool Registry::invalidate_manager(Token t) {
    auto* m = find_manager(t);
    if (!m) return false;
    m->alive = false;
    return true;
}
bool Registry::release_manager(Token t) {
    auto* m = find_manager(t);
    if (!m || m->alive) return false;
    for (const auto& w : widgets_)
        if (w.occupied && w.manager.slot == t.slot &&
                w.manager.generation == t.generation) return false;
    m->occupied = false; m->address = 0;
    return true;
}
void Registry::set_session(const Session& next) {
    // Latch invalidation before accepting a replacement snapshot. Retaining a
    // tombstone prevents a surviving P2 widget from falling back to Self.
    for (auto& w : widgets_) {
        if (!w.occupied || w.revoked) continue;
        if (!dual_session(next) || w.epoch != next.epoch ||
                !same_actor(w.actor, w.member ? next.sub0 : next.self))
            w.revoked = true;
    }
    session_ = next;
}
Token Registry::bind_widget(Token mt, u32 address, u32 vt, WidgetKind kind,
                            u32 member, const Actor& actor, u32 view) {
    const auto* info = kind_info(kind);
    auto* m = find_manager(mt);
    if (!m || !m->alive || !info || info->manager != m->kind ||
            !address || vt != info->vtable || member > 1 || view != member ||
            !dual_session(session_)) return {};
    const Actor& expected = member ? session_.sub0 : session_.self;
    if (!same_actor(actor, expected)) return {};
    for (const auto& w : widgets_)
        if (w.occupied && w.address == address) return {};
    for (u32 i = 0; i < WidgetCapacity; ++i) {
        auto& w = widgets_[i];
        if (w.occupied || w.generation == Invalid) continue;
        ++w.generation;
        w.address = address; w.observed_vtable = vt; w.kind = kind;
        w.manager = mt; w.member = member; w.view = view;
        w.epoch = session_.epoch; w.actor = actor; w.occupied = true; w.revoked = false;
        return {i, w.generation};
    }
    return {};
}
bool Registry::retire_widget(Token t) {
    if (t.slot >= WidgetCapacity || !t.generation) return false;
    auto& w = widgets_[t.slot];
    if (!w.occupied || w.generation != t.generation) return false;
    w.occupied = false; w.address = 0;
    return true;
}
Route Registry::resolve(u32 address, WidgetKind kind) const {
    if (!address || !kind_info(kind)) return {};
    for (const auto& w : widgets_) {
        if (!w.occupied || w.address != address) continue;
        // A tracked pointer presented through another class-specific hook is
        // never interpreted using the previous owner's layout.
        if (w.kind != kind) return {Mode::Hidden, 0, 0, 0};
        const auto* m = find_manager(w.manager);
        // Never fall back to a member call on an invalidated manager's widget.
        if (!m || !m->alive) return {Mode::Hidden, 0, 0, 0};
        const bool current = !w.revoked && dual_session(session_) &&
            session_.epoch == w.epoch &&
            same_actor(w.actor, w.member ? session_.sub0 : session_.self);
        if (!current) return {w.member ? Mode::Hidden : Mode::Stock, 0, 0, 0};
        return {Mode::Local, w.actor.address, w.member, w.view};
    }
    return {}; // original, unregistered GUI retains its Self finder
}
u32 Registry::flags_for(u32 address, WidgetKind kind, u32 original) const {
    const Route r = resolve(address, kind);
    if (r.mode == Mode::Stock) return original;
    u32 result = original;
    replace_draw_view(original, r.mode == Mode::Hidden ? 0 : (u32(1) << r.view), &result);
    return result;
}
u32 Registry::widget_count() const {
    u32 n = 0; for (const auto& w : widgets_) n += w.occupied ? 1 : 0; return n;
}
Registry& registry() { return state; }
}
extern "C" unsigned int rev_hud_resolve_actor(unsigned int widget,
        unsigned int kind, unsigned int* actor_out) {
    if (!actor_out) return 0;
    const auto r = rev_hud::registry().resolve(widget, static_cast<rev_hud::WidgetKind>(kind));
    *actor_out = r.mode == rev_hud::Mode::Local ? r.actor : 0;
    return static_cast<unsigned int>(r.mode);
}
