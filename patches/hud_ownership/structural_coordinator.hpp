#pragma once
#include "allocation_ledger.hpp"
#include "lifetime_source.hpp"
#include "manager_driver.hpp"

namespace rev_hud {

struct CoordinatorOps {
    void* context = nullptr;
    bool (*capture)(void*, LifeSnapshot*) noexcept = nullptr;
    bool (*healthy)(void*) noexcept = nullptr;
    LifeState (*state)(void*) noexcept = nullptr;
    bool (*allocation_clean)(void*) noexcept = nullptr;
    bool (*configure)(void*, const u32 parents[2], const u32 originals[WidgetKinds]) noexcept = nullptr;
    u32 (*prepare)(void*, const Session&, const u32 parents[2], const u32 originals[WidgetKinds]) noexcept = nullptr;
    bool (*publish)(void*, u32 ticket, const Session&) noexcept = nullptr;
    void (*stop)(void*) noexcept = nullptr;
    bool (*collect)(void*) noexcept = nullptr;
};

class NativeCoordinatorBindings {
public:
    NativeCoordinatorBindings(LifetimeSource& source, Lifecycle& life,
                              JanuaryBackend& backend, ManagerDriver& driver,
                              JanuaryAllocationLedger& ledger)
        : source_(source), life_(life), backend_(backend),
          driver_(driver), ledger_(ledger) {}

    CoordinatorOps callbacks() noexcept;

private:
    LifetimeSource& source_;
    Lifecycle& life_;
    JanuaryBackend& backend_;
    ManagerDriver& driver_;
    JanuaryAllocationLedger& ledger_;
};

enum class CoordinatorFault : u32 {
    None, Config, Protocol, Source, Layout, Allocation,
    Backend, Prepare, Publish, Collect, Quarantined
};

class StructuralCoordinator {
public:
    StructuralCoordinator(Reader memory, CoordinatorOps ops, u32 count=LegacyWidgetKinds)
        : memory_(memory), ops_(ops), count_(count) {}

    StructuralCoordinator(const StructuralCoordinator&) = delete;
    StructuralCoordinator& operator=(const StructuralCoordinator&) = delete;

    StructuralTask task() noexcept;
    bool run(u32 frame, u32 owner, u32 frame_base) noexcept;

    u32 ticket() const noexcept { return ticket_; }
    bool live() const noexcept {
        return ticket_ != 0 && fault_ == CoordinatorFault::None;
    }
    CoordinatorFault fault() const noexcept { return fault_; }

private:
    Reader memory_{};
    CoordinatorOps ops_{};
    const u32 count_;
    LifeSnapshot current_{};
    u32 ticket_ = 0;
    u32 last_frame_ = 0;
    bool busy_ = false;
    CoordinatorFault fault_ = CoordinatorFault::None;

    bool fail(CoordinatorFault fault) noexcept {
        fault_ = fault;
        return false;
    }
    bool word(u32 address, u32& value) const noexcept;
    bool originals(const LifeSnapshot& snap, u32 out[WidgetKinds]) const noexcept;
    bool same_snapshot(const LifeSnapshot& a, const LifeSnapshot& b) const noexcept;
    bool drain() noexcept;
    bool create(const LifeSnapshot& snap) noexcept;
};

} // namespace rev_hud
