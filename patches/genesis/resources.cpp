#include "resources.hpp"
namespace rev_genesis {
namespace {
constexpr u32 Limit=~0u;
bool range(u32 p,u32 n) noexcept {return p && n && p<=Limit-n;}
bool overlap(Region a,Region b) noexcept {
    return a.address<b.address+b.bytes && b.address<a.address+a.bytes;
}
bool word(Reader r,u32 p,u32 offset,u32& out) noexcept {
    return p && offset<=Limit-4 && p<=Limit-offset-4 && r.word &&
        r.word(r.context,p+offset,&out);
}
bool append(Resources& g,u32 p,u32 n=4) noexcept {
    if(!range(p,n) || g.count==Resources::Capacity)return false;
    Region v{p,n};
    for(u32 i=0;i<g.count;++i)if(overlap(g.regions[i],v))return false;
    g.regions[g.count++]=v;return true;
}
struct Gui {
    u32 unit=0,resource=0,root=0,table=0,count=0;
    u32 nodes[256]{};
};
bool gui(Reader r,u32 unit,Gui& g,Resources& out) noexcept {
    g.unit=unit;u32 header=0,owner=0;
    if(!word(r,unit,0xf0,g.resource) || !word(r,unit,0xf4,g.root) ||
       !word(r,unit,0xf8,g.table) || !word(r,g.resource,0x68,header) ||
       !word(r,header,0x44,g.count) || !g.count || g.count>256 ||
       !word(r,g.root,0x6c,owner) || owner!=unit ||
       !append(out,g.root) || !append(out,g.table,g.count*4))return false;
    for(u32 i=0;i<g.count;++i)
        if(!word(r,g.table,i*4,g.nodes[i]) || !word(r,g.nodes[i],0x6c,owner) ||
           owner!=unit || !append(out,g.nodes[i]))return false;
    u32 v=0;
    return word(r,unit,0xf0,v) && v==g.resource && word(r,unit,0xf4,v) && v==g.root &&
        word(r,unit,0xf8,v) && v==g.table && word(r,g.resource,0x68,v) && v==header &&
        word(r,header,0x44,v) && v==g.count;
}
bool animations(Reader r,u32 unit,Resources& out,u32* table_out=nullptr,u32* count_out=nullptr) noexcept {
    u32 count=0,table=0;
    if(!word(r,unit,0x224,count) || count>256 || !word(r,unit,0x230,table))return false;
    if(table && !count && !append(out,table))return false;
    if(count) {
        if(!append(out,table,count*4))return false;
        for(u32 i=0;i<count;++i){u32 p=0,vt=0;
            if(!word(r,table,i*4,p) || !word(r,p,0,vt) || !vt || !append(out,p))return false;
        }
    }
    u32 check=0;
    if(!word(r,unit,0x224,check) || check!=count || !word(r,unit,0x230,check) || check!=table)return false;
    if(table_out)*table_out=table;
    if(count_out)*count_out=count;
    return true;
}
bool cached(Reader r,u32 unit,u32 offset,u32 expected) noexcept {
    u32 v=0;return expected && word(r,unit,offset,v) && v==expected;
}
bool valid(const Resources& g) noexcept {
    if(!g.unit || !g.count || g.count>Resources::Capacity)return false;
    for(u32 i=0;i<g.count;++i)if(!range(g.regions[i].address,g.regions[i].bytes))return false;
    return g.regions[0].address==g.unit && g.regions[0].bytes==0x400;
}
}
bool capture_resources(Reader r,u32 unit,Resources* result) noexcept {
    if(!result)return false;
    u32 vt=0;
    Resources g{};g.unit=unit;
    if(!word(r,unit,0,vt) || vt!=0x04DE3D6C || !append(g,unit,0x400))return false;
    Gui top{};
    if(!gui(r,unit,top,g) || top.count<15)return false;
    // Cache field -> index supplied to the native element accessor in initialize.
    constexpr u32 fields[]={0x358,0x35c,0x360,0x364,0x368,0x36c,0x370,0x374,
        0x380,0x384,0x388,0x38c,0x390,0x37c,0x378};
    constexpr u32 indices[]={0,12,2,3,4,5,6,7,1,8,9,10,11,13,14};
    for(u32 i=0;i<15;++i)if(!cached(r,unit,fields[i],top.nodes[indices[i]]))return false;
    u32 table=0,count=0;
    if(!animations(r,unit,g,&table,&count) || count<14)return false;
    constexpr u32 anim_fields[]={0x394,0x398,0x39c,0x3a0,0x3a4,0x3a8,0x3ac,
        0x3b8,0x3bc,0x3c0,0x3c4,0x3c8,0x3b0,0x3b4};
    constexpr u32 anim_indices[]={0,2,3,4,9,10,11,1,5,6,7,8,12,13};
    for(u32 i=0;i<14;++i){u32 p=0;
        if(!word(r,table,anim_indices[i]*4,p) || !cached(r,unit,anim_fields[i],p))return false;
    }
    constexpr u32 panel_indices[]={12,6,7,14};
    constexpr u32 text_counts[]={7,1,1,2};
    constexpr u32 text_indices[]={4,3,2,10,9,8,7,1,2,2,3};
    u32 text=0;
    for(u32 panel=0;panel<4;++panel) {
        Gui child{};const u32 p=top.nodes[panel_indices[panel]];
        if(!gui(r,p,child,g) || !animations(r,p,g))return false;
        for(u32 i=0;i<text_counts[panel];++i,++text)
            if(text_indices[text]>=child.count ||
               !cached(r,unit,0x3cc+text*4,child.nodes[text_indices[text]]))return false;
    }
    // Ctor allocates 21 entries of 0x48 and 0x28, with array cookies of 8/4.
    constexpr u32 pool_offsets[]={0x2b4,0x308},strides[]={0x48,0x28},cookies[]={8,4};
    for(u32 i=0;i<2;++i){u32 p=0,n=0,cookie=0;
        if(!word(r,unit,pool_offsets[i],p) || p<=cookies[i] ||
           !word(r,unit,pool_offsets[i]+4,n) || n!=21 ||
           !word(r,p-cookies[i],0,cookie) || cookie!=n ||
           !append(g,p-cookies[i],cookies[i]+n*strides[i]))return false;
    }
    if(!word(r,unit,0,vt) || vt!=0x04DE3D6C)return false;
    *result=g;return true;
}
bool disjoint(const Resources& a,const Resources& b) noexcept {
    if(!valid(a) || !valid(b))return false;
    for(u32 i=0;i<a.count;++i)for(u32 j=0;j<b.count;++j)
        if(overlap(a.regions[i],b.regions[j]))return false;
    return true;
}
bool same_resources(const Resources& a,const Resources& b) noexcept {
    if(!valid(a) || !valid(b) || a.unit!=b.unit || a.count!=b.count)return false;
    for(u32 i=0;i<a.count;++i)
        if(a.regions[i].address!=b.regions[i].address || a.regions[i].bytes!=b.regions[i].bytes)return false;
    return true;
}
}
