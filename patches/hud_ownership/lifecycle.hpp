#pragma once
#include "ownership.hpp"
// Source-only detached HUD controller. No hooks are installed here.
namespace rev_hud {
struct Reader {
    void* context;
    bool (*word)(void*, u32, u32*) noexcept;
};
struct GuiTree {
    static constexpr u32 MaxNodes = 256; // conservative adapter cap, not game limit
    u32 unit = 0, resource = 0, root = 0, table = 0, count = 0;
    u32 nodes[MaxNodes]{};
};
// Read-only January layout. Caller must serialize against engine destruction.
// Failure never publishes a partially captured graph. Same resource is allowed;
// root, node table, nodes and their owner fields must be instance-specific.
bool capture_tree(Reader reader, u32 unit, WidgetKind kind, GuiTree* out);
bool disjoint_trees(const GuiTree& a, const GuiTree& b);

struct Backend {
    Reader memory;
    // True only on the game thread between phases, with render work drained.
    bool (*safe_point)(void*) noexcept;
    // Contract: freshly constructed, exclusively owned, detached unit or zero.
    // It MUST NOT attach the unit to native arrays, groups or scheduling lists.
    u32 (*construct)(void*, WidgetKind) noexcept;
    // Native initialization returns void. Postconditions are checked separately.
    void (*initialize)(void*, u32, WidgetKind) noexcept;
    // One scalar destructor(flag=1); never individually free root or nodes.
    void (*destroy)(void*, u32, WidgetKind) noexcept;
    // Forward exactly one native virtual phase: 8, 9 or 11, preserving context.
    void (*phase)(void*, u32, WidgetKind, u32, u32) noexcept;
    bool (*write_word)(void*, u32, u32) noexcept;
};
enum class LifeState : u32 { Empty, Preparing, Prepared, Live, Draining, Quarantined };
enum class LifeError : u32 { None, Busy, UnsafePoint, BadInput, Construct,
    Resources, Alias, Registration, Stale, Memory, UnsupportedPhase };
class Lifecycle {
public:
    explicit Lifecycle(Registry& registry, Backend backend) : registry_(registry), backend_(backend) {}
    // All three originals are borrowed P1 units. Parent addresses order:
    // cockpit, minimap. No pointers are installed in their native slot arrays.
    u32 prepare(const Session&, const u32 parents[2], const u32 originals[3]);
    bool publish(u32 ticket, const Session& current);
    // Stops are latched immediately, including reentrant requests from phase().
    void stop();
    void observe(const Session& current);
    void parent_destroying(u32 address);
    // Destructors run only at a safe point and outside an in-flight callback.
    bool collect();
    // Frame is a positive monotonically increasing driver stamp. Draw only P2
    // view 1; original P1 phases remain exclusively owned by the native manager.
    bool dispatch(u32 ticket, const Session& current, u32 frame, u32 phase, u32 view, u32 context);
    LifeState state() const { return state_; }
    LifeError error() const { return error_; }
    u32 unit(u32 index) const { return index < 3 ? units_[index] : 0; }
private:
    Registry& registry_;
    Backend backend_;
    LifeState state_ = LifeState::Empty;
    LifeError error_ = LifeError::None;
    bool busy_ = false;
    u32 ticket_ = 0, frame_ = 0, phases_ = 0;
    u32 parents_[2]{}, originals_[3]{}, units_[3]{};
    bool quarantine_[3]{};
    Token managers_[2]{}, widgets_[3]{};
    GuiTree trees_[3]{}, stocks_[3]{};
    bool valid_backend() const;
    bool live_routes() const;
    bool verify_graphs();
    bool can_destroy(u32 index) const;
    void fail(LifeError error);
};
}
