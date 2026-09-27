#include "../../patches/action_icons/router.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
using namespace rev_action;
static int checks=0,scenarios=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
constexpr u32 I=0x1000,Q=0x2000,M=0x3000,R=0x4000;
struct Fake {
    std::map<u32,u32> mem{{I,0x04CDCA9C},{I+0x40,Q},{Q,0x04CDA850},
        {Q+0x34,0},{Q+0x38,0},{Q+0x3C,0},{M,0x04CDBDB4},
        {0x0556279C,M},{M+0x174,0xAABBCC01},{R+0x158,0}};
    Frame frame{1,{true,1,{0x10000,1,0,1},{0x20000,2,1,1}}};
    SnapshotMode mode=SnapshotMode::Local;
    u32 member=0,member_calls=0,draws=0,writes=0,fail_write=0,fail_read=0;
    bool mutate_member=false,mutate_frame=false,replace_manager=false,nested=false;
    bool priority_ready=true;
    u32 seen_priority=0;
    std::vector<u32> seen;
    Router router;
    Host host(bool priorities){Host h{{this,[](void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fake*>(p);auto i=f.mem.find(a);
        if(a==f.fail_read || i==f.mem.end())return false;
        *v=i->second;return true;
    }},[](void* p,u32 a,u32 v) noexcept {
        auto& f=*static_cast<Fake*>(p);if(++f.writes==f.fail_write)return false;
        f.mem[a]=v;return true;
    },[](void* p,Frame* v) noexcept {
        auto& f=*static_cast<Fake*>(p);*v=f.frame;return f.mode;
    },{this,[](void* p,u32 q) noexcept {
        auto& f=*static_cast<Fake*>(p);C(q==Q);++f.member_calls;
        if(f.mutate_member)f.mem[I+0x40]=Q+4;
        if(f.mutate_frame)++f.frame.number;
        return f.member;
    },[](void* p,u32 i,u32 r) noexcept {
        auto& f=*static_cast<Fake*>(p);C(i==I);C(r==R);++f.draws;
        if(f.mode==SnapshotMode::Stock)return;
        f.seen_priority=f.router.draw_priority(M,77);
        C(f.router.draw_priority(M+4,77)==77);
        f.seen.push_back(f.mem[M+0x174]&255);
        // The native draw claims the slot and may alter adjacent flag bytes.
        f.mem[M+0x174]=0x11223301;
        if(f.replace_manager)f.mem[0x0556279C]=M+0x1000;
        if(f.nested)C(f.router.draw(I,R)==Result::Refused);
    }}};
        if(priorities)h.priority=[](void* p,const Frame& frame,u32 manager,u32 member,u32* out) noexcept {
            auto& f=*static_cast<Fake*>(p);C(manager==M);C(frame.number==f.frame.number);C(member<=1);
            if(!f.priority_ready)return false;
            *out=member?0x202:0x208;return true;
        };
        return h;
    }
    explicit Fake(bool priorities=false):router(host(priorities)){}
    Result draw(){return router.draw(I,R);}
};
int main(){
    {Fake f(true);C(f.draw()==Result::Drawn);C(f.seen_priority==0x208);
     C(f.router.draw_priority(M,77)==77);f.member=1;f.mem[R+0x158]=1;
     C(f.draw()==Result::Drawn);C(f.seen_priority==0x202);++scenarios;}
    {Fake f(true);f.priority_ready=false;C(f.draw()==Result::Refused);C(!f.writes&&!f.draws);++scenarios;}
    {Fake f;f.member=1;auto before=f.mem;C(f.router.mask(I,1)==2);C(f.mem==before);C(!f.writes&&!f.draws);++scenarios;}
    {Fake f;C(f.router.mask(I,2)==1);C(f.router.mask(I,0x103)==0x101);++scenarios;}
    {Fake f;f.member=1;C(f.router.mask(I,0)==0);C(f.router.mask(I,0x100)==0x100);C(!f.member_calls);++scenarios;}
    {Fake f;f.mode=SnapshotMode::Stock;f.member=1;C(f.router.mask(I,1)==1);C(!f.member_calls);++scenarios;}
    {Fake f;f.mode=SnapshotMode::Unavailable;C(f.router.mask(I,1)==0);C(!f.member_calls);++scenarios;}
    {Fake f;f.frame.session.sub0.think_mode=2;C(f.router.mask(I,1)==0);C(!f.member_calls);++scenarios;}
    {Fake f;f.mem[I]=0x04DE28CC;C(f.router.mask(I,0x101)==0x101);C(!f.member_calls);++scenarios;}
    {Fake f;f.member=2;C(f.router.mask(I,1)==0);C(!f.writes);++scenarios;}
    {Fake f;f.mutate_member=true;C(f.router.mask(I,1)==0);C(!f.writes);++scenarios;}
    {Fake f;f.mutate_frame=true;C(f.router.mask(I,1)==0);C(!f.writes);++scenarios;}
    {Fake f;f.mem.erase(Q+0x38);C(f.router.mask(I,1)==0);C(!f.member_calls);++scenarios;}
    {Fake f;f.fail_write=2;C(f.draw()==Result::RestoreFailed);C(f.router.mask(I,1)==0);++scenarios;}
    {Fake f;f.mode=SnapshotMode::Stock;f.mem.clear();C(f.draw()==Result::Stock);C(f.draws==1);C(!f.member_calls&&!f.writes);++scenarios;}
    {Fake f;C(f.draw()==Result::Drawn);C(f.seen.back()==0);C(f.mem[M+0x174]==0x11223301);
     f.member=1;f.mem[R+0x158]=1;C(f.draw()==Result::Drawn);C(f.seen.back()==0);
     f.member=0;f.mem[R+0x158]=0;C(f.draw()==Result::Drawn);C(f.seen.back()==1);
     ++f.frame.number;C(f.draw()==Result::Drawn);C(f.seen.back()==0);++scenarios;}
    {Fake f;f.mem[M+0x174]=0xAABBCC00;C(f.draw()==Result::Drawn);C(f.mem[M+0x174]==0x11223300);++scenarios;}
    for(u32 view:{0u,1u}){Fake f;f.member=1-view;f.mem[R+0x158]=view;auto before=f.mem;
        C(f.draw()==Result::Skipped);C(!f.draws&&!f.writes);C(f.mem==before);++scenarios;}
    {Fake f;f.member=2;C(f.draw()==Result::Skipped);C(!f.draws&&!f.writes);++scenarios;}
    {Fake f;f.mem[R+0x158]=2;C(f.draw()==Result::Skipped);C(!f.member_calls&&!f.draws);++scenarios;}
    {Fake f;f.mem[R+0x158]=0xFF000001;f.member=1;C(f.draw()==Result::Drawn);++scenarios;}
    {Fake f;f.mode=SnapshotMode::Unavailable;C(f.draw()==Result::Refused);C(!f.member_calls&&!f.writes);++scenarios;}
    for(u32 a:{I,I+0x40,Q,Q+0x34,Q+0x38,Q+0x3C,M,0x0556279Cu,R+0x158}){
        Fake f;f.fail_read=a;C(f.draw()==Result::Refused);C(!f.draws&&!f.member_calls&&!f.writes);++scenarios;}
    {Fake f;f.fail_read=M+0x174;C(f.draw()==Result::Refused);C(!f.draws&&!f.writes);++scenarios;}
    for(u32 a:{I,Q,M}){Fake f;f.mem[a]=0xBAD;C(f.draw()==Result::Refused);C(!f.draws&&!f.member_calls);++scenarios;}
    for(int fault=0;fault<7;++fault){Fake f;
        if(fault==0)f.frame.session.active=false;
        if(fault==1)f.frame.number=0;
        if(fault==2)f.frame.session.epoch=0;
        if(fault==3)f.frame.session.sub0.address=f.frame.session.self.address;
        if(fault==4)f.frame.session.sub0.serial=f.frame.session.self.serial;
        if(fault==5)f.frame.session.sub0.lifetime=0;
        if(fault==6)f.frame.session.sub0.think_mode=2;
        C(f.draw()==Result::Refused);C(!f.member_calls&&!f.writes);++scenarios;}
    {Fake f;f.mutate_member=true;C(f.draw()==Result::Refused);C(!f.draws&&!f.writes);++scenarios;}
    {Fake f;f.mutate_frame=true;C(f.draw()==Result::Refused);C(!f.draws&&!f.writes);++scenarios;}
    {Fake f;f.fail_write=1;C(f.draw()==Result::Refused);C(!f.draws);C(!f.router.faulted());++scenarios;}
    {Fake f;f.fail_write=2;C(f.draw()==Result::RestoreFailed);C(f.router.faulted());C(f.draw()==Result::Refused);C(f.draws==1);++scenarios;}
    {Fake f;f.replace_manager=true;C(f.draw()==Result::RestoreFailed);C(f.writes==1);C(f.draw()==Result::Refused);++scenarios;}
    {Fake f;f.nested=true;C(f.draw()==Result::Drawn);C(f.draws==1);++scenarios;}
    {Fake f;C(f.router.draw(Invalid,R)==Result::Refused);C(f.router.draw(I,Invalid)==Result::Refused);C(!f.draws&&!f.writes);++scenarios;}
    {Fake f;C(f.draw()==Result::Drawn);++f.frame.session.sub0.lifetime;C(f.draw()==Result::Drawn);C(f.seen.back()==0);++scenarios;}
    {Fake f;C(f.draw()==Result::Drawn);f.mode=SnapshotMode::Stock;C(f.draw()==Result::Stock);
     f.mode=SnapshotMode::Local;++f.frame.session.epoch;C(f.draw()==Result::Drawn);C(f.seen.back()==0);++scenarios;}
    {Router r({});C(!r.ready());C(r.draw(I,R)==Result::Refused);++scenarios;}
    std::printf("Action icons: %d scenarios, %d checks passed\n",scenarios,checks);
}
