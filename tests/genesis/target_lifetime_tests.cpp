#include "../../patches/genesis/target_lifetime.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
using namespace rev_genesis;
int checks=0;
#define C(v) do {++checks;if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::exit(1);}}while(0)
constexpr u32 Born=0x0281AE26,Dying=0x02823A53;
struct Fixture {
    std::map<u32,u32> memory;
    u32 thread=7,reads=0;bool reenter=false;
    TargetLifetime life{{{this,[](void* p,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fixture*>(p);++f.reads;
        if(f.reenter){f.reenter=false;C(!f.life.event(Dying,a));}
        auto it=f.memory.find(a);if(it==f.memory.end())return false;*out=it->second;return true;
    }},[](void* p) noexcept {return static_cast<Fixture*>(p)->thread;}}};
    Fixture(){C(life.start(7));}
    bool born(u32 a){memory[a]=0x04DA8570;return life.event(Born,a);}
};
int main(){
    {Fixture f;C(!f.life.capture(0x1000).valid());C(f.born(0x1000));auto key=f.life.capture(0x1000);
     C(key.valid());C(f.life.live(key));const u32 reads=f.reads;
     f.memory.clear();C(f.life.event(Dying,0x1000));C(f.reads==reads);C(!f.life.live(key));
     C(f.life.event(Dying,0x1000));C(f.born(0x1000));auto next=f.life.capture(0x1000);
     C(next.valid());C(next.generation!=key.generation);C(!f.life.live(key));C(f.life.live(next));}
    {Fixture f;C(f.born(0x1000));auto key=f.life.capture(0x1000);
     f.thread=8;C(!f.life.capture(0x1000).valid());C(f.life.healthy());
     C(!f.life.event(Dying,0x1000));f.thread=7;C(!f.life.healthy());C(!f.life.live(key));C(!f.born(0x2000));}
    {Fixture f;C(f.born(0x1000));C(!f.born(0x1000));C(!f.life.healthy());C(!f.life.capture(0x1000).valid());}
    {Fixture f;f.reenter=true;C(!f.born(0x1000));C(!f.life.healthy());C(!f.life.capture(0x1000).valid());}
    {Fixture f;C(!f.life.start(7));C(!f.life.event(0x123456,0x1000));C(f.life.healthy());
     C(!f.life.event(Born,0x1000));f.memory[0x1000]=0x1234;C(!f.life.event(Born,0x1000));
     C(!f.life.capture(0x1000).valid());C(f.life.healthy());C(f.born(0x1000));}
    {Fixture f;C(!f.life.event(Born,0));C(!f.life.healthy());}
    {Fixture f;for(u32 i=0;i<TargetLifetime::Capacity;++i)C(f.born(0x1000+i*0x40));
     auto first=f.life.capture(0x1000);C(!f.born(0x100000));C(!f.life.capture(0x100000).valid());
     C(f.life.live(first));C(f.life.healthy());C(f.life.event(Dying,0x1000));
     C(f.born(0x100000));C(f.life.capture(0x100000).valid());C(!f.life.live(first));}
    {Fixture f;C(!f.life.event(Dying,0));C(!f.life.healthy());}
    std::printf("Genesis target lifetime: %d checks PASS\n",checks);
}
