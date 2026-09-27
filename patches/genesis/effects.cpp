#include "effects.hpp"
namespace rev_genesis {
namespace {
constexpr u32 Offsets[]={0x2F0,0x33C,0x338},Sizes[]={0x48,0xD0,0x150};
constexpr u32 VT[]={0x04DB80CC,0x04DB833C,0x04F9A7F4};
bool word(Reader r,u32 p,u32 off,u32& v) noexcept {
    return p && off<=~0u-4 && p<=~0u-off-4 && r.word && r.word(r.context,p+off,&v);
}
bool overlap(u32 a,u32 n,u32 b,u32 m) noexcept {
    return a && b && n && m && a<b+m && b<a+n;
}
bool key(const EffectKey& a,const EffectKey& b) noexcept {
    return a.valid() && b.valid() && a.address==b.address && a.generation==b.generation && a.kind==b.kind;
}
struct Range {u32 p,n;};
struct Regions {Range a[FilterResource::Limit+6]{};u32 count=0;};
struct Guard {
    Reader base;EffectLifetime& life;EffectKey key;
    Reader reader() noexcept {return {this,[](void* p,u32 a,u32* out) noexcept {
        auto& g=*static_cast<Guard*>(p);
        return g.life.live(g.key) && g.base.word && g.base.word(g.base.context,a,out) && g.life.live(g.key);
    }};}
};
bool append(Regions& r,u32 p,u32 n) noexcept {
    if(!p || !n || p>~0u-n || r.count>=FilterResource::Limit+6)return false;
    for(u32 i=0;i<r.count;++i)if(overlap(p,n,r.a[i].p,r.a[i].n))return false;
    r.a[r.count++]={p,n};return true;
}
bool regions(const Effects& e,Regions& r) noexcept {
    if(!append(r,e.scanner,0x400))return false;
    for(u32 i=0;i<3;++i)
        if(!e.units[i].valid() || e.units[i].kind!=static_cast<EffectKind>(i) ||
           !append(r,e.units[i].address,Sizes[i]))return false;
    const auto& f=e.resource;
    if(!same_filter_resource(f,f) || !append(r,f.resource,0x80))return false;
    if(f.table && !append(r,f.table,f.capacity?f.capacity*4:4))return false;
    if(f.capacity && !f.table)return false;
    for(u32 i=0;i<f.count;++i)if(f.units[i] && (!f.vtables[i] || !append(r,f.units[i],0x40)))return false;
    return true;
}
bool capture(Reader r,EffectLifetime& life,u32 scanner,Effects& out) noexcept {
    u32 vt=0;
    if(!scanner || scanner>~0u-0x400 || !word(r,scanner,0,vt) || vt!=0x04DE3D6C)return false;
    out.scanner=scanner;
    for(u32 i=0;i<3;++i){u32 p=0,flags=0;
        if(!word(r,scanner,Offsets[i],p))return false;
        out.units[i]=life.capture(p,static_cast<EffectKind>(i));
        Guard guard{r,life,out.units[i]};const auto memory=guard.reader();
        if(!out.units[i].valid() || !word(memory,p,0,vt) || vt!=VT[i] || !word(memory,p,12,flags) ||
           ((flags>>3)&127)!=6 || (flags&7)<1 || (flags&7)>2)return false;
    }
    u32 callback=0,parent=0,resource=0;
    Guard guard{r,life,out.units[0]};const auto memory=guard.reader();
    if(!word(memory,out.units[0].address,0x44,callback) || callback!=scanner+0x2F4 ||
       !word(memory,callback,0,vt) || vt!=0x04DE4564 || !word(memory,callback,4,parent) || parent!=scanner ||
       !word(memory,out.units[0].address,0x40,resource) || !capture_filter_resource(memory,resource,&out.resource))return false;
    Regions all{};if(!regions(out,all))return false;
    for(const auto& k:out.units)if(!life.live(k))return false;
    return true;
}
}
bool same_effects(const Effects& a,const Effects& b) noexcept {
    if(!a.scanner || a.scanner!=b.scanner || !same_filter_resource(a.resource,b.resource))return false;
    for(u32 i=0;i<3;++i)if(!key(a.units[i],b.units[i]))return false;
    return true;
}
bool capture_effects(Reader r,EffectLifetime& life,u32 scanner,Effects* out) noexcept {
    if(!out)return false;
    Effects a{},b{};
    if(!capture(r,life,scanner,a) || !capture(r,life,scanner,b) || !same_effects(a,b))return false;
    *out=a;return true;
}
bool disjoint_effects(const Effects& a,const Effects& b) noexcept {
    Regions x{},y{};if(!regions(a,x) || !regions(b,y))return false;
    for(u32 i=0;i<x.count;++i)for(u32 j=0;j<y.count;++j)
        if(overlap(x.a[i].p,x.a[i].n,y.a[j].p,y.a[j].n))return false;
    return true;
}
bool effect_view(Reader r,const Effects& e,u32 view) noexcept {
    if(view>1)return false;
    for(const auto& k:e.units){u32 flags=0;
        if(!k.valid() || !word(r,k.address,12,flags) || ((flags>>16)&0x3FF)!=(1u<<view))return false;
    }
    return true;
}
bool retire_effects(Reader r,EffectLifetime& life,const Effects& expected,
                    bool (*write)(void*,u32,u32) noexcept) noexcept {
    Effects now{};u32 flags[3]{};
    if(!write || !capture_effects(r,life,expected.scanner,&now) || !same_effects(now,expected))return false;
    for(u32 i=0;i<3;++i){Guard guard{r,life,now.units[i]};
        if(!word(guard.reader(),now.units[i].address,12,flags[i]))return false;}
    const auto live=[&]() noexcept {for(const auto& k:now.units)if(!life.live(k))return false;return true;};
    // This callback points inside the Scanner being retired. Break it before
    // marking any unit for deferred destruction; the scheduler owns each unit.
    if(!live() || !write(r.context,now.units[0].address+0x44,0))return false;
    for(u32 i=0;i<3;++i){
        // All six cUnit activity bits off, including keep-alive; native kill
        // transitions low three state bits 1/2 -> 3. Group/view bits survive.
        if(!live() || !write(r.context,now.units[i].address+12,(flags[i]&~0xFC07u)|3u))return false;
    }
    for(u32 i=0;i<3;++i)
        if(!live() || !write(r.context,now.scanner+Offsets[i],0))return false;
    u32 value=0;
    if(!live() || !word(r,now.units[0].address,0x44,value) || value)return false;
    for(u32 i=0;i<3;++i)
        if(!live() || !word(r,now.scanner,Offsets[i],value) || value || !live() ||
           !word(r,now.units[i].address,12,value) || value!=((flags[i]&~0xFC07u)|3u))return false;
    return live();
}
}
