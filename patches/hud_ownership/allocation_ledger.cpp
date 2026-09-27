#include "allocation_ledger.hpp"
namespace rev_hud {
namespace {
constexpr AllocationSpec Specs[] = {
    {WidgetKind::Reticle,       0x2E0,0x01C11706,0x01C19CCB,0x01C40C04},
    {WidgetKind::MainEquipment, 0x370,0x01C15775,0x01B7D1F0,0x01C0C74C},
    {WidgetKind::MapHerb,       0x2C0,0x01C7855A,0x01C6392A,0x01BAB4F6},
    {WidgetKind::SubEquipment, 0x370,0x01B8BCE6,0x01C39C88,0x01C32956},
    {WidgetKind::Damage,       0x2B0,0x01B8DD52,0x01BB72E2,0x01C2CA29},
    {WidgetKind::Heal,         0x2B0,0x01C461C2,0x01BEA78C,0x01C490C5},
    {WidgetKind::Scanner,      0x400,0x01B97C8A,0x01C802C8,0x01BED6A3},
};
struct Busy {
    bool& v; explicit Busy(bool& b):v(b){v=true;} ~Busy(){v=false;}
};
bool range_ok(u32 a,u32 n) noexcept { return a && n && a<=Invalid-n; }
bool overlap(u32 a,u32 n,u32 b,u32 m) noexcept {
    return range_ok(a,n) && range_ok(b,m) && a<b+m && b<a+n;
}
}
const AllocationSpec*
JanuaryAllocationLedger::spec_for_allocator(u32 target,u32 size,u32 align) noexcept {
    if(align!=0x10) return nullptr;
    for(const auto& s:Specs) if(s.allocator==target && s.size==size) return &s;
    return nullptr;
}
const AllocationSpec*
JanuaryAllocationLedger::spec_for_constructor(u32 target) noexcept {
    for(const auto& s:Specs) if(s.constructor==target) return &s;
    return nullptr;
}
const AllocationSpec*
JanuaryAllocationLedger::spec_for_destroy(u32 target) noexcept {
    for(const auto& s:Specs) if(s.destroy==target) return &s;
    return nullptr;
}
bool JanuaryAllocationLedger::safe() const noexcept {
    return gate_.safe && gate_.safe(gate_.context);
}
AllocationRecord* JanuaryAllocationLedger::find(u32 a) noexcept {
    for(auto& r:records_) if(r.state!=AllocationState::Empty && r.address==a) return &r;
    return nullptr;
}
const AllocationRecord* JanuaryAllocationLedger::find(u32 a) const noexcept {
    for(const auto& r:records_) if(r.state!=AllocationState::Empty && r.address==a) return &r;
    return nullptr;
}
AllocationRecord* JanuaryAllocationLedger::empty() noexcept {
    for(auto& r:records_) if(r.state==AllocationState::Empty) return &r;
    return nullptr;
}
bool JanuaryAllocationLedger::overlaps(u32 a,u32 n) const noexcept {
    for(const auto& r:records_)
        if(r.state!=AllocationState::Empty && overlap(a,n,r.address,r.size)) return true;
    return false;
}
u32 JanuaryAllocationLedger::retained() const noexcept {
    u32 n=0;for(const auto& r:records_) n+=r.state!=AllocationState::Empty;return n;
}
bool JanuaryAllocationLedger::owns(u32 a,u32 n) const noexcept {
    const auto* r=find(a);
    return r && r->state==AllocationState::Certified && (!n || n==r->size);
}
u32 JanuaryAllocationLedger::allocate(u32 target,u32 size,u32 align) noexcept {
    if(busy_ || fault_!=AllocationFault::None || !safe()) {
        fail(busy_?AllocationFault::Protocol:AllocationFault::Unsafe); return 0;
    }
    if(!native_.allocate) {fail(AllocationFault::Config);return 0;}
    const AllocationSpec* spec=spec_for_allocator(target,size,align);
    if(!spec) {fail(AllocationFault::Target);return 0;}
    for(const auto& r:records_)
        if(r.state==AllocationState::Pending) {fail(AllocationFault::Protocol);return 0;}
    AllocationRecord* slot=empty();
    if(!slot) {fail(AllocationFault::Capacity);return 0;}
    Busy lock(busy_);
    const u32 result=native_.allocate(native_.context,target,size,align);
    if(!result) {fail(AllocationFault::Native);return 0;}
    // Remember an otherwise valid return before reporting a reentrant/scope
    // change, so the backend can quarantine the raw block rather than lose it.
    if((result&(align-1)) || !range_ok(result,size)) {
        fail(AllocationFault::Alignment); return result;
    }
    if(overlaps(result,size)) {fail(AllocationFault::Overlap);return result;}
    *slot={result,size,spec->allocator,spec->constructor,spec->destroy,spec->kind,
           AllocationState::Pending};
    if(!safe()) fail(AllocationFault::Changed);
    return result;
}
bool JanuaryAllocationLedger::certify(u32 a,u32 n) noexcept {
    if(busy_ || fault_!=AllocationFault::None || !safe())
        return fail(busy_?AllocationFault::Protocol:AllocationFault::Unsafe);
    AllocationRecord* r=find(a);
    if(!r || r->state!=AllocationState::Pending || r->size!=n ||
       !range_ok(a,n) || (a&0xF)) return fail(AllocationFault::Protocol);
    r->state=AllocationState::Certified;
    return true;
}
u32 JanuaryAllocationLedger::construct(u32 target,u32 self) noexcept {
    if(busy_ || fault_!=AllocationFault::None || !safe() || !native_.construct) {
        fail(!safe()?AllocationFault::Unsafe:AllocationFault::Config);return 0;
    }
    const AllocationSpec* spec=spec_for_constructor(target);
    AllocationRecord* r=find(self);
    if(!spec || !r || r->state!=AllocationState::Certified ||
       r->kind!=spec->kind || r->constructor!=target) {
        fail(AllocationFault::Target);return 0;
    }
    Busy lock(busy_);
    const u32 result=native_.construct(native_.context,target,self);
    if(!safe()) fail(AllocationFault::Changed);
    return result;
}
void JanuaryAllocationLedger::method0(u32 target,u32 self) noexcept {
    if(native_.method0) native_.method0(native_.context,target,self);
}
void JanuaryAllocationLedger::method1(u32 target,u32 self,u32 argument) noexcept {
    const AllocationSpec* spec=spec_for_destroy(target);
    if(!spec) {
        if(native_.method1) native_.method1(native_.context,target,self,argument);
        return;
    }
    AllocationRecord* r=find(self);
    if(busy_ || fault_!=AllocationFault::None || !safe() || !native_.method1 ||
       !r || r->state!=AllocationState::Certified || r->kind!=spec->kind ||
       r->destroy!=target || argument!=1) {
        fail(!safe()?AllocationFault::Unsafe:AllocationFault::Protocol);
        return; // leak/quarantine is safer than freeing an unproved block
    }
    Busy lock(busy_);
    native_.method1(native_.context,target,self,argument);
    *r={};
    if(!safe()) fail(AllocationFault::Changed);
}
u32 JanuaryAllocationLedger::singleton(u32 target) noexcept {
    return native_.singleton ? native_.singleton(native_.context,target) : 0;
}
u32 JanuaryAllocationLedger::contains(u32 target,u32 self,u32 unit) noexcept {
    return native_.contains ? native_.contains(native_.context,target,self,unit) : 0;
}
JanuaryCalls JanuaryAllocationLedger::callbacks() noexcept {
    JanuaryCalls c{};c.context=this;
    c.allocate=[](void* p,u32 t,u32 n,u32 a) noexcept {
        return static_cast<JanuaryAllocationLedger*>(p)->allocate(t,n,a);
    };
    c.construct=[](void* p,u32 t,u32 s) noexcept {
        return static_cast<JanuaryAllocationLedger*>(p)->construct(t,s);
    };
    c.method0=[](void* p,u32 t,u32 s) noexcept {
        static_cast<JanuaryAllocationLedger*>(p)->method0(t,s);
    };
    c.method1=[](void* p,u32 t,u32 s,u32 a) noexcept {
        static_cast<JanuaryAllocationLedger*>(p)->method1(t,s,a);
    };
    c.singleton=[](void* p,u32 t) noexcept {
        return static_cast<JanuaryAllocationLedger*>(p)->singleton(t);
    };
    c.contains=[](void* p,u32 t,u32 s,u32 u) noexcept {
        return static_cast<JanuaryAllocationLedger*>(p)->contains(t,s,u);
    };
    c.damage=[](void* p,u32 unit,u32 actor) noexcept {
        auto& l=*static_cast<JanuaryAllocationLedger*>(p);
        const auto* record=l.find(unit);
        return record && record->state==AllocationState::Certified &&
            record->kind==WidgetKind::Damage && l.native_.damage &&
            l.native_.damage(l.native_.context,unit,actor);
    };
    return c;
}
bool AllocationAdmissionBridge::safe() const noexcept {
    if(!gate_.safe || !gate_.safe(gate_.context)) return false;
    return !upstream_.structural_safe ||
        upstream_.structural_safe(upstream_.memory.context);
}
AdmissionServices AllocationAdmissionBridge::callbacks() noexcept {
    AdmissionServices s{};s.memory={this,[](void* p,u32 a,u32* out) noexcept {
        auto& b=*static_cast<AllocationAdmissionBridge*>(p);
        return out && b.upstream_.memory.word &&
            b.upstream_.memory.word(b.upstream_.memory.context,a,out);
    }};
    s.thread_id=[](void* p) noexcept -> u32 {
        auto& b=*static_cast<AllocationAdmissionBridge*>(p);
        return b.upstream_.thread_id ?
            b.upstream_.thread_id(b.upstream_.memory.context):0;
    };
    s.image_ok=[](void* p) noexcept {
        auto& b=*static_cast<AllocationAdmissionBridge*>(p);
        return b.upstream_.image_ok &&
            b.upstream_.image_ok(b.upstream_.memory.context);
    };
    s.structural_safe=[](void* p) noexcept {
        return static_cast<AllocationAdmissionBridge*>(p)->safe();
    };
    s.write_word=[](void* p,u32 a,u32 v) noexcept {
        auto& b=*static_cast<AllocationAdmissionBridge*>(p);
        return b.upstream_.write_word &&
            b.upstream_.write_word(b.upstream_.memory.context,a,v);
    };
    s.fresh_allocation=[](void* p,u32 a,u32 n) noexcept {
        auto& b=*static_cast<AllocationAdmissionBridge*>(p);
        if(!b.safe()) return false;
        if(b.upstream_.fresh_allocation &&
           !b.upstream_.fresh_allocation(b.upstream_.memory.context,a,n))
            return false;
        return b.ledger_.certify(a,n);
    };
    return s;
}
}
