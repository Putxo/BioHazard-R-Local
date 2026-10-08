#pragma once
#include <map>
namespace script_fixture {
using u32=unsigned int;
constexpr u32 Manager=0x1000000,Main=0x1100000,Subs=0x1200000;
constexpr u32 Scheduler=0x1600000,Input=0x1610000;
inline u32 entry(u32 group,u32 slot){return Subs+group*0x20000+slot*0x1080;}
inline void populate(std::map<u32,u32>& m,u32 actor,u32 serial,u32 group=15,u32 slot=16){
    m[Input]=0x04E15EF4;m[Input+0x30]=Scheduler;
    // Group is deliberately different from the desired member1.
    m[Input+0x34]=group;m[0x05566570]=Manager;
    m[Manager]=0x04E13248;m[Manager+0x23C]=Main;
    for(u32 g=0;g<16;++g){
        m[Main+g*0x1070]=0x04DCBFAC;m[Main+g*0x1070+0xF90]=0;
        m[Manager+0x240+g*4]=entry(g,0);
        for(u32 s=0;s<17;++s){const u32 p=entry(g,s);
            m[p]=0x04DCF41C;m[p+0xF90]=0;m[p+0x1074]=serial;m[p+0x1078]=actor;
        }
    }
    m[entry(group,slot)+0xF90]=Scheduler;
}
}
