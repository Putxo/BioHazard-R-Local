#include "weapon_lifetime.hpp"
#if defined(_MSC_VER)
#include <intrin.h>
#endif
namespace rev_genesis {
namespace {
WeaponLifetime* Sink=nullptr;
struct Busy {bool& value;explicit Busy(bool& v):value(v){value=true;}~Busy(){value=false;}};
}
bool WeaponLifetime::healthy() const noexcept {
#if defined(_MSC_VER)
    static_assert(sizeof(long)==sizeof(int),"Win32 atomic width");
    return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&poison_),0,0)==0;
#else
    return __atomic_load_n(&poison_,__ATOMIC_ACQUIRE)==0;
#endif
}
void WeaponLifetime::poison() noexcept {
#if defined(_MSC_VER)
    (void)_InterlockedExchange(reinterpret_cast<volatile long*>(&poison_),1);
#else
    __atomic_store_n(&poison_,1,__ATOMIC_RELEASE);
#endif
}
bool WeaponLifetime::owner() const noexcept {
    return healthy() && thread_ && host_.thread &&
        host_.thread(host_.memory.context)==thread_;
}
bool WeaponLifetime::start(u32 thread) noexcept {
    if(thread_ || !healthy() || !thread || !host_.memory.word || !host_.thread ||
       host_.thread(host_.memory.context)!=thread)return false;
    thread_=thread;return true;
}
bool WeaponLifetime::event(u32 site,u32 weapon) noexcept {
    if(site!=0x0248731E && site!=0x02487426)return false;
    if(!owner()){poison();return false;}
    if(busy_ || !weapon){poison();return false;}
    Busy lock(busy_);
    if(site==0x02487426){
        for(auto& e:entries_)if(e.address==weapon)e={};
        return true;
    }
    // A second birth without death is ambiguous, not evidence of a new life.
    for(auto& e:entries_)if(e.address==weapon){e={};poison();return false;}
    u32 vt=0;
    if(weapon>rev_hud::Invalid-0x12AF ||
       !host_.memory.word(host_.memory.context,weapon,&vt) || vt!=0x04D38EE4 ||
       !healthy())return false;
    if(next_==rev_hud::Invalid){poison();return false;}
    for(auto& e:entries_)if(!e.valid()){
        e={weapon,++next_};return true;
    }
    // The untracked object stays unavailable; existing identities remain valid.
    return false;
}
WeaponKey WeaponLifetime::capture(u32 weapon) noexcept {
    if(!owner() || busy_ || !weapon)return {};
    for(const auto& e:entries_)if(e.address==weapon)return e;
    return {};
}
bool WeaponLifetime::live(WeaponKey key) noexcept {
    return slot(key)!=rev_hud::Invalid;
}
u32 WeaponLifetime::slot(WeaponKey key) noexcept {
    if(!key.valid() || !owner() || busy_)return rev_hud::Invalid;
    for(u32 i=0;i<Capacity;++i)
        if(entries_[i].address==key.address && entries_[i].generation==key.generation)
            return healthy()?i:rev_hud::Invalid;
    return rev_hud::Invalid;
}
bool bind_weapons(WeaponLifetime& weapons) noexcept {
    if(Sink)return false;
    Sink=&weapons;return true;
}
}
extern "C" void rev_genesis_weapon_event(unsigned int site,unsigned int weapon) noexcept {
    if(rev_genesis::Sink)(void)rev_genesis::Sink->event(site,weapon);
}
