#pragma once
#include "effect_lifetime.hpp"
#include "filter_resource.hpp"
namespace rev_genesis {
struct Effects {
    u32 scanner=0;
    EffectKey units[3]{}; // FilterSet, TVNoise, Outline
    FilterResource resource{};
};
// Scanner and scheduler lifetimes must be serialized by the caller. Each root
// must have an observed generation before its memory is inspected. Resource
// children are pinned through the FilterSet; their base identities are checked,
// not the full recursive render-resource allocation graph.
bool capture_effects(Reader,EffectLifetime&,u32 scanner,Effects*) noexcept;
bool same_effects(const Effects&,const Effects&) noexcept;
bool disjoint_effects(const Effects&,const Effects&) noexcept;
bool effect_view(Reader,const Effects&,u32 view) noexcept;
// Requires an exclusively owned graph and a serialized structural window.
// Disconnects the embedded callback, disables activity, marks native state3,
// then clears the three parent pointers. Scheduler retains deletion ownership.
// A partial failure MUST retain/quarantine the Scanner; never run its destructor.
bool retire_effects(Reader,EffectLifetime&,const Effects&,
                    bool (*write)(void*,u32,u32) noexcept) noexcept;
}
