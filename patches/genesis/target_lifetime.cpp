#include "target_lifetime.hpp"
#if defined(_MSC_VER)
#include <intrin.h>
#endif
namespace rev_genesis {
namespace {
TargetLifetime* Sink=nullptr;
struct Busy {bool& value;explicit Busy(bool& v):value(v){value=true;}~Busy(){value=false;}};
}
bool TargetLifetime::healthy() const noexcept {
#if defined(_MSC_VER)
    static_assert(sizeof(long)==sizeof(int),"Win32 atomic width");
    return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&poison_),0,0)==0;
#else
    return __atomic_load_n(&poison_,__ATOMIC_ACQUIRE)==0;
#endif
}
void TargetLifetime::poison() noexcept {
#if defined(_MSC_VER)
    (void)_InterlockedExchange(reinterpret_cast<volatile long*>(&poison_),1);
#else
    __atomic_store_n(&poison_,1,__ATOMIC_RELEASE);
#endif
}
bool TargetLifetime::owner() const noexcept {
    return healthy() && thread_ && host_.thread &&
        host_.thread(host_.memory.context)==thread_;
}
bool TargetLifetime::start(u32 thread) noexcept {
    if(thread_ || !healthy() || !thread || !host_.memory.word || !host_.thread ||
       host_.thread(host_.memory.context)!=thread)return false;
    thread_=thread;return true;
}
bool TargetLifetime::event(u32 site,u32 target) noexcept {
    if(site!=0x0281AE26 && site!=0x02823A53)return false;
    if(!owner()){poison();return false;}
    if(busy_ || !target){poison();return false;}
    Busy lock(busy_);
    if(site==0x02823A53){
        for(auto& e:entries_)if(e.address==target)e={};
        return true;
    }
    // A second birth without death is ambiguous, not evidence of a new life.
    for(auto& e:entries_)if(e.address==target){e={};poison();return false;}
    u32 vt=0;
    if(target>rev_hud::Invalid-3 ||
       !host_.memory.word(host_.memory.context,target,&vt) || vt!=0x04DA8570 ||
       !healthy())return false;
    if(next_==rev_hud::Invalid){poison();return false;}
    for(auto& e:entries_)if(!e.valid()){
        e={target,++next_};return true;
    }
    // The untracked object stays unavailable; existing identities remain valid.
    return false;
}
TargetKey TargetLifetime::capture(u32 target) noexcept {
    if(!owner() || busy_ || !target)return {};
    for(const auto& e:entries_)if(e.address==target)return e;
    return {};
}
bool TargetLifetime::live(TargetKey key) noexcept {
    if(!key.valid())return false;
    const auto now=capture(key.address);return now.valid() && now.generation==key.generation;
}
bool bind_targets(TargetLifetime& targets) noexcept {
    if(Sink)return false;
    Sink=&targets;return true;
}
}
extern "C" void rev_genesis_target_event(unsigned int site,unsigned int target) noexcept {
    if(rev_genesis::Sink)(void)rev_genesis::Sink->event(site,target);
}
