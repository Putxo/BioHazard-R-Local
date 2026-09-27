#pragma once
#include "../hud_ownership/lifecycle.hpp"
namespace rev_genesis {
using rev_hud::u32;
using rev_hud::Reader;
// Identity certificate, not a recursive allocation/lifetime certificate.
// The January resource owns an MtArray of mutable cUnit-derived filters.
struct FilterResource {
    static constexpr u32 Limit=64;
    u32 resource=0,table=0,count=0,capacity=0;
    u32 units[Limit]{},vtables[Limit]{};
};
bool capture_filter_resource(Reader,u32,FilterResource*) noexcept;
bool same_filter_resource(const FilterResource&,const FilterResource&) noexcept;
bool disjoint_filter_resources(const FilterResource&,const FilterResource&) noexcept;
struct FilterCopyHost {
    Reader memory{};
    void* context=nullptr;
    // Allocates AND constructs a fresh rFilterSet with one reference, no cache
    // key and no accounting bytes. The buffer's native allocator owns it.
    u32 (*create)(void*) noexcept=nullptr;
    // Synchronous native serialization/deserialization through a private stream.
    bool (*copy)(void*,u32 source,u32 destination) noexcept=nullptr;
    void (*release)(void*,u32 resource) noexcept=nullptr;
};
// Source and children must be pinned by the caller throughout the transaction.
// Does not consume source's reference. Returns one reference to an independent
// resource; caller must release it. Never installs a pointer on a filter unit.
u32 copy_filter_resource(const FilterCopyHost&,u32 source) noexcept;
FilterCopyHost january_filter_copy_host(Reader) noexcept;
}
