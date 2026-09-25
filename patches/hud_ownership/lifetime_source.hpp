#pragma once
#include "ownership.hpp"
// Event-fed January lifetime evidence. Source only: no hook installation, no
// allocation/free, no renderer fence, no guessed presentation-frame counter.
namespace rev_hud {
struct LifetimeAccess {
    void* context = nullptr;
    bool (*word)(void*, u32 address, u32* out) noexcept = nullptr;
    u32 (*thread_id)(void*) noexcept = nullptr;
    // January cdecl 0x01C16468 -> 0x026D7EF0, not the ret4 Self predicate.
    u32 (*stock_self)(void*) noexcept = nullptr;
};
struct LifeSnapshot {
    Session session{};
    u32 parents[2]{}, parent_lifetimes[2]{}, revision = 0;
    // Intentionally no frame: the sUnit update stamp freezes during pause.
};
enum class LifeObject : u32 { Actor, Cockpit, MiniMap };
enum class SourceFault : u32 { None, Protocol, Capacity, CounterExhausted };
struct LifetimeSite { u32 address, event; LifeObject object; };
// event: 0=constructor tail, 1=destructor entry, 2/3=binder begin/end,
// 4=PCS destructor. Only these exact source entry sites are accepted.
const LifetimeSite* lifetime_site(u32 address);
class LifetimeSource {
public:
    static constexpr u32 Capacity = 256, BindCapacity = 8; // adapter limits
    LifetimeSource(Registry& registry, LifetimeAccess access)
        : registry_(registry), access_(access) {}
    LifetimeSource(const LifetimeSource&) = delete;
    LifetimeSource& operator=(const LifetimeSource&) = delete;
    // Observer must start before world constructors, with ALL matching hooks
    // covered. This arms a source receiver; it does not prove/install hooks.
    bool start(u32 owning_thread);
    bool event(u32 exact_site, u32 object);
    // Parents must have observed constructor events. Does not read pointers or
    // promote a first-seen phase callback to a birth event.
    bool select_managers(u32 cockpit, u32 minimap);
    bool capture(LifeSnapshot* out);
    SourceFault fault() const { return fault_; }
    u32 epoch() const { return epoch_; }
    u32 binder_depth() const { return depth_; }
private:
    struct Live { u32 address=0, generation=0; LifeObject kind=LifeObject::Actor; bool alive=false; };
    struct Role { u32 pcs=0; Token token{}; Actor actor{}; u32 vtable=0; };
    struct Binding { u32 pcs=0, role=Invalid; };
    Registry& registry_; LifetimeAccess access_;
    Live live_[Capacity]{}; Role roles_[2]{};
    Token parents_[2]{}; Binding bindings_[BindCapacity]{};
    u32 thread_=0, next_generation_=0, revision_=1, epoch_=1, depth_=0;
    bool capturing_=false, was_ready_=false;
    SourceFault fault_=SourceFault::None;
    bool on_thread() const;
    bool word(u32 address, u32& value, u32 revision);
    bool field(u32 address, u32 offset, u32& value, u32 revision);
    bool change(bool session_changed);
    bool poison(SourceFault);
    void revoke();
    Token find(u32 address, LifeObject) const;
    const Live* get(Token, LifeObject) const;
    bool born(u32 address, LifeObject);
    bool dying(u32 address, LifeObject);
    bool bind_begin(u32 pcs);
    bool bind_end(u32 pcs);
    bool pcs_dying(u32 pcs);
    bool actor(Token, Actor&, u32& vtable, u32 revision);
};
// Bind once while no hook is reachable; no hot swap or concurrent teardown.
bool bind_lifetime_sink(LifetimeSource&);
}
extern "C" unsigned int rev_hud_lifetime_event(unsigned int site, unsigned int object);
