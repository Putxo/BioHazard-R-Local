#include "effect_fixture.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_genesis;
unsigned checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
constexpr u32 Stock=0x100000,Local=0x200000;
struct Fake {
    std::map<u32,u32> m;
    u32 thread=7,fail=0,mutate=0,death_read=0,death_unit=0,forbidden=0,forbidden_reads=0;
    Reader reader(){return {this,[](void* p,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fake*>(p);if(a==f.forbidden)++f.forbidden_reads;
        auto it=f.m.find(a);if(a==f.fail || it==f.m.end())return false;
        *out=it->second;
        if(a==f.mutate){++it->second;f.mutate=0;}
        if(a==f.death_read){f.death_read=0;C(f.life.event(effect_fixture::Death[0],f.death_unit));}
        return true;
    }};}
    EffectLifetime life;
    Fake():life({reader(),[](void* p) noexcept ->u32 {return static_cast<Fake*>(p)->thread;}}){
        C(life.start(7));
        for(u32 s:{Stock,Local}){m[s]=0x04DE3D6C;effect_fixture::populate(m,s);C(effect_fixture::observe(m,life,s));}
    }
};
int main(){
    Fake f;Effects a{},b{},out{};C(capture_effects(f.reader(),f.life,Stock,&a));
    C(capture_effects(f.reader(),f.life,Local,&b));C(disjoint_effects(a,b));C(disjoint_effects(b,a));
    const Effects empty{};
    C(!disjoint_effects(a,a));C(!disjoint_effects(a,empty));C(!same_effects(a,b));
    C(capture_effects(f.reader(),f.life,Stock,&out));C(same_effects(a,out));
    C(!capture_effects(f.reader(),f.life,0,&out));C(!capture_effects(f.reader(),f.life,Stock,nullptr));
    C(!effect_view(f.reader(),b,1));C(!effect_view(f.reader(),b,2));
    for(const auto& k:b.units)f.m[k.address+12]=(f.m[k.address+12]&~0x03FF0000u)|0x20000u;
    C(effect_view(f.reader(),b,1));C(!effect_view(f.reader(),b,0));
    for(u32 i=0;i<3;++i){
        Fake g;const u32 p=g.m[Local+effect_fixture::Offsets[i]];out=b;
        C(g.life.event(effect_fixture::Death[i],p));g.forbidden=p;
        C(!capture_effects(g.reader(),g.life,Local,&out));C(!g.forbidden_reads);C(same_effects(out,b));
        C(g.life.event(effect_fixture::Birth[i],p));C(capture_effects(g.reader(),g.life,Local,&out));C(!same_effects(out,b));
        g.m[p+12]=(g.m[p+12]&~7u)|3;C(!capture_effects(g.reader(),g.life,Local,&out));
        g.m[p+12]=(g.m[p+12]&~7u)|1;C(capture_effects(g.reader(),g.life,Local,&out));
        g.m[p+12]&=~0x3F8u;C(!capture_effects(g.reader(),g.life,Local,&out));
    }
    for(u32 i=1;i<3;++i){Fake g;g.m[Local+effect_fixture::Offsets[i]]=g.m[Stock+effect_fixture::Offsets[i]];
        C(capture_effects(g.reader(),g.life,Local,&out));C(!disjoint_effects(a,out));}
    {Fake g;g.m[Local+0xE040]=g.m[Stock+0xE040];
        C(capture_effects(g.reader(),g.life,Local,&out));C(!disjoint_effects(a,out));}
    // A resource child overlapping a concrete root's interior is not private.
    {Fake g;g.m[Local+0xE900]=Local+0xE210;g.m[Local+0xE210]=0x04FC0000;
        C(!capture_effects(g.reader(),g.life,Local,&out));}
    {Fake g;g.m[Local+0xE044]=Stock+0x2F4;C(!capture_effects(g.reader(),g.life,Local,&out));}
    {Fake g;g.m[Local+0x2F8]=Stock;C(!capture_effects(g.reader(),g.life,Local,&out));}
    {Fake g;g.m[Local+0x2F4]=0;C(!capture_effects(g.reader(),g.life,Local,&out));}
    for(u32 off:{0u,0x2F0u,0x33Cu,0x338u,0xE000u,0xE20Cu,0xE044u,0x2F8u,0xE840u,0xE878u}){
        Fake g;g.fail=Local+off;out=b;
        // +E840 is not a graph field and deliberately does not cause a read.
        if(off==0xE840){C(capture_effects(g.reader(),g.life,Local,&out));continue;}
        C(!capture_effects(g.reader(),g.life,Local,&out));C(same_effects(out,b));
    }
    {Fake g;g.mutate=Local+0x338;C(!capture_effects(g.reader(),g.life,Local,&out));}
    {Fake g;g.death_read=Local+0xEA00;g.death_unit=Local+0xE000;
        C(!capture_effects(g.reader(),g.life,Local,&out));}
    {Fake g;g.thread=8;g.forbidden=Local+0xE000;
        C(!capture_effects(g.reader(),g.life,Local,&out));C(!g.forbidden_reads);}
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"gameplay_executed\":false}\n",checks);
}
