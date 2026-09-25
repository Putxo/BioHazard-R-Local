#pragma once
#include "lifecycle.hpp"
// Exact January engine operations. Still opt-in source: no installer or hooks.
namespace rev_hud {
enum class NativeOp : u32 { Structural, Read, Write, Initialize, Destroy, Phase };
enum class NativeFault : u32 { None, Permit, Memory, Layout, Borrowed, Allocation,
    Attached, Target, Resources, Reentrant, Quarantined };
struct JanuaryType {
    u32 size, allocator, constructor, destroy, initialize, phase8, phase9, draw;
};
const JanuaryType* january_type(WidgetKind);
struct JanuaryCalls {
    void* context = nullptr;
    u32 (*allocate)(void*, u32 target, u32 size, u32 align) noexcept = nullptr;
    u32 (*construct)(void*, u32 target, u32 self) noexcept = nullptr;
    void (*method0)(void*, u32 target, u32 self) noexcept = nullptr;
    void (*method1)(void*, u32 target, u32 self, u32 argument) noexcept = nullptr;
    u32 (*singleton)(void*, u32 target) noexcept = nullptr;
    u32 (*contains)(void*, u32 target, u32 self, u32 unit) noexcept = nullptr;
};
// On non-i386 builds returns an empty table, never silently casts 64-bit calls.
JanuaryCalls january_native_calls();
struct JanuaryHost {
    Reader memory{};
    bool (*write)(void*, u32, u32) noexcept = nullptr;
    // Not a snapshot boolean. Each call must check current driver admission.
    // Structural: exact loaded image/thread, no callbacks or render work in
    // flight, allocation lifetime serialized. Phase: correct manager branch,
    // installed actor bridges, current view transform/clipping and draw setup.
    // A driver with missing evidence MUST return false. No default permission.
    bool (*permit)(void*, NativeOp, WidgetKind, u32 unit, u32 phase, u32 context) noexcept = nullptr;
    // Must attest this entire allocation as newly returned by our allocator,
    // exclusively ours and disjoint from ALL live engine allocations/borrows.
    bool (*fresh_allocation)(void*, u32, u32) noexcept = nullptr;
};
class JanuaryBackend {
public:
    JanuaryBackend(JanuaryHost host, JanuaryCalls calls) : host_(host), calls_(calls) {}
    // Bind the same borrowed parents/originals passed to Lifecycle::prepare.
    // Must remain alive throughout preparation/publication. No live reconfigure.
    bool configure(const u32 parents[2], const u32 originals[3]);
    Backend callbacks();
    NativeFault fault() const { return fault_; }
    u32 retained(WidgetKind k) const;
private:
    struct Entry { u32 unit = 0; bool constructed = false, ready = false, quarantine = false; };
    JanuaryHost host_; JanuaryCalls calls_;
    Entry entries_[3]{};
    u32 parents_[2]{}, originals_[3]{};
    bool configured_ = false, busy_ = false;
    NativeFault fault_ = NativeFault::None;
    bool allow(NativeOp, WidgetKind, u32 unit=0, u32 phase=0, u32 context=0);
    bool word(u32, u32&); bool field(u32, u32, u32&);
    bool write(u32, u32);
    bool targets(WidgetKind); bool detached(u32, WidgetKind);
    bool borrowed_or_overlap(u32, u32) const;
    bool safe();
    u32 create(WidgetKind);
    bool init(u32, WidgetKind); bool destroy(u32, WidgetKind);
    bool phase(u32, WidgetKind, u32, u32);
    bool fail(NativeFault f) { fault_ = f; return false; }
};
}
