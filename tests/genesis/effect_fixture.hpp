#pragma once
#include "../../patches/genesis/effects.hpp"
#include <map>
namespace effect_fixture {
using rev_hud::u32;
constexpr u32 Offsets[]={0x2F0,0x33C,0x338};
constexpr u32 Birth[]={0x0293344E,0x0293559E,0x037DD309};
constexpr u32 Death[]={0x02933576,0x02935816,0x037DD4A2};
constexpr u32 VT[]={0x04DB80CC,0x04DB833C,0x04F9A7F4};
inline void populate(std::map<u32,u32>& m,u32 scanner){
    for(u32 i=0;i<3;++i){const u32 p=scanner+0xE000+i*0x200;
        m[scanner+Offsets[i]]=p;m[p]=VT[i];m[p+12]=0x80032032;}
    m[scanner+0x2F4]=0x04DE4564;m[scanner+0x2F8]=scanner;
    const u32 root=m[scanner+0x2F0],r=scanner+0xE800;
    m[root+0x40]=r;m[root+0x44]=scanner+0x2F4;
    m[r]=0x04DB7A34;m[r+0x68]=0x04CC7278;m[r+0x6C]=m[r+0x70]=2;m[r+0x74]=1;m[r+0x78]=r+0x100;
    for(u32 i=0;i<2;++i){const u32 p=r+0x200+i*0x100;m[r+0x100+i*4]=p;m[p]=0x04FC0000+i*4;}
}
inline bool observe(std::map<u32,u32>& m,rev_genesis::EffectLifetime& life,u32 scanner){
    for(u32 i=0;i<3;++i)if(!life.event(Birth[i],m.at(scanner+Offsets[i])))return false;
    return true;
}
}
