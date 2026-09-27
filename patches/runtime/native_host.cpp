#include "runtime.hpp"
#if !defined(__i386__)
#error This native adapter requires the January PE32 x86 image.
#endif
using rev_hud::u32;
using usize=__SIZE_TYPE__;
void* operator new(usize,void* p) noexcept {return p;}
extern "C" {
void __attribute__((thiscall)) rev_action_stock_draw(u32 icon,u32 context) noexcept;
void __attribute__((thiscall)) rev_genesis_filter_stock_draw(u32 unit,u32 context) noexcept;
void __attribute__((thiscall)) rev_genesis_noise_stock_draw(u32 unit,u32 context) noexcept;
void __attribute__((thiscall)) rev_genesis_outline_stock_draw(u32 unit,u32 context) noexcept;
void* memset(void* p,int v,usize n) {
    auto* d=static_cast<unsigned char*>(p);for(usize i=0;i<n;++i)d[i]=static_cast<unsigned char>(v);return p;
}
void* memcpy(void* p,const void* q,usize n) {
    auto* d=static_cast<unsigned char*>(p);auto* s=static_cast<const unsigned char*>(q);
    for(usize i=0;i<n;++i)d[i]=s[i];return p;
}
}
namespace {
struct MemoryInfo {u32 base,allocation_base,allocation_protect,size,state,protect,type;};
static_assert(sizeof(MemoryInfo)==28,"Win32 MEMORY_BASIC_INFORMATION");
using Query=u32(__attribute__((stdcall))*)(const void*,MemoryInfo*,u32);
using Thread=u32(__attribute__((stdcall))*)();
bool readable(u32 a,bool writing) noexcept {
    if(!a || a>0xFFFFFFFCu)return false;
    const auto query=*reinterpret_cast<Query volatile*>(0x057DB034u);
    if(!query)return false;
    MemoryInfo m{};
    if(query(reinterpret_cast<void*>(a),&m,sizeof(m))!=sizeof(m) ||
       m.state!=0x1000 || (m.protect&0x100) || a<m.base ||
       m.size<4 || a-m.base>m.size-4)return false;
    const u32 p=m.protect&255;
    return writing ? (p==4 || p==8 || p==0x40 || p==0x80) :
        (p==2 || p==4 || p==8 || p==0x20 || p==0x40 || p==0x80);
}
bool word(void*,u32 a,u32* out) noexcept {
    if(!out || !readable(a,false))return false;
    *out=*reinterpret_cast<volatile u32*>(a);return true;
}
bool write(void*,u32 a,u32 v) noexcept {
    if(!readable(a,true))return false;
    *reinterpret_cast<volatile u32*>(a)=v;return true;
}
u32 thread(void*) noexcept {
    const auto f=*reinterpret_cast<Thread volatile*>(0x057DB214u);return f?f():0;
}
bool accepted=false;
bool image(void*) noexcept {return accepted;}
u32 self(void*) noexcept {return reinterpret_cast<u32(*)()>(0x01C16468u)();}
u32 action_member(void*,u32 command) noexcept {
    using Member=u32(__attribute__((thiscall))*)(u32);
    return reinterpret_cast<Member>(0x01E8CDC0u)(command);
}
void action_draw(void*,u32 icon,u32 context) noexcept {
    rev_action_stock_draw(icon,context);
}
void effect_draw(void*,u32 kind,u32 unit,u32 context) noexcept {
    if(kind==0)rev_genesis_filter_stock_draw(unit,context);
    if(kind==1)rev_genesis_noise_stock_draw(unit,context);
    if(kind==2)rev_genesis_outline_stock_draw(unit,context);
}
u32 action_rank(void*,u32 raw) noexcept {return reinterpret_cast<u32(*)(u32)>(0x01E87DB0u)(raw);}
bool aim_weapon_hidden(void*,u32 actor) noexcept {
    using Predicate=unsigned char(__attribute__((thiscall))*)(u32);
    return reinterpret_cast<Predicate>(0x01BB2FE4u)(actor)!=0;
}
bool weapon_class(void*,u32 weapon,u32* out) noexcept {
    u32 vt=0,method=0;
    if(!out || !word(nullptr,weapon,&vt) || vt>0xFFFFFFEFu || !word(nullptr,vt+0x10,&method) || !method)return false;
    using Type=u32(__attribute__((thiscall))*)(u32);
    const u32 dti=reinterpret_cast<Type>(method)(weapon);
    return dti && dti<=0xFFFFFFDFu && word(nullptr,dti+0x1C,out);
}
void scope_activate(void*,u32 unit,u32 weapon,u32 flag) noexcept {
    using Activate=void(__attribute__((thiscall))*)(u32,u32,u32);
    reinterpret_cast<Activate>(0x01BE9C6Fu)(unit,weapon,flag);
}
alignas(rev_runtime::Runtime) unsigned char storage[sizeof(rev_runtime::Runtime)];
bool attempted=false;
}
extern "C" void (*rev_init_begin[])();
extern "C" void (*rev_init_end[])();
extern "C" u32 rev_runtime_initialize() noexcept {
    if(attempted)return 0;
    attempted=true;
    // The offline installer verifies the complete input SHA and every site.
    // These checks additionally reject relocation/wrong mapped architecture.
    u32 mz=0,peoff=0,sig=0,machine=0,magic=0,base=0;
    if(!word(nullptr,0x400000,&mz) || (mz&0xFFFF)!=0x5A4D ||
       !word(nullptr,0x40003C,&peoff) || peoff>0x1000 ||
       !word(nullptr,0x400000+peoff,&sig) || sig!=0x4550 ||
       !word(nullptr,0x400004+peoff,&machine) || (machine&0xFFFF)!=0x14C ||
       !word(nullptr,0x400018+peoff,&magic) || (magic&0xFFFF)!=0x10B ||
       !word(nullptr,0x400034+peoff,&base) || base!=0x400000)return 0;
    for(auto p=rev_init_begin;p!=rev_init_end;++p)if(*p)(*p)();
    accepted=true;
    rev_runtime::Host h{{nullptr,word},write,thread,image,self,rev_hud::january_native_calls()};
    h.action={nullptr,action_member,action_draw};
    h.action_rank=action_rank;
    h.aim_weapon_hidden=aim_weapon_hidden;
    h.weapon_class=weapon_class;
    h.scope_activate=scope_activate;
    h.filter_copy=rev_genesis::january_filter_copy_host(h.memory);
    h.effect_draw=effect_draw;
    auto* r=new(storage) rev_runtime::Runtime(rev_hud::registry(),h,rev_hud::WidgetKinds);
    if(!r->start() || !r->bind()){accepted=false;return 0;}
    return 1;
}
