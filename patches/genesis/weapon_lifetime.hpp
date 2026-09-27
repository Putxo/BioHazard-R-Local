#pragma once
#include "../hud_ownership/lifecycle.hpp"
namespace rev_genesis {
using rev_hud::u32;
struct WeaponKey {
    u32 address=0,generation=0;
    bool valid() const noexcept {return address && generation;}
};
struct WeaponHost {
    rev_hud::Reader memory{};
    u32 (*thread)(void*) noexcept=nullptr;
};
// Concrete uWpScanner observations, not a generic weapon registry. This records
// identity; it does not defer native deletion or pin storage through callbacks.
class WeaponLifetime {
public:
    static constexpr u32 Capacity=64;
    explicit WeaponLifetime(WeaponHost host):host_(host){}
    bool start(u32 thread) noexcept;
    bool event(u32 site,u32 weapon) noexcept;
    WeaponKey capture(u32 weapon) noexcept;
    bool live(WeaponKey key) noexcept;
    u32 slot(WeaponKey key) noexcept;
    bool healthy() const noexcept;
private:
    WeaponHost host_{};
    WeaponKey entries_[Capacity]{};
    u32 thread_=0,next_=0;
    // Off-thread events cannot safely update the owner-thread table. They
    // atomically poison admission so a missed destructor can never look live.
    alignas(4) mutable volatile int poison_=0;
    bool busy_=false;
    bool owner() const noexcept;
    void poison() noexcept;
};
bool bind_weapons(WeaponLifetime&) noexcept;
}
extern "C" void rev_genesis_weapon_event(unsigned int site,unsigned int weapon) noexcept;
