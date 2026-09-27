#pragma once
#include "../hud_ownership/lifecycle.hpp"
namespace rev_genesis {
using rev_hud::u32;
struct TargetKey {
    u32 address=0,generation=0;
    bool valid() const noexcept {return address && generation;}
};
struct TargetHost {
    rev_hud::Reader memory{};
    u32 (*thread)(void*) noexcept=nullptr;
};
// Observations, never pointer discovery. A generation is issued only at the
// pinned constructor boundary; death revokes it without dereferencing memory.
class TargetLifetime {
public:
    static constexpr u32 Capacity=512;
    explicit TargetLifetime(TargetHost host):host_(host){}
    bool start(u32 thread) noexcept;
    bool event(u32 site,u32 target) noexcept;
    TargetKey capture(u32 target) noexcept;
    bool live(TargetKey key) noexcept;
    bool healthy() const noexcept;
private:
    TargetHost host_{};
    TargetKey entries_[Capacity]{};
    u32 thread_=0,next_=0;
    // Off-thread events cannot safely update the owner-thread table. They
    // atomically poison admission so a missed destructor can never look live.
    alignas(4) mutable volatile int poison_=0;
    bool busy_=false;
    bool owner() const noexcept;
    void poison() noexcept;
};
bool bind_targets(TargetLifetime&) noexcept;
}
extern "C" void rev_genesis_target_event(unsigned int site,unsigned int target) noexcept;
