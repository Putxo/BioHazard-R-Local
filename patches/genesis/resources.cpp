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
namespace {
bool list(Reader r,u32 header,u32 pool,u32 stride,u32 node_offset,u32 list_vt,
          u32& seen,u32& members) noexcept {
    const u32 sentinel=header+4;u32 vt=0,end=0,node=0,tail=0;
    if(!word(r,header,0,vt) || vt!=list_vt ||
       !word(r,sentinel,0,vt) || vt!=0x04CBE468 ||
       !word(r,header,0x10,end) || end!=sentinel ||
       !word(r,sentinel,8,node) || !word(r,sentinel,4,tail))return false;
    u32 previous=sentinel;members=0;
    while(node!=sentinel){
        if(node<pool+node_offset)return false;
        const u32 delta=node-pool-node_offset,index=delta/stride;
        if(delta%stride || index>=Collections::Count || (seen&(1u<<index)))return false;
        seen|=1u<<index;members|=1u<<index;
        u32 prev=0,next=0,data=0;
        if(!word(r,node,4,prev) || prev!=previous || !word(r,node,8,next) ||
           !word(r,node,0x10,data) || data!=pool+index*stride)return false;
        previous=node;node=next;
    }
    return tail==previous;
}
bool collections(Reader r,u32 unit,Collections& out) noexcept {
    u32 vt=0,n=0,cookie=0;
    if(!range(unit,0x400) || !word(r,unit,0,vt) || vt!=0x04DE3D6C ||
       !word(r,unit,0x2B4,out.icon_pool) || !word(r,unit,0x308,out.target_pool) ||
       !range(out.icon_pool,21*0x48) || !range(out.target_pool,21*0x28) ||
       out.icon_pool<8 || out.target_pool<4 ||
       !word(r,unit,0x2B8,n) || n!=21 || !word(r,unit,0x30C,n) || n!=21 ||
       !word(r,out.icon_pool-8,0,cookie) || cookie!=21 ||
       !word(r,out.target_pool-4,0,cookie) || cookie!=21)return false;
    const Region regions[]={{unit,0x400},{out.icon_pool-8,8+21*0x48},{out.target_pool-4,4+21*0x28}};
    for(u32 i=0;i<3;++i)for(u32 j=0;j<i;++j)if(overlap(regions[i],regions[j]))return false;
    u32 seen=0,free=0;
    if(!list(r,unit+0x2BC,out.icon_pool,0x48,0x24,0x04DE45AC,seen,free) ||
       !list(r,unit+0x2D0,out.icon_pool,0x48,0x24,0x04DE45AC,seen,out.active_icons) || seen!=0x1FFFFF)return false;
    seen=0;
    if(!list(r,unit+0x310,out.target_pool,0x28,4,0x04DE45B8,seen,free) ||
       !list(r,unit+0x324,out.target_pool,0x28,4,0x04DE45B8,seen,out.active_targets) || seen!=0x1FFFFF)return false;
    for(u32 i=0;i<Collections::Count;++i){
        const u32 icon=out.icon_pool+i*0x48,target=out.target_pool+i*0x28;
        if(!word(r,icon,0,vt) || vt!=0x04DE44B4 ||
           !word(r,icon,0x24,vt) || vt!=0x04DE45D4 ||
           !word(r,icon,0x30,vt) || vt!=0x04DE45C4 ||
           !word(r,target,0,vt) || vt!=0x04DE45A0 ||
           !word(r,target,4,vt) || vt!=0x04DE45F4 ||
           !word(r,target,0x10,vt) || vt!=0x04DE45E4 ||
           !word(r,target,0x1C,out.targets[i]) ||
           !word(r,icon,0x3C,out.icon_targets[i]) ||
           !word(r,icon,0x40,out.gui_indices[i]))return false;
        const bool target_active=(out.active_targets&(1u<<i))!=0;
        if(target_active!=static_cast<bool>(out.targets[i]))return false;
        if(out.gui_indices[i]!=Limit && out.gui_indices[i]>=5)return false;
        if(!(out.active_icons&(1u<<i)) && (out.icon_targets[i] || out.gui_indices[i]!=Limit))return false;
        if(out.active_icons&(1u<<i)){
            const u32 p=out.icon_targets[i];
            if(p<out.target_pool || (p-out.target_pool)%0x28)return false;
            const u32 index=(p-out.target_pool)/0x28;
            if(index>=Collections::Count || !(out.active_targets&(1u<<index)))return false;
        }
        if(out.targets[i])for(u32 j=0;j<i;++j)if(out.targets[j]==out.targets[i])return false;
        if(out.icon_targets[i])for(u32 j=0;j<i;++j)if(out.icon_targets[j]==out.icon_targets[i])return false;
    }
    return true;
}
bool same(const Collections& a,const Collections& b) noexcept {
    if(a.icon_pool!=b.icon_pool || a.target_pool!=b.target_pool ||
       a.active_icons!=b.active_icons || a.active_targets!=b.active_targets)return false;
    for(u32 i=0;i<Collections::Count;++i)
        if(a.targets[i]!=b.targets[i] || a.icon_targets[i]!=b.icon_targets[i] || a.gui_indices[i]!=b.gui_indices[i])return false;
    return true;
}
}
bool capture_collections(Reader r,u32 unit,Collections* result) noexcept {
    if(!result)return false;
    Collections a{},b{};
    if(!collections(r,unit,a) || !collections(r,unit,b) || !same(a,b))return false;
    *result=a;return true;
}
}
