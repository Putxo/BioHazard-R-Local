#include "structural_coordinator.hpp"

namespace rev_hud {
namespace {

struct Busy {
    bool& value;
    explicit Busy(bool& v) : value(v) { value = true; }
    ~Busy() { value = false; }
};

bool same_actor(const Actor& a, const Actor& b) noexcept {
    return a.address == b.address &&
           a.lifetime == b.lifetime &&
           a.serial == b.serial &&
           a.think_mode == b.think_mode;
}

} // namespace

CoordinatorOps NativeCoordinatorBindings::callbacks() noexcept {
    CoordinatorOps out{};
    out.context = this;
    out.capture = [](void* p, LifeSnapshot* snap) noexcept {
        return snap && static_cast<NativeCoordinatorBindings*>(p)->source_.capture(snap);
    };
    out.healthy = [](void* p) noexcept {
        return static_cast<NativeCoordinatorBindings*>(p)->source_.fault() == SourceFault::None;
    };
    out.state = [](void* p) noexcept {
        return static_cast<NativeCoordinatorBindings*>(p)->life_.state();
    };
    out.allocation_clean = [](void* p) noexcept {
        return static_cast<NativeCoordinatorBindings*>(p)->ledger_.retained() == 0;
    };
    out.configure = [](void* p, const u32 parents[2], const u32 originals[WidgetKinds]) noexcept {
        return static_cast<NativeCoordinatorBindings*>(p)->backend_.configure(parents, originals);
    };
    out.prepare = [](void* p, const Session& session,
                     const u32 parents[2], const u32 originals[WidgetKinds]) noexcept {
        return static_cast<NativeCoordinatorBindings*>(p)->life_.prepare(
            session, parents, originals);
    };
    out.publish = [](void* p, u32 ticket, const Session& session) noexcept {
        return static_cast<NativeCoordinatorBindings*>(p)->life_.publish(ticket, session);
    };
    out.stop = [](void* p) noexcept {
        static_cast<NativeCoordinatorBindings*>(p)->driver_.stop();
    };
    out.collect = [](void* p) noexcept {
        return static_cast<NativeCoordinatorBindings*>(p)->life_.collect();
    };
    return out;
}

bool StructuralCoordinator::word(u32 address, u32& value) const noexcept {
    return address && address <= Invalid - 3 &&
           memory_.word && memory_.word(memory_.context, address, &value);
}

bool StructuralCoordinator::same_snapshot(const LifeSnapshot& a,
                                          const LifeSnapshot& b) const noexcept {
    if (a.session.active != b.session.active ||
        a.session.epoch != b.session.epoch ||
        !same_actor(a.session.self, b.session.self) ||
        !same_actor(a.session.sub0, b.session.sub0))
        return false;

    for (u32 i = 0; i < 2; ++i)
        if (a.parents[i] != b.parents[i] ||
            a.parent_lifetimes[i] != b.parent_lifetimes[i])
            return false;

    return true;
}

bool StructuralCoordinator::originals(const LifeSnapshot& snap,
                                      u32 out[WidgetKinds]) const noexcept {
    if (!valid_widget_count(count_) || !out || !snap.session.active || !snap.session.epoch ||
        !snap.parents[0] || !snap.parents[1])
        return false;

    u32 value = 0;
    if (!word(snap.parents[0], value) ||
        value != manager_vtable(ManagerKind::Cockpit) ||
        !word(snap.parents[1], value) ||
        value != manager_vtable(ManagerKind::MiniMap))
        return false;

    // Lifecycle order: Reticle, MainEquipment, MapHerb.
    u32 map_alias = 0;
    if (!word(snap.parents[0] + 0x90, out[0]) ||
        !word(snap.parents[0] + 0x5C, out[1]) ||
        !word(snap.parents[1] + 0x30, out[2]) ||
        !word(snap.parents[1] + 0x40, map_alias) ||
        !out[0] || !out[1] || !out[2] ||
        map_alias != out[2] ||
        out[0] == out[1] || out[0] == out[2] || out[1] == out[2])
        return false;

    if (count_ >= 4 && !word(snap.parents[0] + 0x64, out[3])) return false;
    if (count_ >= 5 && !word(snap.parents[0] + 0x3C, out[4])) return false;
    if (count_ >= 6 && !word(snap.parents[0] + 0x44, out[5])) return false;
    if (count_ >= 7 && !word(snap.parents[0] + 0x48, out[6])) return false;
    for (u32 i = 0; i < count_; ++i) {
        if (!out[i]) return false;
        for (u32 j=0;j<i;++j) if (out[i]==out[j]) return false;
        if (!word(out[i], value) ||
            value != kind_info(static_cast<WidgetKind>(i))->vtable)
            return false;
    }
    return true;
}

bool StructuralCoordinator::drain() noexcept {
    const LifeState before = ops_.state(ops_.context);
    if (before == LifeState::Quarantined)
        return fail(CoordinatorFault::Quarantined);

    if (before == LifeState::Empty) {
        if (!ops_.allocation_clean(ops_.context))
            return fail(CoordinatorFault::Allocation);
        ticket_ = 0;
        current_ = {};
        return true;
    }

    ops_.stop(ops_.context);
    const LifeState stopped = ops_.state(ops_.context);
    if (stopped == LifeState::Quarantined)
        return fail(CoordinatorFault::Quarantined);

    if (stopped != LifeState::Empty && !ops_.collect(ops_.context)) {
        return fail(ops_.state(ops_.context) == LifeState::Quarantined
                    ? CoordinatorFault::Quarantined
                    : CoordinatorFault::Collect);
    }

    if (ops_.state(ops_.context) != LifeState::Empty)
        return fail(CoordinatorFault::Collect);
    if (!ops_.allocation_clean(ops_.context))
        return fail(CoordinatorFault::Allocation);

    ticket_ = 0;
    current_ = {};
    return true;
}

bool StructuralCoordinator::create(const LifeSnapshot& snap) noexcept {
    u32 original[WidgetKinds]{};
    if (!originals(snap, original))
        return fail(CoordinatorFault::Layout);
    if (!ops_.allocation_clean(ops_.context))
        return fail(CoordinatorFault::Allocation);

    if (!ops_.configure(ops_.context, snap.parents, original))
        return fail(CoordinatorFault::Backend);

    const u32 ticket = ops_.prepare(
        ops_.context, snap.session, snap.parents, original);
    if (!ticket) {
        return fail(ops_.state(ops_.context) == LifeState::Quarantined
                    ? CoordinatorFault::Quarantined
                    : CoordinatorFault::Prepare);
    }

    if (!ops_.publish(ops_.context, ticket, snap.session)) {
        ops_.stop(ops_.context);
        const LifeState stopped = ops_.state(ops_.context);
        if (stopped != LifeState::Empty &&
            !ops_.collect(ops_.context))
            return fail(CoordinatorFault::Collect);
        if (ops_.state(ops_.context) != LifeState::Empty ||
            !ops_.allocation_clean(ops_.context))
            return fail(CoordinatorFault::Collect);
        return fail(CoordinatorFault::Publish);
    }

    if (ops_.state(ops_.context) != LifeState::Live)
        return fail(CoordinatorFault::Publish);

    ticket_ = ticket;
    current_ = snap;
    return true;
}

bool StructuralCoordinator::run(u32 frame, u32 owner, u32 frame_base) noexcept {
    if (busy_)
        return fail(CoordinatorFault::Protocol);

    if (fault_ != CoordinatorFault::None ||
        !frame || !owner || !frame_base || (frame_base & 3) ||
        frame <= last_frame_ ||
        !memory_.word ||
        !ops_.capture || !ops_.healthy || !ops_.state ||
        !ops_.allocation_clean || !ops_.configure || !ops_.prepare ||
        !ops_.publish || !ops_.stop || !ops_.collect)
        return fail(CoordinatorFault::Config);

    Busy lock(busy_);
    last_frame_ = frame;

    LifeSnapshot snap{};
    const bool captured = ops_.capture(ops_.context, &snap);
    if (!captured) {
        if (!ops_.healthy(ops_.context))
            return fail(CoordinatorFault::Source);

        if (ops_.state(ops_.context) == LifeState::Empty) {
            if (!ops_.allocation_clean(ops_.context))
                return fail(CoordinatorFault::Allocation);
            ticket_ = 0;
            current_ = {};
            return true;
        }
        return drain();
    }

    const LifeState state = ops_.state(ops_.context);
    if (state == LifeState::Quarantined)
        return fail(CoordinatorFault::Quarantined);

    if (state == LifeState::Draining || state == LifeState::Prepared)
        return drain();

    if (state == LifeState::Live) {
        if (ticket_ && same_snapshot(current_, snap))
            return true;

        // A changed life is retired now; a replacement is created no earlier
        // than the next structural END, keeping teardown and construction
        // separate even when the new snapshot is already visible.
        return drain();
    }

    if (state != LifeState::Empty)
        return fail(CoordinatorFault::Protocol);

    return create(snap);
}

StructuralTask StructuralCoordinator::task() noexcept {
    return {this, [](void* p, u32 frame, u32 owner, u32 frame_base) noexcept {
        return static_cast<StructuralCoordinator*>(p)->run(
            frame, owner, frame_base);
    }};
}

} // namespace rev_hud
