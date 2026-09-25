#pragma once
#include "lifecycle.hpp"
namespace rev_hud {
struct ManagerSite {
    u32 address;
    ManagerKind manager;
    u32 phase;
};
const ManagerSite* manager_site(u32 address);
// Supplied by the engine lifetime/frame owner, not by a GUI callback's guess.
// Parent generations must change BEFORE destruction/reuse, even at same VA.
struct ManagerFrame {
    u32 frame = 0;
    Session session{};
    u32 parents[2]{}, parent_lifetimes[2]{};
};
struct ManagerHost {
    Reader memory{};
    u32 (*thread_id)(void*) noexcept = nullptr;
    // Called before each event. Must pin all supplied lifetimes through it.
    bool (*sample)(void*, ManagerFrame*) noexcept = nullptr;
    // Establishes the actor bridges and scoped renderer state for this event.
    // No default success: absent transform/clipping/lifetime proof -> false.
    bool (*enter_scope)(void*, const ManagerSite&, const ManagerFrame&, u32 context) noexcept = nullptr;
    bool (*leave_scope)(void*) noexcept = nullptr;
};
class ManagerDriver {
public:
    ManagerDriver(Lifecycle& life, ManagerHost host) : life_(life), host_(host) {}
    ManagerDriver(const ManagerDriver&) = delete;
    ManagerDriver& operator=(const ManagerDriver&) = delete;
    // Called at a proved quiescent point on the owning game thread. The ticket
    // must already be Live; this driver never constructs or destroys a widget.
    bool attach(u32 ticket);
    // Revokes dispatch immediately; never collects/destructs during a phase.
    void stop();
    bool event(u32 site, u32 manager, u32 context);
    // Branch component of JanuaryHost::permit, NOT the entire permission.
    // It grants no Structural/Destroy permission and no allocation certificate.
    bool phase_scope(WidgetKind, u32 unit, u32 phase, u32 context) const;
    bool in_event() const { return busy_; }
private:
    Lifecycle& life_; ManagerHost host_;
    u32 ticket_ = 0, thread_ = 0, last_frame_ = 0;
    u32 parents_[2]{}, lifetimes_[2]{};
    bool attached_ = false, busy_ = false, admitted_ = false;
    const ManagerSite* current_ = nullptr;
    u32 context_ = 0;
    bool sample(ManagerFrame&);
};
// Optional process-lifetime sink for the source-only gateways. Bind once BEFORE
// any hook is reachable; do not replace/free the object while hooks can run.
// This operation does not install hooks, patch memory or grant admission.
bool bind_manager_sink(ManagerDriver&);
}
extern "C" unsigned int rev_hud_manager_event(unsigned int site,
        unsigned int manager, unsigned int context);
