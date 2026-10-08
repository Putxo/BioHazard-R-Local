#include "../../patches/script_input/script_input.hpp"
#include "fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace script_fixture;
static unsigned checks=0,scenarios=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Fixture {
    std::map<u32,u32> m;std::vector<u32> trace;
    rev_script::Owner owner{0x320000,7,1};
    bool thread=true,live=true;u32 captures=0,fail=0,trigger=0,mutate=0,value=0,nested=99;
    bool reenter=false,revoke=false;u32 capture_fault=0;
    rev_script::Host host(){return {this,[](void* p,u32 a,u32* v) noexcept {
        auto& z=*static_cast<Fixture*>(p);z.trace.push_back(a);
        if(a==z.trigger){z.trigger=0;
            if(z.reenter)z.nested=z.router.member(Input);
            if(z.revoke)z.live=false;
            if(z.mutate)z.m[z.mutate]=z.value;
        }
        const auto it=z.m.find(a);if(a==z.fail || it==z.m.end())return false;
        *v=it->second;return true;
    },[](void* p) noexcept {return static_cast<Fixture*>(p)->thread;},
    [](void* p,rev_script::Owner* out) noexcept {
        auto& z=*static_cast<Fixture*>(p);++z.captures;
        if(z.captures==2){
            if(z.capture_fault==1)++z.owner.actor;
            if(z.capture_fault==2)++z.owner.serial;
            if(z.capture_fault==3)++z.owner.epoch;
        }
        *out=z.owner;return z.live;
    }};}
    rev_script::InputRouter router;
    Fixture():router(host()){populate(m,owner.actor,owner.serial);}
};
int main(){
    C(rev_script_input_member(Input)==0);
    for(u32 g=0;g<16;++g)for(u32 s=0;s<17;++s){Fixture f;
        populate(f.m,f.owner.actor,f.owner.serial,g,s);const auto before=f.m;
        C(f.router.member(Input)==1);C(f.m==before);C(f.captures==2);++scenarios;
    }
    for(u32 g=0;g<16;++g){Fixture f;f.m[Main+g*0x1070+0xF90]=Scheduler;
        C(f.router.member(Input)==0);++scenarios;}
    for(u32 g=0;g<16;++g)for(u32 s=0;s<17;++s){if(g==15 && s==16)continue;
        Fixture f;f.m[entry(g,s)+0xF90]=Scheduler;C(f.router.member(Input)==0);++scenarios;}
    // Every read in a successful resolution is required, including the last group.
    Fixture reference;C(reference.router.member(Input)==1);
    for(u32 a:reference.trace){Fixture f;f.fail=a;C(f.router.member(Input)==0);++scenarios;}
    const u32 selected=entry(15,16);
    for(auto pair:std::vector<std::pair<u32,u32>>{
        {Input,0},{Input+0x30,0},{0x05566570,0},{0x05566570,0xFFFFFF00},
        {Manager,0},{Manager+0x23C,0},{Manager+0x23C,0xFFFF0000},
        {Manager+0x240,0},{Manager+0x240,0xFFFF0000},
        {Main,0},{entry(0,0),0},{selected+0xF90,0},
        {selected+0x1078,0x300000},{selected+0x1074,8}}){
        Fixture f;f.m[pair.first]=pair.second;C(f.router.member(Input)==0);++scenarios;
    }
    // Changes after initial anchor reads must be rejected on the same call.
    for(u32 a:{Input,Input+0x30,0x05566570u,Manager,Manager+0x23C,
               Manager+0x240,Manager+0x27C,selected,selected+0xF90,selected+0x1074,selected+0x1078}){
        Fixture f;f.trigger=selected+0x1074;f.mutate=a;f.value=0;
        C(f.router.member(Input)==0);++scenarios;
    }
    for(u32 fault=1;fault<=3;++fault){Fixture f;f.capture_fault=fault;C(f.router.member(Input)==0);++scenarios;}
    for(u32 input:{0u,0xFFFFFFF0u,0x1234u}){Fixture f;C(f.router.member(input)==0);++scenarios;}
    {Fixture f;f.thread=false;C(f.router.member(Input)==0);C(f.trace.empty() && !f.captures);++scenarios;}
    {Fixture f;f.live=false;C(f.router.member(Input)==0);C(f.trace.empty());++scenarios;}
    for(u32 field=0;field<3;++field){Fixture f;if(field==0)f.owner.actor=0;
        if(field==1)f.owner.serial=0xFFFFFFFFu;
        if(field==2)f.owner.epoch=0;
        C(f.router.member(Input)==0);C(f.trace.empty());++scenarios;}
    {Fixture f;f.trigger=Input;f.reenter=true;C(f.router.member(Input)==1);C(f.nested==0);
        C(f.router.member(Input)==1);++scenarios;}
    {Fixture f;f.trigger=selected;f.revoke=true;C(f.router.member(Input)==0);
        f.live=true;C(f.router.member(Input)==1);++scenarios;}
    {Fixture f;C(f.router.member(Input)==1);f.m[selected+0x1078]=0x300000;
        C(f.router.member(Input)==0);f.m[selected+0x1078]=f.owner.actor;
        f.m[Input+0x30]=Scheduler+4;C(f.router.member(Input)==0);
        f.m[selected+0xF90]=Scheduler+4;C(f.router.member(Input)==1);++scenarios;}
    {rev_script::InputRouter absent({});C(absent.member(Input)==0);++scenarios;}
    {Fixture f;C(rev_script::bind(f.router));C(!rev_script::bind(f.router));
        C(rev_script_input_member(Input)==1);f.live=false;C(rev_script_input_member(Input)==0);++scenarios;}
    std::printf("PASS script input: %u scenarios, %u assertions; synthetic memory only\n",scenarios,checks);
}
