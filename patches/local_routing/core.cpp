// Original local-coop routing helpers. Engine addresses are supplied by the linker.
// Requires the exact January build and the live Sub0 tracker from the base patch.
// Host tests exercise this source with mocked engine services, not gameplay.
using U8=unsigned char; using U32=unsigned int; using I32=int;
#if defined(__i386__)
#define TC __attribute__((thiscall))
#else
#define TC
#endif
struct Binding { void* owner; void* actor; };
extern "C" {
extern volatile U32 lc_active;
extern void* volatile lc_sub0;
Binding lc_bindings[512] = {};
extern U8 api_door_dti;
void* api_self();
void* TC api_find(void*,I32);
I32 TC api_local_serial(void*);
bool TC api_allowed(void*,I32);
bool TC api_kind_of(void*,void*);
}
static I32 field(void* p,U32 n) { return *reinterpret_cast<I32*>(static_cast<U8*>(p)+n); }
static void* valid_sub() {
    void* p=lc_sub0;
    if(!lc_active || !p || field(p,0xE40)!=1) return nullptr;
    return p;
}
static bool valid_serial(void* p) { return p && U32(field(p,0xE3C))<=127; }
static Binding* binding(void* owner,bool create) {
    if(!owner) return nullptr;
    for(auto& b:lc_bindings) {
        void* key=__atomic_load_n(&b.owner,__ATOMIC_ACQUIRE);
        if(key==owner) return &b;
        if(!key && create) {
            void* expected=nullptr;
            if(__atomic_compare_exchange_n(&b.owner,&expected,owner,false,
                                           __ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) return &b;
            if(expected==owner) return &b;
        }
    }
    return nullptr; // A full table falls back to the original P1 route.
}
static bool is_door(void* owner) {
    if(!owner) return false;
    using DtiFn=void* (TC *)(void*);
    auto vtable=*reinterpret_cast<void***>(owner);
    if(!vtable || !vtable[4]) return false;
    void* dti=reinterpret_cast<DtiFn>(vtable[4])(owner);
    return dti && api_kind_of(dti,&api_door_dti);
}
static bool known_actor(void* p) {
    if(!p) return false;
    void* self=api_self();
    void* sub=valid_sub();
    if(p!=self && p!=sub) return false; // Never dereference a stale table pointer.
    if(!valid_serial(p)) return false;
    if(p==self) return true;
    return !self || field(self,0xE3C)!=field(p,0xE3C);
}
static bool finite(float v) { return v<=3.402823466e38F && v>=-3.402823466e38F; }
static float distance_squared(void* owner,void* actor) {
    using MatrixFn=void* (TC *)(void*,I32);
    auto vt=*reinterpret_cast<void***>(owner);
    if(!vt || !vt[24]) return -1;
    void* matrix=reinterpret_cast<MatrixFn>(vt[24])(owner,-1);
    if(!matrix) return -1;
    const float* a=reinterpret_cast<float*>(static_cast<U8*>(actor)+0x40);
    const float* b=reinterpret_cast<float*>(static_cast<U8*>(matrix)+0x30);
    float sum=0;
    for(U32 k=0;k!=3;++k) {
        if(!finite(a[k]) || !finite(b[k])) return -1;
        float delta=a[k]-b[k]; sum+=delta*delta;
    }
    return finite(sum)?sum:-1;
}
extern "C" {
// Executed at the binder boundary only for a newly converted local session.
// Entry lifecycle within a scene remains owned by the engine's object updates.
void lc_forget_all() {
    for(auto& b:lc_bindings) { b.actor=nullptr; b.owner=nullptr; }
}
// Called only at the audited door initialization hook; clears reused addresses.
void lc_reset_owner(void* owner) {
    Binding* b=binding(owner,lc_active!=0);
    if(b) b->actor=nullptr;
}
// Start of the audited WaitState proximity pass, before any early exit.
void* lc_begin(void* owner) {
    lc_reset_owner(owner);
    void* self=api_self();
    return self ? self : valid_sub();
}
// The original pre-sensor availability gate checked only Self. Preserve all its
// original checks, applying the same method separately to the exact local Sub0.
bool lc_is_managed(void* owner) { return lc_active && binding(owner,false); }
bool lc_availability(void* owner,void* actor,I32 kind) {
    if(actor && api_allowed(actor,kind)) return true;
    if(!lc_is_managed(owner)) return false;
    void* sub=valid_sub();
    return sub && sub!=actor && valid_serial(sub) && api_allowed(sub,kind);
}
bool lc_wait_allowed(void* owner,void* actor,I32 kind) {
    Binding* b=binding(owner,false);
    if(lc_active && b && (!known_actor(b->actor) || b->actor!=actor)) return false;
    return actor && api_allowed(actor,kind);
}
// Called only for actors the original sensor actually returned. Neither expands
// sensor range nor admits arbitrary NPCs. A tie stays with Self.
bool lc_consider(void* owner,void* actor) {
    if(!lc_active || !known_actor(actor)) return false;
    Binding* b=binding(owner,false);
    if(!b || !api_allowed(actor,0)) return false;
    float d=distance_squared(owner,actor);
    if(d<0) return false;
    void* old=b->actor;
    float previous=(old && known_actor(old))?distance_squared(owner,old):-1;
    if(previous<0 || d<previous || (d==previous && actor==api_self())) b->actor=actor;
    return b->actor!=nullptr;
}
void* lc_wait_actor(void* owner) {
    Binding* b=binding(owner,false);
    if(lc_active && b && known_actor(b->actor)) return b->actor;
    return api_self();
}
I32 lc_wait_serial(void* manager,void* actor) {
    if(lc_active && known_actor(actor)) return field(actor,0xE3C);
    return api_local_serial(manager);
}
// Wrapper is installed at five door callsites, NOT at the global finder.
void* TC lc_find_actor(void* manager,I32 serial) {
    if(!manager) return nullptr;
    void* stock=api_find(manager,serial);
    if(stock) return stock;
    void* sub=valid_sub();
    if(sub && U32(serial)<=127 && field(sub,0xE3C)==serial) return sub;
    return nullptr;
}
U32 TC lc_actor_pad(void* actor) {
    return actor && actor==valid_sub() ? 1U : 0U;
}
// The actual uItem callback. Also reject a stale local flag after the actor
// changes away from Pad; this never treats a Network actor as a local member.
U32 TC lc_item_member(void* owner,U32) {
    if(!owner) return 0;
    void* sub=valid_sub();
    if(!sub) return 0;
    void* candidate=*reinterpret_cast<void**>(static_cast<U8*>(owner)+0xF48);
    return candidate==sub ? 1U : 0U;
}
// Common uObjModel member callback: no uItem layout is assumed here.
U32 TC lc_model_member(void* owner,U32) {
    if(!lc_active || !binding(owner,false) || !is_door(owner)) return 0;
    Binding* b=binding(owner,false);
    void* sub=valid_sub();
    if(!sub) return 0;
    if(b->actor && known_actor(b->actor)) return b->actor==sub ? 1U : 0U;
    return 0;
}
}

// Focused action/rescue continuation. No global Self lookup is replaced.
struct ActionDelegate {
    void* context;
    U32 (*invoke)(void* const*,U32);
    bool (*manage)(void**,void**,U32);
};
struct ActionGroup { ActionDelegate callbacks[5]; };
#if defined(__i386__)
static_assert(sizeof(ActionDelegate)==12 && sizeof(ActionGroup)==60,"January delegate ABI");
#endif
extern "C" {
void* TC api_set_group(void*,const ActionGroup*);
U32 api_static_invoke(void* const*,U32);
bool api_static_manage(void**,void**,U32);
extern U8 api_charge_zero,api_quint_zero,api_ac_charge_zero,api_foot_zero;
// Success changes the victim but consumes inventory from the OTHER actor.
// Both eligibility and member selection must use that same opposite actor.
void* lc_rescue_other(void* victim,void* stock) {
    void* sub=valid_sub();
    if(!victim || !sub) return stock;
    void* self=api_self();
    if(!self || self==sub || field(self,0xE40)!=1 || !valid_serial(self) ||
       !valid_serial(sub) || field(self,0xE3C)==field(sub,0xE3C)) return stock;
    if(victim==self) return sub;
    if(victim==sub) return self;
    return stock;
}
void* lc_rescue_control(void* control,void* stock) {
    if(!control || !lc_active) return stock;
    void* victim;
    __builtin_memcpy(&victim,static_cast<U8*>(control)+0x0C,sizeof(victim));
    return lc_rescue_other(victim,stock);
}
// Same cdecl (pointer-to-context, event argument) ABI as the audited static
// invoker. The context is a non-owning actor pointer, never a new allocation.
U32 lc_delegate_pad(void* const* context,U32) {
    return context ? lc_actor_pad(*context) : 0;
}
void* lc_install_actor_group(void* command,const ActionGroup* group,void* actor) {
    if(!command || !group) return nullptr;
    const auto& first=group->callbacks[0];
    if(lc_actor_pad(actor)!=1 || first.invoke!=api_static_invoke ||
       first.manage!=api_static_manage ||
       (first.context!=&api_charge_zero && first.context!=&api_quint_zero))
        return api_set_group(command,group);
    // POD snapshot: retain the other four callbacks and let the native setter
    // perform their native copies. Neither mutate nor destroy the caller group.
    ActionGroup selected=*group;
    selected.callbacks[0].context=actor;
    selected.callbacks[0].invoke=lc_delegate_pad;
    // The original static manager only copies/clears/compares the pointer;
    // it does not free its pointee. Keep that audited lifetime policy.
    return api_set_group(command,&selected);
}
}

extern "C" {
// These two components are initialized before PCS may convert the actor to Pad.
// Capture the verified actor argument now; decide local membership at invocation.
// With local mode off the replacement still returns the original constant zero.
void* lc_install_component_group(void* command,const ActionGroup* group,void* actor) {
    if(!command || !group) return nullptr;
    const auto& first=group->callbacks[0];
    if(!actor || first.invoke!=api_static_invoke || first.manage!=api_static_manage ||
       (first.context!=&api_ac_charge_zero && first.context!=&api_foot_zero))
        return api_set_group(command,group);
    ActionGroup selected=*group;
    selected.callbacks[0].context=actor;
    selected.callbacks[0].invoke=lc_delegate_pad;
    return api_set_group(command,&selected);
}
}
