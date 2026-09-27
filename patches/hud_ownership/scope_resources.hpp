#pragma once
#include "lifecycle.hpp"
namespace rev_hud {
struct ScopeResources {
    static constexpr u32 MaxAnimations=32; // Adapter bound, not an engine limit.
    GuiTree tree{};
    u32 table=0,count=0,animations[MaxAnimations]{},vtables[MaxAnimations]{};
};
bool capture_scope(Reader,u32 unit,ScopeResources*) noexcept;
bool same_scope(const ScopeResources&,const ScopeResources&) noexcept;
bool disjoint_scopes(const ScopeResources&,const ScopeResources&) noexcept;
}
