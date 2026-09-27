#pragma once
#include "../../patches/genesis/resources.hpp"
#include <map>
namespace scanner_fixture {
using rev_hud::u32;
inline void collections(std::map<u32,u32>& m,u32 u,u32 active=0) {
    for(u32 kind=0;kind<2;++kind){
        const u32 pool=m[u+(kind?0x308:0x2B4)],stride=kind?0x28:0x48,node_offset=kind?4:0x24;
        for(u32 i=0;i<21;++i){
            const u32 object=pool+i*stride,node=object+node_offset;
            m[object]=kind?0x04DE45A0:0x04DE44B4;
            m[node]=kind?0x04DE45F4:0x04DE45D4;
            m[node+0xC]=kind?0x04DE45E4:0x04DE45C4;m[node+0x10]=object;
            if(kind)m[object+0x1C]=i<active?0xEF0000+i*0x100:0;
            else {m[object+0x3C]=i<active?m[u+0x308]+i*0x28:0;m[object+0x40]=i<active && i<5?i:~0u;}
        }
        for(u32 used=0;used<2;++used){
            const u32 header=u+(kind?0x310:0x2BC)+used*0x14,sentinel=header+4;
            m[header]=kind?0x04DE45B8:0x04DE45AC;m[sentinel]=0x04CBE468;m[header+0x10]=sentinel;
            u32 previous=sentinel;
            for(u32 i=used?0:active;i<(used?active:21);++i){
                const u32 node=pool+i*stride+node_offset;
                m[previous+8]=node;m[node+4]=previous;previous=node;
            }
            m[previous+8]=sentinel;m[sentinel+4]=previous;
        }
    }
}
inline void populate(std::map<u32,u32>& m,u32 u,u32 resource) {
    m[u]=0x04DE3D6C;
    m[u+0x2FC]=0; // Native constructor at 02B1EE13.
    m[u+0xf0]=resource;m[u+0xf4]=u+0x1000;m[u+0xf8]=u+0x2000;
    m[resource+0x68]=resource+0x100;m[resource+0x144]=15;m[u+0x106c]=u;
    for(u32 i=0;i<15;++i){m[u+0x2000+i*4]=u+0x3000+i*0x400;m[u+0x306c+i*0x400]=u;}
    const u32 fields[]={0x358,0x380,0x360,0x364,0x368,0x36c,0x370,0x374,0x384,0x388,0x38c,0x390,0x35c,0x37c,0x378};
    for(u32 i=0;i<15;++i)m[u+fields[i]]=m[u+0x2000+i*4];
    m[u+0x224]=14;m[u+0x230]=u+0xa000;
    const u32 af[]={0x394,0x3b8,0x398,0x39c,0x3a0,0x3bc,0x3c0,0x3c4,0x3c8,0x3a4,0x3a8,0x3ac,0x3b0,0x3b4};
    for(u32 i=0;i<14;++i){const u32 a=u+0xa100+i*0x40;m[u+0xa000+i*4]=m[u+af[i]]=a;m[a]=0x04fc0000;}
    const u32 indices[]={12,6,7,14},counts[]={11,2,3,4},cached_counts[]={7,1,1,2},text_indices[]={4,3,2,10,9,8,7,1,2,2,3};
    u32 t=0;
    for(u32 i=0;i<4;++i){
        const u32 p=m[u+0x2000+indices[i]*4],root=u+0x8000+i*0x800,rr=resource+0x1000+i*0x400;
        m[p+0xf0]=rr;m[p+0xf4]=root;m[p+0xf8]=root+0x100;
        m[rr+0x68]=rr+0x100;m[rr+0x144]=counts[i];m[root+0x6c]=p;
        m[p+0x224]=m[p+0x230]=0;
        for(u32 j=0;j<counts[i];++j){const u32 node=root+0x200+j*0x40;m[root+0x100+j*4]=node;m[node+0x6c]=p;}
        for(u32 j=0;j<cached_counts[i];++j,++t)m[u+0x3cc+t*4]=m[root+0x100+text_indices[t]*4];
    }
    m[u+0x2b4]=u+0xc008;m[u+0x2b8]=m[u+0xc000]=21;
    m[u+0x308]=u+0xd004;m[u+0x30c]=m[u+0xd000]=21;
    collections(m,u);
}
}
