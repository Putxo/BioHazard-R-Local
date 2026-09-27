#include "../../patches/genesis/effect_lifetime.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_genesis;
unsigned checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
constexpr u32 Birth[]={0x0293344E,0x0293559E,0x037DD309};
constexpr u32 Death[]={0x02933576,0x02935816,0x037DD4A2};
constexpr u32 VT[]={0x04DB80CC,0x04DB833C,0x04F9A7F4};
struct Fake {
    std::map<u32,u32> m;
    u32 thread=7,reads=0,fail=0;
    EffectLifetime* reenter=nullptr;
    EffectHost host(){return {{this,[](void* p,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fake*>(p);++f.reads;
        if(f.reenter){auto* r=f.reenter;f.reenter=nullptr;C(!r->event(Death[0],a));}
        const auto it=f.m.find(a);if(a==f.fail || it==f.m.end())return false;
        *out=it->second;return true;
    }},[](void* p) noexcept ->u32 {return static_cast<Fake*>(p)->thread;}};}
};
int main(){
    for(u32 i=0;i<3;++i){
        Fake f;EffectLifetime life(f.host());const auto kind=static_cast<EffectKind>(i);
        const u32 p=0x100000+i*0x1000;f.m[p]=VT[i];
        C(!life.capture(p,kind).valid());C(!life.start(0));C(!life.start(8));
        C(life.start(7));C(!life.start(7));C(!life.event(123,p));C(life.healthy());
        C(life.event(Birth[i],p));const auto old=life.capture(p,kind);C(old.valid());C(life.live(old));
        C(!life.capture(p,static_cast<EffectKind>((i+1)%3)).valid());
        C(!life.live({p,old.generation,static_cast<EffectKind>((i+1)%3)}));
        const u32 reads=f.reads;f.m.clear();
        C(life.event(Death[i],p));C(f.reads==reads);C(!life.live(old));
        C(life.event(Death[i],p));C(life.healthy());
        f.m[p]=VT[i];C(life.event(Birth[i],p));const auto fresh=life.capture(p,kind);
        C(fresh.valid() && fresh.generation>old.generation);C(!life.live(old));C(life.live(fresh));
        f.thread=8;C(!life.capture(p,kind).valid());C(!life.live(fresh));C(life.healthy());
        C(!life.event(Death[i],p));f.thread=7;C(!life.healthy());C(!life.live(fresh));
    }
    {Fake f;EffectLifetime life(f.host());C(life.start(7));
        for(u32 i=0;i<3;++i){f.m[0x100000+i*0x1000]=VT[i];C(life.event(Birth[i],0x100000+i*0x1000));}
        for(u32 i=0;i<3;++i)C(life.live(life.capture(0x100000+i*0x1000,static_cast<EffectKind>(i))));
        const auto a=life.capture(0x100000,EffectKind::FilterSet);
        C(!life.event(Death[1],a.address));C(!life.healthy());C(!life.live(a));}
    {Fake f;EffectLifetime life(f.host());C(life.start(7));f.m[0x100000]=VT[0];
        C(life.event(Birth[0],0x100000));const auto a=life.capture(0x100000,EffectKind::FilterSet);
        C(!life.event(Birth[0],0x100000));C(!life.healthy());C(!life.live(a));}
    {Fake f;EffectLifetime life(f.host());C(life.start(7));
        C(!life.event(Birth[0],0x100000));C(life.healthy());
        f.m[0x100000]=VT[1];C(!life.event(Birth[0],0x100000));C(life.healthy());
        f.m[0x100000]=VT[0];f.fail=0x100000;C(!life.event(Birth[0],0x100000));
        C(!life.capture(0x100000,EffectKind::FilterSet).valid());
        C(!life.event(Birth[0],~0u));C(!life.live({}));}
    {Fake f;EffectLifetime life(f.host());C(life.start(7));f.m[0x100000]=VT[0];f.reenter=&life;
        C(!life.event(Birth[0],0x100000));C(!life.healthy());}
    {Fake f;EffectLifetime life(f.host());C(!life.event(Birth[0],0x100000));C(!life.start(7));}
    {Fake f;EffectLifetime life(f.host());C(life.start(7));C(!life.event(Birth[0],0));C(!life.healthy());}
    {Fake f;EffectLifetime life(f.host());C(life.start(7));EffectKey keys[EffectLifetime::Capacity]{};
        for(u32 i=0;i<EffectLifetime::Capacity;++i){const u32 p=0x100000+i*0x1000;
            f.m[p]=VT[0];C(life.event(Birth[0],p));keys[i]=life.capture(p,EffectKind::FilterSet);C(keys[i].valid());}
        constexpr u32 extra=0x800000;f.m[extra]=VT[0];C(!life.event(Birth[0],extra));C(life.healthy());
        for(const auto& k:keys)C(life.live(k));
        C(!life.capture(extra,EffectKind::FilterSet).valid());
        C(life.event(Death[0],keys[4].address));C(life.event(Birth[0],extra));C(!life.live(keys[4]));
        C(life.live(life.capture(extra,EffectKind::FilterSet)));}
    {Fake f;EffectLifetime life(f.host());C(life.start(7));C(bind_effects(life));C(!bind_effects(life));
        f.m[0x100000]=VT[0];rev_genesis_effect_event(Birth[0],0x100000);
        C(life.capture(0x100000,EffectKind::FilterSet).valid());rev_genesis_effect_event(Death[0],0x100000);
        C(!life.capture(0x100000,EffectKind::FilterSet).valid());}
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"gameplay_executed\":false}\n",checks);
}
