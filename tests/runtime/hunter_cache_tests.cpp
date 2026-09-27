#include "../../patches/runtime/draw_schedule.hpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using namespace rev_action;
static unsigned int checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Fake {
    Frame frame{1,{true,1,{0x1000,1,0,1},{0x2000,2,1,1}}},after=frame;
    SnapshotMode mode=SnapshotMode::Local,mode_after=SnapshotMode::Local;
    u32 model=0x04D09FBC,context=0x04F79014,view=0,reads=0,snapshots=0,fail_read=0,change_read=0;
    Host host(){Host h{};h.memory={this,[](void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fake*>(p);++f.reads;
        if(f.reads==f.fail_read)return false;
        if(a==0x4000)*v=f.model;
        else if(a==0x3000)*v=f.context;
        else if(a==0x3158)*v=f.view;
        else return false;
        if(f.reads==f.change_read)*v^=1;
        return true;
    }};h.snapshot=[](void* p,Frame* out) noexcept {
        auto& f=*static_cast<Fake*>(p);++f.snapshots;
        *out=f.snapshots==1?f.frame:f.after;return f.snapshots==1?f.mode:f.mode_after;
    };return h;}
    u32 choose(u32 stock=0,u32 m=0x4000,u32 c=0x3000){return rev_runtime::hunter_uncached(host(),m,c,stock);}
};
int main(){
    for(u32 view=0;view<256;++view){Fake f;f.view=view;C(f.choose()==(view<2?1u:0u));}
    {Fake f;f.view=0x12340001;C(f.choose()==1);C(f.snapshots==2);C(f.reads==6);}
    for(u32 value=1;value<256;++value){Fake f;C(f.choose(0xABCD0000|value)==value);C(f.reads==0);C(f.snapshots==0);}
    {Fake f;C(f.choose(0xABCD0000)==1);}
    for(u32 which=0;which<2;++which)for(u32 value: {0u,Invalid-2,Invalid}){
        Fake f;C(f.choose(0,which?0x4000:value,which?value:0x3000)==0);C(f.reads==0);C(f.snapshots==0);
    }
    for(auto mode:{SnapshotMode::Stock,SnapshotMode::Unavailable}){
        Fake f;f.mode=mode;C(f.choose()==0);C(f.reads==0);
        Fake g;g.mode_after=mode;C(g.choose()==0);C(g.snapshots==2);
    }
    {Fake f;f.model=0x04D3AE5C;C(f.choose()==0);C(f.reads==1);}
    {Fake f;f.context=0xBAD;C(f.choose()==0);}
    for(u32 read=1;read<=6;++read){Fake f;f.fail_read=read;C(f.choose()==0);C(f.reads==read);}
    for(u32 read=4;read<=6;++read){Fake f;f.change_read=read;C(f.choose()==0);}
    for(u32 fault=0;fault<12;++fault){Fake f;
        if(fault==0)++f.after.number;
        if(fault==1)f.after.session.active=false;
        if(fault==2)++f.after.session.epoch;
        if(fault==3)++f.after.session.self.address;
        if(fault==4)++f.after.session.self.lifetime;
        if(fault==5)++f.after.session.self.serial;
        if(fault==6)++f.after.session.self.think_mode;
        if(fault==7)++f.after.session.sub0.address;
        if(fault==8)++f.after.session.sub0.lifetime;
        if(fault==9)++f.after.session.sub0.serial;
        if(fault==10)++f.after.session.sub0.think_mode;
        if(fault==11)f.after={};
        C(f.choose()==0);
    }
    for(u32 fault=0;fault<12;++fault){Fake f;
        if(fault==0)f.frame.number=0;
        if(fault==1)f.frame.session.active=false;
        if(fault==2)f.frame.session.epoch=0;
        if(fault==3)f.frame.session.self.address=0;
        if(fault==4)f.frame.session.sub0.address=0;
        if(fault==5)f.frame.session.sub0.address=f.frame.session.self.address;
        if(fault==6)f.frame.session.self.lifetime=0;
        if(fault==7)f.frame.session.sub0.lifetime=0;
        if(fault==8)f.frame.session.self.serial=-1;
        if(fault==9)f.frame.session.sub0.serial=f.frame.session.self.serial;
        if(fault==10)f.frame.session.self.think_mode=0;
        if(fault==11)f.frame.session.sub0.think_mode=0;
        C(f.choose()==0);C(f.reads==0);
    }
    C(rev_runtime::hunter_uncached({},0x4000,0x3000,0)==0);
    {Fake f;auto h=f.host();h.snapshot=nullptr;C(rev_runtime::hunter_uncached(h,0x4000,0x3000,0)==0);}
    {Fake f;auto h=f.host();h.memory.word=nullptr;C(rev_runtime::hunter_uncached(h,0x4000,0x3000,0)==0);}
    std::printf("Hunter uncached local drawing: %u checks passed\n",checks);
}
