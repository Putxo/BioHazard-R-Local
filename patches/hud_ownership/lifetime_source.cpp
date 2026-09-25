#include "lifetime_source.hpp"
namespace rev_hud {
namespace {
constexpr u32 NpcVT=0x04D9B25C, PlayerVT=0x04D9CDF4;
constexpr u32 MainVT=0x04E1642C, SubVT=0x04E1649C;
constexpr u32 Active=0x057D9188, Sub=0x057D9184;
constexpr LifetimeSite Sites[] = {
    {0x0277D4FB,0,LifeObject::Actor},
    {0x02B47907,0,LifeObject::Cockpit},
    {0x02B67F51,0,LifeObject::MiniMap},
    {0x0277DC70,1,LifeObject::Actor},
    {0x027B5D00,1,LifeObject::Actor},
    {0x02B47A30,1,LifeObject::Cockpit},
    {0x02B68040,1,LifeObject::MiniMap},
    {0x02DF4F65,2,LifeObject::Actor},
    {0x02DF504B,3,LifeObject::Actor},
    {0x02DF5970,4,LifeObject::Actor},
    {0x02DF5F90,4,LifeObject::Actor},
};
bool equal(Token a,Token b){return a.slot==b.slot && a.generation==b.generation;}
bool equal(const Actor& a,const Actor& b){return a.address==b.address && a.lifetime==b.lifetime && a.serial==b.serial && a.think_mode==b.think_mode;}
LifetimeSource* Sink=nullptr;
struct CaptureGuard { bool& flag; explicit CaptureGuard(bool& f):flag(f){flag=true;} ~CaptureGuard(){flag=false;} };
}
const LifetimeSite* lifetime_site(u32 address) {
    for (const auto& s:Sites) if(s.address==address)return &s;
    return nullptr;
}
bool LifetimeSource::on_thread() const {
    return thread_ && access_.thread_id && access_.thread_id(access_.context)==thread_;
}
bool LifetimeSource::start(u32 thread) {
    if(thread_ || !thread || !access_.word || !access_.thread_id || !access_.stock_self ||
        access_.thread_id(access_.context)!=thread)return false;
    thread_=thread;return true;
}
void LifetimeSource::revoke() {
    // No engine call or destructor. Existing P2 widgets become tombstones now,
    // before a future manager sample, including reentrant destructor events.
    registry_.set_session({});was_ready_=false;
}
bool LifetimeSource::poison(SourceFault f) {
    fault_=f;revoke();return false;
}
bool LifetimeSource::change(bool session_changed) {
    if(revision_==Invalid || (session_changed && epoch_==Invalid))
        return poison(SourceFault::CounterExhausted);
    ++revision_;
    if(session_changed){++epoch_;revoke();}
    return true;
}
bool LifetimeSource::word(u32 a,u32& v,u32 revision) {
    u32 tmp=0;
    if(!a || a>Invalid-3 || revision!=revision_ || fault_!=SourceFault::None ||
        !access_.word(access_.context,a,&tmp) || revision!=revision_ ||
        fault_!=SourceFault::None)return false;
    v=tmp;return true;
}
bool LifetimeSource::field(u32 a,u32 offset,u32& out,u32 revision) {
    return a && offset<=Invalid-3 && a<=Invalid-offset-3 && word(a+offset,out,revision);
}
Token LifetimeSource::find(u32 address,LifeObject kind) const {
    for(u32 i=0;i<Capacity;++i)
        if(live_[i].alive && live_[i].address==address && live_[i].kind==kind)
            return {i,live_[i].generation};
    return {};
}
const LifetimeSource::Live* LifetimeSource::get(Token t,LifeObject kind) const {
    if(!t.valid() || t.slot>=Capacity)return nullptr;
    const auto& e=live_[t.slot];
    return e.alive && e.kind==kind && e.generation==t.generation ? &e : nullptr;
}
bool LifetimeSource::born(u32 address,LifeObject kind) {
    if(!address)return poison(SourceFault::Protocol);
    for(const auto& e:live_)if(e.alive && e.address==address)return poison(SourceFault::Protocol);
    u32 vt=0;
    const u32 expected=kind==LifeObject::Actor ? NpcVT :
        manager_vtable(kind==LifeObject::Cockpit ? ManagerKind::Cockpit : ManagerKind::MiniMap);
    if(!word(address,vt,revision_) || vt!=expected)return poison(SourceFault::Protocol);
    if(next_generation_==Invalid)return poison(SourceFault::CounterExhausted);
    for(auto& e:live_)if(!e.alive){
        if(!change(false))return false;
        e={address,++next_generation_,kind,true};return true;
    }
    return poison(SourceFault::Capacity);
}
bool LifetimeSource::dying(u32 address,LifeObject kind) {
    // Deliberately NEVER read the object, even its vtable. A base destructor can
    // arrive after the derived one has already revoked this same generation.
    const Token t=find(address,kind);
    if(!t.valid())return true;
    bool selected=false;
    for(auto& r:roles_)if(equal(r.token,t)){r={};selected=true;}
    for(auto& p:parents_)if(equal(p,t)){p={};selected=true;}
    live_[t.slot].alive=false;
    return change(selected);
}
bool LifetimeSource::pcs_dying(u32 pcs) {
    bool selected=false;
    for(auto& r:roles_)if(r.pcs==pcs && pcs){r={};selected=true;}
    // A binder destroyed from within its own callback is not a valid pair.
    for(u32 i=0;i<depth_;++i)if(bindings_[i].pcs==pcs)return poison(SourceFault::Protocol);
    return !selected || change(true);
}
bool LifetimeSource::actor(Token token,Actor& out,u32& vt,u32 revision) {
    const auto* e=get(token,LifeObject::Actor);
    if(!e)return false;
    const u32 a=e->address,g=e->generation;u32 serial=0,mode=0;
    if(!word(a,vt,revision) || (vt!=NpcVT && vt!=PlayerVT) ||
        !field(a,0xE3C,serial,revision) || serial>0x7FFFFFFFu ||
        !field(a,0xE40,mode,revision) || mode!=1 || !get(token,LifeObject::Actor))return false;
    out={a,g,static_cast<i32>(serial),mode};return true;
}
bool LifetimeSource::bind_begin(u32 pcs) {
    if(depth_==BindCapacity)return poison(SourceFault::Capacity);
    u32 vt=0;
    if(!word(pcs,vt,revision_))return poison(SourceFault::Protocol);
    const u32 role=vt==MainVT ? 0 : vt==SubVT ? 1 : Invalid;
    bindings_[depth_++]={pcs,role};
    return change(false); // no generation change for an unchanged reenlace
}
bool LifetimeSource::bind_end(u32 pcs) {
    if(!depth_ || bindings_[depth_-1].pcs!=pcs)return poison(SourceFault::Protocol);
    const Binding binding=bindings_[--depth_];
    if(!change(false))return false;
    if(binding.role==Invalid)return true;
    Role next{};u32 vt=0,address=0;
    const u32 revision=revision_;
    if(!word(pcs,vt,revision) || vt!=(binding.role ? SubVT : MainVT) ||
        !field(pcs,0x44,address,revision))return poison(SourceFault::Protocol);
    if(address){
        next.token=find(address,LifeObject::Actor);
        if(actor(next.token,next.actor,next.vtable,revision))next.pcs=pcs;
        else next={}; // never manufacture a birth from the pointer at +44
    }
    auto& old=roles_[binding.role];
    const bool changed=old.pcs!=next.pcs || !equal(old.token,next.token) ||
        !equal(old.actor,next.actor) || old.vtable!=next.vtable;
    old=next;
    return !changed || change(true);
}
bool LifetimeSource::event(u32 site,u32 address) {
    const auto* s=lifetime_site(site);
    if(!s || !on_thread() || fault_!=SourceFault::None)return false;
    switch(s->event){
        case 0:return born(address,s->object);
        case 1:return dying(address,s->object);
        case 2:return bind_begin(address);
        case 3:return bind_end(address);
        case 4:return pcs_dying(address);
    }
    return false;
}
bool LifetimeSource::select_managers(u32 cockpit,u32 minimap) {
    if(!on_thread() || fault_!=SourceFault::None || capturing_ || depth_)return false;
    const Token next[]={find(cockpit,LifeObject::Cockpit),find(minimap,LifeObject::MiniMap)};
    if(!next[0].valid() || !next[1].valid() || cockpit==minimap)return false;
    if(equal(next[0],parents_[0]) && equal(next[1],parents_[1]))return true;
    parents_[0]=next[0];parents_[1]=next[1];return change(true);
}
bool LifetimeSource::capture(LifeSnapshot* out) {
    if(!out || !on_thread() || fault_!=SourceFault::None || capturing_ || depth_)return false;
    CaptureGuard guard(capturing_);
    const u32 revision=revision_;LifeSnapshot snap{};Actor actors[2]{};u32 vts[2]{};
    bool valid=true;
    for(u32 i=0;i<2 && valid;++i){
        const auto* m=get(parents_[i],i ? LifeObject::MiniMap : LifeObject::Cockpit);
        if(!m){valid=false;break;}
        snap.parents[i]=m->address;snap.parent_lifetimes[i]=m->generation;
        u32 vt=0;
        valid=word(m->address,vt,revision) && vt==manager_vtable(static_cast<ManagerKind>(i)) &&
            actor(roles_[i].token,actors[i],vts[i],revision) &&
            equal(actors[i],roles_[i].actor) && vts[i]==roles_[i].vtable;
    }
    u32 active=0,sub=0;
    valid=valid && word(Active,active,revision) && active && word(Sub,sub,revision) &&
        sub==actors[1].address && actors[0].address!=actors[1].address && actors[0].serial!=actors[1].serial;
    if(valid){
        const u32 self=access_.stock_self(access_.context);
        valid=revision_==revision && !depth_ && fault_==SourceFault::None && self==actors[0].address;
    }
    // Re-read all anchors after the native Self lookup. This detects ordinary
    // rebinding/reentrant notifications, not missing hooks or external races.
    for(u32 i=0;i<2 && valid;++i){
        Actor again{};u32 vt=0,mvt=0;
        valid=actor(roles_[i].token,again,vt,revision) && equal(again,actors[i]) && vt==vts[i] &&
            word(snap.parents[i],mvt,revision) && mvt==manager_vtable(static_cast<ManagerKind>(i));
    }
    u32 active2=0,sub2=0;
    valid=valid && word(Active,active2,revision) && active2==active &&
        word(Sub,sub2,revision) && sub2==sub;
    if(!valid){
        if(was_ready_ && fault_==SourceFault::None)change(true);
        else revoke();
        return false; // caller's output remains entirely unchanged
    }
    snap.session={true,epoch_,actors[0],actors[1]};snap.revision=revision_;
    registry_.set_session(snap.session);was_ready_=true;*out=snap;return true;
}
bool bind_lifetime_sink(LifetimeSource& source){if(Sink)return false;Sink=&source;return true;}
}
extern "C" unsigned int rev_hud_lifetime_event(unsigned int site,unsigned int object) {
    return rev_hud::Sink && rev_hud::Sink->event(site,object) ? 1u : 0u;
}
