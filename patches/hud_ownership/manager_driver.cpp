#include "manager_driver.hpp"
namespace rev_hud {
namespace {
constexpr ManagerSite Sites[] = {
    {0x02B497CA, ManagerKind::Cockpit, 8},
    {0x02B49C5B, ManagerKind::Cockpit, 9},
    {0x02B49DE3, ManagerKind::Cockpit, 11},
    {0x02B6847B, ManagerKind::MiniMap, 8},
    {0x02B685B3, ManagerKind::MiniMap, 9},
    {0x02B68724, ManagerKind::MiniMap, 11},
};
ManagerDriver* Sink = nullptr;
struct Busy {
    bool& value;
    explicit Busy(bool& v) : value(v) { value = true; }
    ~Busy() { value = false; }
};
}
const ManagerSite* manager_site(u32 address) {
    for (const auto& s : Sites) if (s.address == address) return &s;
    return nullptr;
}
bool ManagerDriver::sample(ManagerFrame& out) {
    ManagerFrame next{};
    if (!host_.sample || !host_.sample(host_.memory.context, &next) || !next.frame) return false;
    for (u32 i = 0; i < 2; ++i)
        if (!next.parents[i] || !next.parent_lifetimes[i] ||
            next.parents[i] != life_.parent(static_cast<ManagerKind>(i))) return false;
    out = next; return true;
}
bool ManagerDriver::attach(u32 ticket) {
    if (busy_ || attached_ || !life_.live_ticket(ticket) || !host_.memory.word ||
        !host_.thread_id || !host_.sample || !host_.enter_scope || !host_.leave_scope) return false;
    Busy guard(busy_);
    const u32 thread = host_.thread_id(host_.memory.context);
    ManagerFrame frame{};
    if (!thread || !sample(frame) || !life_.live_ticket(ticket)) return false;
    life_.observe(frame.session);
    if (!life_.live_ticket(ticket)) return false;
    for (u32 i = 0; i < 2; ++i) {
        parents_[i] = frame.parents[i]; lifetimes_[i] = frame.parent_lifetimes[i];
    }
    ticket_ = ticket; thread_ = thread; last_frame_ = frame.frame;
    attached_ = true; return true;
}
void ManagerDriver::stop() {
    // A call from a phase may revoke, but does not free or reset the busy guard.
    attached_ = admitted_ = false;
    life_.stop();
}
bool ManagerDriver::phase_scope(WidgetKind kind, u32 unit, u32 phase, u32 context) const {
    const KindInfo* info = kind_info(kind);
    return attached_ && admitted_ && busy_ && current_ && info &&
        host_.thread_id && host_.thread_id(host_.memory.context) == thread_ &&
        life_.live_ticket(ticket_) && info->manager == current_->manager &&
        unit && unit == life_.unit(static_cast<u32>(kind)) &&
        phase == current_->phase && context == context_;
}
bool ManagerDriver::event(u32 address, u32 manager, u32 context) {
    const ManagerSite* site = manager_site(address);
    if (busy_ || !attached_ || !site || !life_.live_ticket(ticket_) ||
        !host_.thread_id || host_.thread_id(host_.memory.context) != thread_) return false;
    const u32 owner = static_cast<u32>(site->manager);
    if (!manager || manager != parents_[owner] ||
        (site->phase != 11 && context)) return false;
    Busy guard(busy_);
    ManagerFrame frame{};
    if (!sample(frame)) { stop(); return false; }
    if (!attached_ || !life_.live_ticket(ticket_)) return false; // sample can reenter
    if (frame.frame < last_frame_) return false; // stale frames cannot poison session
    for (u32 i = 0; i < 2; ++i) {
        if (frame.parents[i] != parents_[i] || frame.parent_lifetimes[i] != lifetimes_[i]) {
            stop(); return false;
        }
    }
    u32 vt = 0, view = 0;
    if (!host_.memory.word(host_.memory.context, manager, &vt) || vt != manager_vtable(site->manager)) {
        stop(); return false;
    }
    if (site->phase == 11) {
        if (!context || context > Invalid - 0x15B ||
            !host_.memory.word(host_.memory.context, context + 0x158, &view)) return false;
        if ((view & 255) != 1) return false; // P1, mirrors and nonplayer views unchanged
    }
    life_.observe(frame.session);
    if (!life_.live_ticket(ticket_)) { attached_ = false; return false; }
    last_frame_ = frame.frame;
    if (!host_.enter_scope(host_.memory.context, *site, frame, context)) {
        if (host_.scope_failed && host_.scope_failed(host_.memory.context)) stop();
        return false;
    }
    // A successful scope must always be paired, including reentrant stop.
    current_ = site; context_ = context; admitted_ = attached_ && life_.live_ticket(ticket_);
    const bool result = admitted_ && life_.dispatch_manager(ticket_, frame.session, frame.frame,
                            site->manager, manager, site->phase, site->phase == 11 ? 1 : 0, context);
    admitted_ = false; current_ = nullptr; context_ = 0;
    const bool restored = host_.leave_scope(host_.memory.context);
    if (!restored) stop();
    return result && restored && attached_ && life_.live_ticket(ticket_);
}
bool bind_manager_sink(ManagerDriver& driver) {
    if (Sink) return false;
    Sink = &driver; return true;
}
}
extern "C" unsigned int rev_hud_manager_event(unsigned int site,
        unsigned int manager, unsigned int context) {
    return rev_hud::Sink && rev_hud::Sink->event(site, manager, context) ? 1u : 0u;
}
