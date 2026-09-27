#include "filter_resource.hpp"
namespace rev_genesis {
namespace {
constexpr u32 ResourceVT=0x04DB7A34,Invalid=~0u;
bool range(u32 p,u32 n) noexcept {return p && n && p<=Invalid-n;}
bool overlap(u32 a,u32 an,u32 b,u32 bn) noexcept {
    return a<b+bn && b<a+an;
}
bool word(Reader r,u32 p,u32 off,u32& value) noexcept {
    return range(p,off+4) && r.word && r.word(r.context,p+off,&value);
}
bool capture(Reader r,u32 p,FilterResource& out) noexcept {
    u32 vt=0,owns=0,array_vt=0;
    if(!range(p,0x80) || !word(r,p,0,vt) || vt!=ResourceVT ||
       !word(r,p,0x68,array_vt) || array_vt!=0x04CC7278 ||
       !word(r,p,0x6c,out.count) || out.count>FilterResource::Limit ||
       !word(r,p,0x70,out.capacity) || out.capacity<out.count ||
       out.capacity>FilterResource::Limit || !word(r,p,0x74,owns) ||
       (owns&255)!=1 || !word(r,p,0x78,out.table))return false;
    if(out.capacity) {
        if(!range(out.table,out.capacity*4) || overlap(p,0x80,out.table,out.capacity*4))return false;
    } else if(out.table)return false;
    out.resource=p;
    for(u32 i=0;i<out.count;++i) {
        u32 unit=0,tableVT=0;
        if(!word(r,out.table,i*4,unit))return false;
        // Native consumers explicitly permit null array entries.
        if(unit) {
            if(!range(unit,0x40) || !word(r,unit,0,tableVT) || !tableVT ||
               overlap(unit,0x40,p,0x80) || overlap(unit,0x40,out.table,out.capacity*4))return false;
            for(u32 j=0;j<i;++j)if(out.units[j] && overlap(unit,0x40,out.units[j],0x40))return false;
        }
        out.units[i]=unit;out.vtables[i]=tableVT;
    }
    return true;
}
bool valid(const FilterResource& g) noexcept {
    if(!range(g.resource,0x80) || g.count>g.capacity || g.capacity>FilterResource::Limit)return false;
    if(g.capacity ? !range(g.table,g.capacity*4) : g.table!=0)return false;
    for(u32 i=0;i<g.count;++i)
        if(g.units[i] ? (!range(g.units[i],0x40) || !g.vtables[i]) : g.vtables[i]!=0)return false;
    return true;
}
bool independent(u32 p,u32 bytes,const FilterResource& g) noexcept {
    if(overlap(p,bytes,g.resource,0x80) ||
       (g.capacity && overlap(p,bytes,g.table,g.capacity*4)))return false;
    for(u32 i=0;i<g.count;++i)if(g.units[i] && overlap(p,bytes,g.units[i],0x40))return false;
    return true;
}
bool private_header(Reader r,u32 p) noexcept {
    u32 refs=0,low=0,high=0,accounted=0;
    return word(r,p,0x48,refs) && refs==1 && word(r,p,0x58,low) && !low &&
        word(r,p,0x5c,high) && !high && word(r,p,0x54,accounted) && !accounted;
}
}
bool same_filter_resource(const FilterResource& a,const FilterResource& b) noexcept {
    if(!valid(a) || !valid(b) || a.resource!=b.resource || a.table!=b.table ||
       a.count!=b.count || a.capacity!=b.capacity)return false;
    for(u32 i=0;i<a.count;++i)if(a.units[i]!=b.units[i] || a.vtables[i]!=b.vtables[i])return false;
    return true;
}
bool capture_filter_resource(Reader r,u32 p,FilterResource* out) noexcept {
    if(!out)return false;
    FilterResource a{},b{};
    if(!capture(r,p,a) || !capture(r,p,b) || !same_filter_resource(a,b))return false;
    *out=a;return true;
}
bool disjoint_filter_resources(const FilterResource& a,const FilterResource& b) noexcept {
    if(!valid(a) || !valid(b) || !independent(a.resource,0x80,b) ||
       (a.capacity && !independent(a.table,a.capacity*4,b)))return false;
    for(u32 i=0;i<a.count;++i)if(a.units[i] && !independent(a.units[i],0x40,b))return false;
    return true;
}
u32 copy_filter_resource(const FilterCopyHost& h,u32 source) noexcept {
    FilterResource before{},empty{},after{},copy{};
    if(!h.create || !h.copy || !h.release || !capture_filter_resource(h.memory,source,&before))return 0;
    const u32 p=h.create(h.context);
    // A forged allocator result must never reach native load or release.
    if(!range(p,0x80) || !independent(p,0x80,before))return 0;
    if(!capture_filter_resource(h.memory,p,&empty) || empty.count ||
       !disjoint_filter_resources(empty,before) || !private_header(h.memory,p))return 0;
    const bool copied=h.copy(h.context,source,p);
    const bool captured=capture_filter_resource(h.memory,p,&copy);
    // Do not destruct a graph whose ownership is uncertain: native array
    // destruction would delete its entries, potentially including P1 filters.
    if(!captured || !disjoint_filter_resources(copy,before) || !private_header(h.memory,p))return 0;
    if(!capture_filter_resource(h.memory,source,&after) || !same_filter_resource(before,after))return 0;
    bool ok=copied && copy.count==before.count;
    for(u32 i=0;ok && i<before.count;++i)
        if(copy.vtables[i]!=before.vtables[i])ok=false;
    if(!ok) {h.release(h.context,p);return 0;}
    return p;
}
}
