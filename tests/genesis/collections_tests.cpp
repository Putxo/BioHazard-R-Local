#include "resource_fixture.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_genesis;
unsigned checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Fixture {
    static constexpr u32 Unit=0x100000;
    std::map<u32,u32> memory;u32 reads=0,mutate_at=0;
    Fixture(u32 active=0){scanner_fixture::populate(memory,Unit,0x500000);scanner_fixture::collections(memory,Unit,active);}
    Reader reader(){return {this,[](void* p,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fixture*>(p);++f.reads;
        if(f.reads==f.mutate_at)f.memory[f.memory[Unit+0x308]+0x1C]=0xEE0000;
        auto i=f.memory.find(a);if(i==f.memory.end())return false;
        *out=i->second;return true;
    }};}
    bool capture(){Collections c{};return capture_collections(reader(),Unit,&c);}
};
int main(){
    for(u32 n=0;n<=21;++n){Fixture f(n);const auto before=f.memory;Collections c{};
        C(capture_collections(f.reader(),Fixture::Unit,&c));C(f.memory==before);
        C(c.active_targets==((1u<<n)-1));C(c.active_icons==c.active_targets);
        for(u32 i=0;i<21;++i)C(c.targets[i]==(i<n?0xEF0000+i*0x100:0));
    }
    const u32 u=Fixture::Unit;
    // Each certificate field is essential: zeroing any field of the populated
    // nonempty lists must fail unless it was zero or a legal GUI index already.
    Fixture base(3);const auto data=base.memory;
    for(const auto& entry:data){
        const u32 a=entry.first,v=entry.second;
        bool relevant=(a>=u+0x2B4 && a<u+0x338) ||
            (a>=u+0xC000 && a<u+0xC008+21*0x48) || (a>=u+0xD000 && a<u+0xD004+21*0x28);
        if(!relevant || !v)continue;
        if(a>=u+0xC008 && a<u+0xC008+21*0x48 && (a-u-0xC008)%0x48==0x40)continue;
        Fixture f(3);f.memory[a]=0;C(!f.capture());
    }
    for(u32 fault=0;fault<9;++fault){Fixture f(3);u32 p=f.memory[u+0x2B4],t=f.memory[u+0x308];
        switch(fault){
        case 0:f.memory[t+4+8]=t+4;break; // cycle
        case 1:f.memory[u+0x324+0xC]=p+0x24;break; // foreign pool
        case 2:f.memory[t+0x28+0x1C]=f.memory[t+0x1C];break;
        case 3:f.memory[p+0x48+0x3C]=t;break;
        case 4:f.memory[p+0x40]=5;break;
        case 5:f.memory[p+0x3C]=t+1;break;
        case 6:f.memory[p+0x3C]=t+20*0x28;break;
        case 7:f.memory[t+20*0x28+0x1C]=0xFFFF0000;break;
        case 8:f.memory[u+0x308]=0xFFFFFFFC;break;
        }C(!f.capture());
    }
    Fixture f(3);C(f.capture());const u32 total=f.reads;C(total%2==0);
    for(u32 boundary:{1u,total/2,total/2+1}){
        Fixture change(3);change.mutate_at=boundary;
        const bool ok=change.capture();C(boundary==1?ok:!ok);
    }
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"gameplay_executed\":false}\n",checks);
}
