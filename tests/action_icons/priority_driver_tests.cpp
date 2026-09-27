#include "../../patches/action_icons/priority_driver.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_action;
static int checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
constexpr u32 S=0x10000,M=0x20000,Q=0x30000;
struct Fake {
    std::map<u32,u32> mem{{S-8,M},{M,0x04CDBDB4},{0x0556279C,M},
        {S-0x100,0},{S-0x10C,0},{S-0x128,0},{Q,0x04CDA850},
        {Q+0x34,0},{Q+0x38,0},{Q+0x3C,0}};
    Frame frame{1,{true,1,{0x1000,1,0,1},{0x2000,2,1,1}}};
    SnapshotMode mode=SnapshotMode::Local;
    bool thread=true,change_session=false;
    u32 member=0,writes=0,fail_write=0,member_calls=0;
    PriorityDriver driver;
    PriorityHost host(){
        Host h{{this,[](void* p,u32 a,u32* v) noexcept {
            auto& f=*static_cast<Fake*>(p);auto i=f.mem.find(a);
            if(i==f.mem.end())return false;
            *v=i->second;return true;
        }},[](void* p,u32 a,u32 v) noexcept {
            auto& f=*static_cast<Fake*>(p);if(++f.writes==f.fail_write)return false;
            f.mem[a]=v;return true;
        },[](void* p,Frame* v) noexcept {
            auto& f=*static_cast<Fake*>(p);*v=f.frame;return f.mode;
        },{this,[](void* p,u32 command) noexcept {
            C(command==Q);auto& f=*static_cast<Fake*>(p);++f.member_calls;
            if(f.change_session)++f.frame.session.epoch;
            return f.member;
        },nullptr}};
        return {h,[](void* p) noexcept {return static_cast<Fake*>(p)->thread;},
            [](void*,u32 raw) noexcept {return raw*10;}};
    }
    Fake():driver(host()){}
    void start(){driver.begin(S);}
    void prepare(u32 m){member=m;driver.prepare(S,Q);}
    void commit(u32 priority,u32 flags){mem[S-0x100]=priority;mem[S-0x10C]=flags;C(driver.commit(S));}
};
int main(){
    {Fake f;f.start();f.prepare(0);C(f.driver.decide(S,true)==Decision::Proceed);f.commit(8,3);
     C(f.mem[S-0x10C]==2);f.prepare(1);C(f.mem[S-0x100]==0&&f.mem[S-0x128]==0&&f.mem[S-0x10C]==2);
     C(f.driver.decide(S,true)==Decision::Proceed);f.commit(5,2);
     f.prepare(0);C(f.mem[S-0x128]==80);C(f.driver.decide(S,true)==Decision::Next);
     f.prepare(1);C(f.mem[S-0x128]==50);C(f.driver.decide(S,false)==Decision::Next);
     f.driver.finish(S);C(f.mem[S-0x100]==8);u32 p=99;
     C(f.driver.priority(f.frame,M,0,&p)&&p==8);C(f.driver.priority(f.frame,M,1,&p)&&p==5);
     ++f.frame.number;C(!f.driver.priority(f.frame,M,1,&p));}
    {Fake f;f.mode=SnapshotMode::Stock;auto before=f.mem;f.start();f.prepare(0);
     C(f.driver.decide(S,false)==Decision::End);C(f.driver.commit(S));f.driver.finish(S);
     C(f.mem==before&&f.writes==0&&f.member_calls==0);}
    {Fake f;f.thread=false;auto before=f.mem;f.start();f.prepare(0);
     C(f.driver.decide(S,true)==Decision::Proceed);C(f.driver.commit(S));f.driver.finish(S);
     C(f.mem==before&&f.writes==0&&f.member_calls==0);}
    for(u32 fail:{1u,2u,3u}){Fake f;f.mem[S-0x128]=123;auto before=f.mem;f.start();f.fail_write=fail;
     f.prepare(0);C(f.mem==before);C(f.driver.decide(S,true)==Decision::End);f.driver.finish(S);
     u32 p=99;C(!f.driver.priority(f.frame,M,0,&p));C(p==99);}
    {Fake f;f.start();f.change_session=true;f.prepare(0);C(f.writes==0);
     C(f.driver.decide(S,true)==Decision::End);f.driver.finish(S);u32 p;C(!f.driver.priority(f.frame,M,0,&p));}
    {Fake f;f.start();f.mem.erase(Q+0x38);f.prepare(0);C(!f.member_calls&&!f.writes);
     C(f.driver.decide(S,true)==Decision::End);}
    {Fake f;f.start();f.prepare(0);C(f.driver.decide(S,true)==Decision::Proceed);f.fail_write=4;
     f.mem[S-0x100]=3;f.mem[S-0x10C]=3;C(!f.driver.commit(S));f.driver.finish(S);
     u32 p;C(!f.driver.priority(f.frame,M,0,&p));}
    {Fake f;f.start();f.prepare(0);C(f.driver.decide(S,true)==Decision::Proceed);f.commit(3,3);
     f.fail_write=f.writes+1;f.driver.finish(S);u32 p;C(!f.driver.priority(f.frame,M,0,&p));}
    {Fake f;f.start();f.prepare(0);C(f.driver.decide(S,true)==Decision::Proceed);f.commit(3,3);
     f.mem[0x0556279C]=M+4;f.driver.finish(S);u32 p;C(!f.driver.priority(f.frame,M,0,&p));}
    std::printf("Priority native-stack adapter: %d checks passed\n",checks);
}
