#include "begin_activator.hpp"

namespace rev_hud {
namespace {
BeginActivator* Sink = nullptr;
struct Busy {
    bool& value;
    explicit Busy(bool& v) : value(v) { value = true; }
    ~Busy() { value = false; }
};
}

BeginActivationOps NativeBeginBindings::callbacks() noexcept {
    BeginActivationOps out{};
    out.context = this;
    out.ticket = [](void* p) noexcept {
        return static_cast<NativeBeginBindings*>(p)->coordinator_.ticket();
    };
    out.attach = [](void* p, u32 ticket) noexcept {
        return static_cast<NativeBeginBindings*>(p)->driver_.attach(ticket);
    };
    out.stop = [](void* p) noexcept {
        static_cast<NativeBeginBindings*>(p)->driver_.stop();
    };
    return out;
}

bool BeginActivator::event(u32 site, u32 owner, u32 frame_base) noexcept {
    if (busy_)
        return fail(BeginActivationFault::Protocol);

    if (fault_ != BeginActivationFault::None ||
        site != PipelineBegin || !owner || !frame_base || (frame_base & 3) ||
        !ops_.ticket || !ops_.attach || !ops_.stop)
        return fail(BeginActivationFault::Config);

    PipelineStamp stamp{};
    if (!clock_.capture(&stamp) ||
        stamp.owner != owner || stamp.frame_base != frame_base)
        return fail(BeginActivationFault::Clock);

    Busy lock(busy_);
    const u32 ticket = ops_.ticket(ops_.context);

    if (!ticket) {
        attached_ticket_ = 0;
        rejected_ticket_ = 0;
        return true;
    }

    if (ticket == attached_ticket_)
        return true;

    if (ticket == rejected_ticket_)
        return false;

    if (ops_.attach(ops_.context, ticket)) {
        attached_ticket_ = ticket;
        rejected_ticket_ = 0;
        return true;
    }

    // attach() may fail because the lifetime snapshot changed between END and
    // this BEGIN. Revocation is safe here; destruction remains deferred to the
    // next StructuralWindow task.
    ops_.stop(ops_.context);
    attached_ticket_ = 0;
    rejected_ticket_ = ticket;
    fault_ = BeginActivationFault::Attach;
    return false;
}

bool bind_begin_activator_sink(BeginActivator& activator) noexcept {
    if (Sink) return false;
    Sink = &activator;
    return true;
}

} // namespace rev_hud

extern "C" unsigned int rev_hud_begin_activation_event(
        unsigned int site, unsigned int owner, unsigned int frame_base) noexcept {
    return rev_hud::Sink &&
           rev_hud::Sink->event(site, owner, frame_base) ? 1u : 0u;
}
