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
    u32 writes=0,fail_write=0,death_write=0,noop_write=0;
    static bool write(void* p,u32 a,u32 value) noexcept {
        auto& f=*static_cast<Fake*>(p);++f.writes;
        if(f.writes==f.fail_write || !f.m.count(a))return false;
        if(f.writes!=f.noop_write)f.m[a]=value;
        if(f.writes==f.death_write)C(f.life.event(effect_fixture::Death[0],Local+0xE000));
        return true;
    }
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
    {Fake g;Effects e{};C(capture_effects(g.reader(),g.life,Local,&e));const auto before=g.m;
        C(retire_effects(g.reader(),g.life,e,Fake::write));C(g.writes==7);
        C(g.m[e.units[0].address+0x44]==0);
        for(u32 i=0;i<3;++i){const u32 p=e.units[i].address;C(g.m[Local+effect_fixture::Offsets[i]]==0);
            C(g.m[p+12]==((before.at(p+12)&~0xFC07u)|3));C(g.life.live(e.units[i]));
            C(g.m[p]==before.at(p));}
        for(const auto& kv:before)if(kv.first<Local)C(g.m.at(kv.first)==kv.second);
        C(!retire_effects(g.reader(),g.life,e,Fake::write));C(g.writes==7);
        for(u32 i=0;i<3;++i){C(g.life.event(effect_fixture::Death[i],e.units[i].address));C(!g.life.live(e.units[i]));}}
    for(u32 failure=1;failure<=7;++failure){Fake g;Effects e{};C(capture_effects(g.reader(),g.life,Local,&e));
        g.fail_write=failure;C(!retire_effects(g.reader(),g.life,e,Fake::write));C(g.writes==failure);
        C(g.m[e.units[0].address+0x44]==(failure==1?Local+0x2F4:0));
        if(failure<=4)for(u32 off:effect_fixture::Offsets)C(g.m[Local+off]!=0);
    }
    {Fake g;Effects e{};C(capture_effects(g.reader(),g.life,Local,&e));g.noop_write=7;
        C(!retire_effects(g.reader(),g.life,e,Fake::write));C(g.m[Local+0x338]!=0);}
    {Fake g;Effects e{};C(capture_effects(g.reader(),g.life,Local,&e));g.death_write=1;
        C(!retire_effects(g.reader(),g.life,e,Fake::write));C(g.writes==1);}
    {Fake g;Effects e{};C(capture_effects(g.reader(),g.life,Local,&e));
        C(g.life.event(effect_fixture::Death[0],e.units[0].address));C(g.life.event(effect_fixture::Birth[0],e.units[0].address));
        C(!retire_effects(g.reader(),g.life,e,Fake::write));C(!g.writes);}
    {Fake g;Effects e{};C(capture_effects(g.reader(),g.life,Local,&e));
        C(!retire_effects(g.reader(),g.life,e,nullptr));C(!g.writes);}
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
    {Fake g;g.death_read=Local+0xE000;g.death_unit=Local+0xE000;g.forbidden=Local+0xE00C;
        C(!capture_effects(g.reader(),g.life,Local,&out));C(!g.forbidden_reads);}
    {Fake g;g.thread=8;g.forbidden=Local+0xE000;
        C(!capture_effects(g.reader(),g.life,Local,&out));C(!g.forbidden_reads);}
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"gameplay_executed\":false}\n",checks);
}
