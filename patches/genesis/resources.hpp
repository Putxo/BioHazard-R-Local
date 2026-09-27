#pragma once
#include "../hud_ownership/lifecycle.hpp"
namespace rev_genesis {
using rev_hud::u32;
using rev_hud::Reader;
struct Region {u32 address=0,bytes=0;};
struct Resources {
    static constexpr u32 Capacity=1024;
    u32 unit=0,count=0;
    Region regions[Capacity]{};
};
// January scanner only. Call while the host pins its native lifetime.
// Enumerates mutable GUI trees, animations and the two owned target pools.
// Resource templates are borrowed and deliberately excluded from the graph.
bool capture_resources(Reader,u32 scanner,Resources*) noexcept;
bool disjoint(const Resources&,const Resources&) noexcept;
bool same_resources(const Resources&,const Resources&) noexcept;
}
