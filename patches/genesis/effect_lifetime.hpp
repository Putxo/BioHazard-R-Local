#pragma once
#include "../hud_ownership/lifecycle.hpp"
namespace rev_genesis {
using rev_hud::u32;
enum class EffectKind : u32 { FilterSet, TVNoise, Outline };
struct EffectKey {
    u32 address=0,generation=0;
    EffectKind kind=EffectKind::FilterSet;
    bool valid() const noexcept {return address && generation && static_cast<u32>(kind)<3;}
};
struct EffectHost {
    rev_hud::Reader memory{};
    u32 (*thread)(void*) noexcept=nullptr;
};
// Records the three concrete scheduled filter types, including stock instances.
// Birth is a constructor VT-store observation, NOT completed initialization.
// Death revokes before native cleanup. The caller must separately pin graph
// ownership, completed initialization and a serialized use window.
class EffectLifetime {
public:
    static constexpr u32 Capacity=256;
    explicit EffectLifetime(EffectHost host):host_(host){}
    bool start(u32 thread) noexcept;
    bool event(u32 site,u32 unit) noexcept;
    EffectKey capture(u32 unit,EffectKind kind) noexcept;
    bool live(EffectKey key) noexcept;
    bool healthy() const noexcept;
private:
    EffectHost host_{};
    EffectKey entries_[Capacity]{};
    u32 thread_=0,next_=0;
    alignas(4) mutable volatile int poison_=0;
    bool busy_=false;
    bool owner() const noexcept;
    void poison() noexcept;
};
bool bind_effects(EffectLifetime&) noexcept;
}
extern "C" void rev_genesis_effect_event(unsigned int site,unsigned int unit) noexcept;
