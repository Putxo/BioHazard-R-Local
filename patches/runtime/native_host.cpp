#include "runtime.hpp"
#if !defined(__i386__)
#error This native adapter requires the January PE32 x86 image.
#endif
using rev_hud::u32;
using usize=__SIZE_TYPE__;
void* operator new(usize,void* p) noexcept {return p;}
extern "C" {
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
    auto* r=new(storage) rev_runtime::Runtime(rev_hud::registry(),h);
    if(!r->start() || !r->bind()){accepted=false;return 0;}
    return 1;
}
