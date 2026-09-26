#pragma once
#include "january_admission.hpp"
#include "structural_window.hpp"
namespace rev_hud {
struct AllocationGate {
    void* context=nullptr;
    bool (*safe)(void*) noexcept=nullptr;
};
inline AllocationGate structural_allocation_gate(StructuralWindow& w) noexcept {
    return {&w, [](void* c) noexcept {
        return static_cast<StructuralWindow*>(c)->safe();
    }};
}
enum class AllocationState : u32 { Empty, Pending, Certified };
enum class AllocationFault : u32 {
    None, Config, Unsafe, Protocol, Target, Native, Alignment,
    Overlap, Capacity, Changed
};
struct AllocationRecord {
    u32 address=0,size=0,allocator=0,constructor=0,destroy=0;
    WidgetKind kind=WidgetKind::Reticle;
    AllocationState state=AllocationState::Empty;
};
class JanuaryAllocationLedger {
public:
    JanuaryAllocationLedger(AllocationGate gate, JanuaryCalls native)
        : gate_(gate), native_(native) {}
    JanuaryAllocationLedger(const JanuaryAllocationLedger&)=delete;
    JanuaryAllocationLedger& operator=(const JanuaryAllocationLedger&)=delete;
    // Wrapped calls are passed to JanuaryBackend instead of the raw table.
    JanuaryCalls callbacks() noexcept;
    // Exact Pending -> Certified transition used by fresh_allocation.
    bool certify(u32 address,u32 size) noexcept;
    bool owns(u32 address,u32 size=0) const noexcept;
    u32 retained() const noexcept;
    AllocationFault fault() const noexcept { return fault_; }
private:
    struct Spec {
        WidgetKind kind; u32 size,allocator,constructor,destroy;
    };
    static const Spec* spec_for_allocator(u32 target,u32 size,u32 align) noexcept;
    static const Spec* spec_for_constructor(u32 target) noexcept;
    static const Spec* spec_for_destroy(u32 target) noexcept;
    AllocationGate gate_{};
    JanuaryCalls native_{};
    AllocationRecord records_[3]{};
    bool busy_=false;
    AllocationFault fault_=AllocationFault::None;
    bool safe() const noexcept;
    bool fail(AllocationFault f) noexcept { fault_=f; return false; }
    AllocationRecord* find(u32 address) noexcept;
    const AllocationRecord* find(u32 address) const noexcept;
    AllocationRecord* empty() noexcept;
    bool overlaps(u32 address,u32 size) const noexcept;
    u32 allocate(u32 target,u32 size,u32 align) noexcept;
    u32 construct(u32 target,u32 self) noexcept;
    void method0(u32 target,u32 self) noexcept;
    void method1(u32 target,u32 self,u32 argument) noexcept;
    u32 singleton(u32 target) noexcept;
    u32 contains(u32 target,u32 self,u32 unit) noexcept;
};
// Rebinds AdmissionServices to the same proven StructuralWindow gate and ledger.
// Existing image/thread/memory/write checks are forwarded, not replaced.
class AllocationAdmissionBridge {
public:
    AllocationAdmissionBridge(AllocationGate gate, JanuaryAllocationLedger& ledger,
                              AdmissionServices upstream)
        : gate_(gate),ledger_(ledger),upstream_(upstream) {}
    AdmissionServices callbacks() noexcept;
private:
    AllocationGate gate_{};
    JanuaryAllocationLedger& ledger_;
    AdmissionServices upstream_{};
    bool safe() const noexcept;
};
}
