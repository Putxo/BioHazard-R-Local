#include "../../patches/runtime/draw_schedule.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_action;
static unsigned int checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Fake {
    Frame frame{1,{true,1,{0x1000,1,0,1},{0x2000,2,1,1}}};
    SnapshotMode mode=SnapshotMode::Local;
    u32 vt=0x04F79014,view=0,reads=0;bool readable=true;
    Host host(){Host h{};h.memory={this,[](void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fake*>(p);++f.reads;
        if(!f.readable)return false;
        if(a==0x3000)*v=f.vt;
        else if(a==0x3158)*v=f.view;
        else return false;
        return true;
    }};h.snapshot=[](void* p,Frame* f) noexcept {
        auto& x=*static_cast<Fake*>(p);*f=x.frame;return x.mode;
    };return h;}
    u32 choose(u32 stock=0,u32 context=0x3000){return rev_runtime::choose_draw_schedule(host(),context,stock);}
};
int main(){
    {Fake f;C(f.choose()==1);f.view=1;C(f.choose()==1);f.view=0x12340001;C(f.choose()==1);
     for(u32 view=2;view<256;++view){f.view=view;C(f.choose()==0);}}
    {Fake f;f.mode=SnapshotMode::Stock;C(f.choose()==0);C(f.reads==0);}
    {Fake f;f.mode=SnapshotMode::Unavailable;C(f.choose()==0);C(f.reads==0);}
    {Fake f;f.readable=false;C(f.choose()==0);}
    {Fake f;f.vt=0xBAD;C(f.choose()==0);}
    {Fake f;C(f.choose(0,0)==0);C(f.choose(0,Invalid)==0);C(f.reads==0);}
    for(u32 value=1;value<256;++value){Fake f;C(f.choose(0xABCD0000|value)==value);C(f.reads==0);}
    {Fake f;C(f.choose(0xABCD0000)==1);}
    for(u32 fault=0;fault<9;++fault){Fake f;
        if(fault==0)f.frame.number=0;
        if(fault==1)f.frame.session.active=false;
        if(fault==2)f.frame.session.epoch=0;
        if(fault==3)f.frame.session.self.address=0;
        if(fault==4)f.frame.session.sub0.address=f.frame.session.self.address;
        if(fault==5)f.frame.session.sub0.lifetime=0;
        if(fault==6)f.frame.session.sub0.serial=-1;
        if(fault==7)f.frame.session.sub0.serial=0;
        if(fault==8)f.frame.session.sub0.think_mode=2;
        C(f.choose()==0);C(f.reads==0);
    }
    C(rev_runtime::choose_draw_schedule({},0x3000,0xABC00001)==1);
    C(rev_runtime::choose_draw_schedule({},0x3000,0)==0);
    std::printf("Local draw scheduling: %u checks passed\n",checks);
}
