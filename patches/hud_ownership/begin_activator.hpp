#pragma once
#include "pipeline_clock.hpp"
#include "structural_coordinator.hpp"
#include "manager_driver.hpp"

namespace rev_hud {

struct BeginActivationOps {
    void* context = nullptr;
    u32 (*ticket)(void*) noexcept = nullptr;
    bool (*attach)(void*, u32 ticket) noexcept = nullptr;
    void (*stop)(void*) noexcept = nullptr;
};

class NativeBeginBindings {
public:
    NativeBeginBindings(StructuralCoordinator& coordinator, ManagerDriver& driver)
        : coordinator_(coordinator), driver_(driver) {}

    BeginActivationOps callbacks() noexcept;

private:
    StructuralCoordinator& coordinator_;
    ManagerDriver& driver_;
};

enum class BeginActivationFault : u32 {
    None, Config, Protocol, Clock, Attach
};

class BeginActivator {
public:
    BeginActivator(PipelineClock& clock, BeginActivationOps ops)
        : clock_(clock), ops_(ops) {}

    BeginActivator(const BeginActivator&) = delete;
    BeginActivator& operator=(const BeginActivator&) = delete;

    bool event(u32 site, u32 owner, u32 frame_base) noexcept;

    u32 attached_ticket() const noexcept { return attached_ticket_; }
    u32 rejected_ticket() const noexcept { return rejected_ticket_; }
    BeginActivationFault fault() const noexcept { return fault_; }

private:
    PipelineClock& clock_;
    BeginActivationOps ops_{};
    u32 attached_ticket_ = 0;
    u32 rejected_ticket_ = 0;
    bool busy_ = false;
    BeginActivationFault fault_ = BeginActivationFault::None;

    bool fail(BeginActivationFault f) noexcept {
        fault_ = f;
        return false;
    }
};

bool bind_begin_activator_sink(BeginActivator&) noexcept;

} // namespace rev_hud

extern "C" unsigned int rev_hud_begin_activation_event(
    unsigned int site, unsigned int owner, unsigned int frame_base) noexcept;
