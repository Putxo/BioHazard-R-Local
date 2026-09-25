#include "january_backend.hpp"
namespace rev_hud {
namespace {
constexpr JanuaryType Types[] = {
    {0x2E0,0x01C11706,0x01C19CCB,0x01C40C04,0x01B94A7B,0x01C7272C,0x01C7E437,0x01C6EF7D},
    {0x370,0x01C15775,0x01B7D1F0,0x01C0C74C,0x01B96D58,0x01C65A9A,0x01BF295F,0x01C6445B},
    {0x2C0,0x01C7855A,0x01C6392A,0x01BAB4F6,0x01C33D4C,0x01C28CAD,0x01BF295F,0x01C67381},
};
constexpr u32 GetUnit = 0x01C8C27B, Contains = 0x0326AA90;
struct Busy { bool& flag; explicit Busy(bool& f):flag(f){flag=true;} ~Busy(){flag=false;} };
bool overlap(u32 a,u32 n,u32 b,u32 m) {
    return a && b && a <= Invalid-n && b <= Invalid-m && a < b+m && b < a+n;
}
}
const JanuaryType* january_type(WidgetKind k) {
    const u32 i=static_cast<u32>(k); return i<3 ? &Types[i] : nullptr;
}
bool JanuaryBackend::allow(NativeOp op,WidgetKind k,u32 u,u32 phase,u32 context) {
    return (host_.permit && host_.permit(host_.memory.context,op,k,u,phase,context)) || fail(NativeFault::Permit);
}
bool JanuaryBackend::word(u32 a,u32& out) {
    return (a && a<=Invalid-3 && host_.memory.word &&
        host_.memory.word(host_.memory.context,a,&out)) || fail(NativeFault::Memory);
}
bool JanuaryBackend::field(u32 u,u32 off,u32& v) {
    return (u && off<=Invalid-3 && u<=Invalid-off-3 && word(u+off,v)) || fail(NativeFault::Memory);
}
bool JanuaryBackend::write(u32 a,u32 v) {
    // Only flags and time scale inside our exclusively owned, constructed units.
    for (u32 i=0;i<3;++i) if (entries_[i].unit && entries_[i].constructed &&
        !entries_[i].quarantine && (a==entries_[i].unit+0xC || a==entries_[i].unit+0x1C))
        return (allow(NativeOp::Write,static_cast<WidgetKind>(i),entries_[i].unit) &&
            targets(static_cast<WidgetKind>(i)) && detached(entries_[i].unit,static_cast<WidgetKind>(i)) &&
            host_.write && host_.write(host_.memory.context,a,v)) || fail(NativeFault::Memory);
    return fail(NativeFault::Borrowed);
}
bool JanuaryBackend::targets(WidgetKind k) {
    const auto* t=january_type(k); const auto* info=kind_info(k);
    if (!t || !info) return fail(NativeFault::Layout);
    const u32 slots[]={0,5,8,9,11}, expected[]={t->destroy,t->initialize,t->phase8,t->phase9,t->draw};
    for (u32 i=0;i<5;++i) {
        u32 got=0; if (!word(info->vtable+slots[i]*4,got) || got!=expected[i]) return fail(NativeFault::Target);
    }
    return true;
}
bool JanuaryBackend::borrowed_or_overlap(u32 u,u32 size) const {
    if (!u || u>Invalid-size) return true;
    for (u32 i=0;i<3;++i) {
        if (overlap(u,size,originals_[i],Types[i].size)) return true;
        if (overlap(u,size,entries_[i].unit,Types[i].size)) return true;
    }
    // Parent allocation sizes are not asserted here; full-range exclusion is
    // mandatory in fresh_allocation(). At least reject direct parent aliases.
    return u==parents_[0] || u==parents_[1];
}
bool JanuaryBackend::detached(u32 u,WidgetKind k) {
    u32 vt=0,flags=0,next=0,prev=0,link=0;
    if (!field(u,0,vt) || vt!=kind_info(k)->vtable || !field(u,0xC,flags) ||
        !field(u,0x14,next) || !field(u,0x18,prev)) return fail(NativeFault::Layout);
    if (((flags>>3)&127)!=127 || next || prev) return fail(NativeFault::Attached);
    if (k!=WidgetKind::MapHerb && (!field(u,0x290,link) || link)) return fail(NativeFault::Attached);
    // cUnit flags are not enough: a single scheduled unit may have null links.
    const u32 manager=calls_.singleton(calls_.context,GetUnit);
    if (!manager) return fail(NativeFault::Layout);
    const u32 found=calls_.contains(calls_.context,Contains,manager,u);
    if (found!=0) return fail(NativeFault::Attached);
    return true;
}
bool JanuaryBackend::configure(const u32 p[2],const u32 originals[3]) {
    if (busy_) return fail(NativeFault::Reentrant);
    for (const auto& e:entries_) if (e.unit) return fail(NativeFault::Quarantined);
    configured_=false;
    if (!p || !originals || !host_.write || !host_.fresh_allocation || !host_.memory.word ||
        !calls_.allocate || !calls_.construct || !calls_.method0 || !calls_.method1 ||
        !calls_.singleton || !calls_.contains) return fail(NativeFault::Layout);
    Busy lock(busy_);
    if (!allow(NativeOp::Structural,WidgetKind::Reticle)) return false;
    for (u32 i=0;i<2;++i) {
        u32 vt=0; if (!field(p[i],0,vt) || vt!=manager_vtable(static_cast<ManagerKind>(i))) return fail(NativeFault::Layout);
    }
    if (p[0]==p[1]) return fail(NativeFault::Borrowed);
    GuiTree trees[3]{};
    for (u32 i=0;i<3;++i) {
        const auto k=static_cast<WidgetKind>(i);
        if (!targets(k) || !capture_tree(host_.memory,originals[i],k,&trees[i])) return fail(NativeFault::Resources);
        for (u32 j=0;j<i;++j) if (!disjoint_trees(trees[i],trees[j])) return fail(NativeFault::Borrowed);
        if (originals[i]==p[0] || originals[i]==p[1]) return fail(NativeFault::Borrowed);
    }
    for(u32 i=0;i<2;++i) parents_[i]=p[i];
    for(u32 i=0;i<3;++i) originals_[i]=originals[i];
    fault_=NativeFault::None; configured_=true; return true;
}
bool JanuaryBackend::safe() {
    return !busy_ && configured_ && allow(NativeOp::Structural,WidgetKind::Reticle);
}
u32 JanuaryBackend::retained(WidgetKind k) const {
    const u32 i=static_cast<u32>(k); return i<3 ? entries_[i].unit : 0;
}
u32 JanuaryBackend::create(WidgetKind k) {
    const auto* t=january_type(k);
    if (busy_ || !configured_ || !t) { fail(NativeFault::Reentrant);return 0; }
    auto& e=entries_[static_cast<u32>(k)];
    if(e.unit) {fail(NativeFault::Quarantined);return 0;}
    Busy lock(busy_);
    if (!allow(NativeOp::Structural,k) || !targets(k)) return 0;
    const u32 u=calls_.allocate(calls_.context,t->allocator,t->size,0x10);
    if (!u) { fail(NativeFault::Allocation);return 0; }
    if ((u&15) || borrowed_or_overlap(u,t->size)) { fail(NativeFault::Borrowed);return 0; }
    // Keep an allocation record even if construction/admission later fails.
    // Such a raw or uncertain block is NOT passed to a scalar destructor.
    e.unit=u;e.quarantine=true;
    if (!host_.fresh_allocation(host_.memory.context,u,t->size) || !allow(NativeOp::Structural,k,u)) {
        fail(NativeFault::Allocation);return 0;
    }
    const u32 result=calls_.construct(calls_.context,t->constructor,u);
    e.constructed=result==u;
    if (!e.constructed) { fail(NativeFault::Layout); return 0; }
    if (!allow(NativeOp::Structural,k,u) || !detached(u,k)) return 0;
    e.quarantine=false;return u;
}
bool JanuaryBackend::init(u32 u,WidgetKind k) {
    if (busy_ || !january_type(k)) return fail(NativeFault::Reentrant);
    auto& e=entries_[static_cast<u32>(k)];
    if (!u || u!=e.unit || !e.constructed || e.quarantine || e.ready) return fail(NativeFault::Layout);
    Busy lock(busy_);
    if (!allow(NativeOp::Initialize,k,u) || !targets(k) || !detached(u,k)) return false;
    calls_.method0(calls_.context,january_type(k)->initialize,u); // void, EAX ignored
    if (!allow(NativeOp::Initialize,k,u) || !detached(u,k)) { e.quarantine=true;return false; }
    GuiTree tree{};
    if (!capture_tree(host_.memory,u,k,&tree)) return fail(NativeFault::Resources);
    e.ready=true;return true;
}
bool JanuaryBackend::destroy(u32 u,WidgetKind k) {
    if (busy_ || !january_type(k)) return fail(NativeFault::Reentrant);
    auto& e=entries_[static_cast<u32>(k)];
    if (!u || u!=e.unit || !e.constructed || e.quarantine) return fail(NativeFault::Quarantined);
    Busy lock(busy_);
    if (!allow(NativeOp::Destroy,k,u) || !targets(k) || !detached(u,k)) return false;
    calls_.method1(calls_.context,january_type(k)->destroy,u,1);
    e={}; return true; // no memory read after scalar deleting destructor
}
bool JanuaryBackend::phase(u32 u,WidgetKind k,u32 phase,u32 context) {
    if (busy_ || !january_type(k)) return fail(NativeFault::Reentrant);
    auto& e=entries_[static_cast<u32>(k)];
    if (!u || u!=e.unit || !e.ready || e.quarantine || (phase!=8 && phase!=9 && phase!=11)) return fail(NativeFault::Layout);
    Busy lock(busy_);
    if (!allow(NativeOp::Phase,k,u,phase,context) || !targets(k) || !detached(u,k)) return false;
    u32 flags=0,skip=0;
    if (!field(u,0xC,flags)) return false;
    if (k!=WidgetKind::MapHerb) {
        if (!field(u,0x294,skip)) return false; // only low byte belongs to ForceSkip
        if (!(flags&0x4000) || (skip&255)) return true; // successful native skip
    } else if (phase==11 && !(flags&0x4000)) return true;
    const auto* t=january_type(k);
    if (phase==11) {
        u32 view=0;
        if (!field(context,0x158,view) || (view&255)!=1) return fail(NativeFault::Layout);
        if (!view_allowed(flags,1)) return true;
        calls_.method1(calls_.context,t->draw,u,context);
    } else {
        if (phase==8) {
            const u32 parent=parents_[k==WidgetKind::MapHerb?1:0]; u32 vt=0,scale=0;
            if (!field(parent,0,vt) || vt!=manager_vtable(kind_info(k)->manager) || !field(parent,0x1C,scale)) return fail(NativeFault::Layout);
            // Reject NaN, infinity and negative scales rather than feeding them
            // into an unvalidated frame. This is conservative adapter policy.
            if ((scale&0x80000000u) || (scale&0x7F800000u)==0x7F800000u) return fail(NativeFault::Layout);
            if (!write(u+0x1C,scale)) return false;
        }
        calls_.method0(calls_.context,phase==8?t->phase8:t->phase9,u);
    }
    // A phase can enqueue work or reenter the host; do not treat a changed
    // ownership/driver admission as a successful detached callback.
    if (!allow(NativeOp::Phase,k,u,phase,context) || !detached(u,k)) {
        e.quarantine=true; return false;
    }
    return true;
}
Backend JanuaryBackend::callbacks() {
    Backend b{};
    b.memory={this,[](void* c,u32 a,u32* v) noexcept {
        auto& n=*static_cast<JanuaryBackend*>(c);return v && n.allow(NativeOp::Read,WidgetKind::Reticle,a) && n.word(a,*v);
    }};
    b.safe_point=[](void* c) noexcept {return static_cast<JanuaryBackend*>(c)->safe();};
    b.construct=[](void* c,WidgetKind k) noexcept {return static_cast<JanuaryBackend*>(c)->create(k);};
    b.write_word=[](void* c,u32 a,u32 v) noexcept {return static_cast<JanuaryBackend*>(c)->write(a,v);};
    b.checked_initialize=[](void* c,u32 u,WidgetKind k) noexcept {return static_cast<JanuaryBackend*>(c)->init(u,k);};
    b.checked_destroy=[](void* c,u32 u,WidgetKind k) noexcept {return static_cast<JanuaryBackend*>(c)->destroy(u,k);};
    b.checked_phase=[](void* c,u32 u,WidgetKind k,u32 p,u32 x) noexcept {return static_cast<JanuaryBackend*>(c)->phase(u,k,p,x);};
    return b;
}
}
