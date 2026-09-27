#include "effect_lifetime.hpp"
#if defined(_MSC_VER)
#include <intrin.h>
#endif
namespace rev_genesis {
namespace {
EffectLifetime* Sink=nullptr;
struct Boundary {u32 birth,death,vtable,size;};
constexpr Boundary Types[]={
    {0x0293344E,0x02933576,0x04DB80CC,0x48},
    {0x0293559E,0x02935816,0x04DB833C,0xD0},
    {0x037DD309,0x037DD4A2,0x04F9A7F4,0x150},
};
struct Busy {bool& value;explicit Busy(bool& v):value(v){value=true;}~Busy(){value=false;}};
}
bool EffectLifetime::healthy() const noexcept {
#if defined(_MSC_VER)
    static_assert(sizeof(long)==sizeof(int),"Win32 atomic width");
    return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&poison_),0,0)==0;
#else
    return __atomic_load_n(&poison_,__ATOMIC_ACQUIRE)==0;
#endif
}
void EffectLifetime::poison() noexcept {
#if defined(_MSC_VER)
    (void)_InterlockedExchange(reinterpret_cast<volatile long*>(&poison_),1);
#else
    __atomic_store_n(&poison_,1,__ATOMIC_RELEASE);
#endif
}
bool EffectLifetime::owner() const noexcept {
    return healthy() && thread_ && host_.thread && host_.thread(host_.memory.context)==thread_;
}
bool EffectLifetime::start(u32 thread) noexcept {
    if(thread_ || !healthy() || !thread || !host_.memory.word || !host_.thread ||
       host_.thread(host_.memory.context)!=thread)return false;
    thread_=thread;return true;
}
bool EffectLifetime::event(u32 site,u32 unit) noexcept {
    u32 type=3;
    for(u32 i=0;i<3;++i)if(site==Types[i].birth || site==Types[i].death)type=i;
    if(type==3)return false;
    if(!owner() || busy_ || !unit){poison();return false;}
    Busy lock(busy_);
    const auto& t=Types[type];const auto kind=static_cast<EffectKind>(type);
    if(site==t.death){
        // Never inspect a dying allocation, including unobserved stock objects.
        for(auto& e:entries_)if(e.address==unit){
            const bool matched=e.kind==kind;e={};
            if(!matched){poison();return false;}
        }
        return true;
    }
    for(auto& e:entries_)if(e.address==unit){e={};poison();return false;}
    u32 vt=0;
    if(unit>rev_hud::Invalid-t.size ||
       !host_.memory.word(host_.memory.context,unit,&vt) || vt!=t.vtable || !healthy())return false;
    if(next_==rev_hud::Invalid){poison();return false;}
    for(auto& e:entries_)if(!e.valid()){
        e={unit,++next_,kind};return true;
    }
    // A full table makes new objects unadmitted; it never evicts a live key.
    return false;
}
EffectKey EffectLifetime::capture(u32 unit,EffectKind kind) noexcept {
    if(!owner() || busy_ || !unit || static_cast<u32>(kind)>=3)return {};
    for(const auto& e:entries_)if(e.address==unit && e.kind==kind)return e;
    return {};
}
bool EffectLifetime::live(EffectKey key) noexcept {
    if(!key.valid() || !owner() || busy_)return false;
    for(const auto& e:entries_)
        if(e.address==key.address && e.generation==key.generation && e.kind==key.kind)return healthy();
    return false;
}
bool bind_effects(EffectLifetime& effects) noexcept {
    if(Sink)return false;
    Sink=&effects;return true;
}
}
extern "C" void rev_genesis_effect_event(unsigned int site,unsigned int unit) noexcept {
    if(rev_genesis::Sink)(void)rev_genesis::Sink->event(site,unit);
}
