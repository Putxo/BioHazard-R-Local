#pragma once
#include "target_lifetime.hpp"
#include "progress.hpp"
namespace rev_genesis {
struct ViewSample {
    // January Vector3 stores x/y/z plus a canonical fourth lane. These are
    // IEEE754 bit patterns, not integer coordinates. Persistent +20 scan state
    // and rewards deliberately do not belong to this view-derived record.
    u32 position[3]{};
    u32 focus=0;
};
struct ViewFrame {Owner owner{};u32 number=0;};
struct TargetViewHost {
    void* context=nullptr;
    bool (*on_thread)(void*) noexcept=nullptr;
    Mode (*resolve)(void*,u32 widget,ViewFrame*) noexcept=nullptr;
};
// Source component for the forthcoming producer/consumer gateways. No native
// target memory is modified. Every entry is tied to observed target and actor
// generations; a reused address cannot inherit an old view sample.
class TargetViews {
public:
    TargetViews(TargetLifetime& life,TargetViewHost host):life_(life),host_(host){}
    Mode publish(u32 widget,TargetKey target,const ViewSample&) noexcept;
    Mode read(u32 widget,TargetKey target,ViewSample* out) noexcept;
private:
    struct Entry {TargetKey target{};ViewSample sample{};u32 frame=0;};
    TargetLifetime& life_;
    TargetViewHost host_{};
    Entry entries_[TargetLifetime::Capacity]{};
    Owner owner_{};
    bool bound_=false,busy_=false;
    u32 frame_=0;
    bool prepare(const ViewFrame&) noexcept;
    bool same(const ViewFrame&,const ViewFrame&) const noexcept;
    Entry* find(TargetKey,bool create) noexcept;
};
}
