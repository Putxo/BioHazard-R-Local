#include "january_backend.hpp"
#if defined(__i386__) || defined(_M_IX86)
extern "C" unsigned int rev_january_cdecl0(unsigned int);
extern "C" unsigned int rev_january_cdecl2(unsigned int,unsigned int,unsigned int);
extern "C" unsigned int rev_january_this0(unsigned int,unsigned int);
extern "C" unsigned int rev_january_this1(unsigned int,unsigned int,unsigned int);
#endif
namespace rev_hud {
JanuaryCalls january_native_calls() {
    JanuaryCalls c{};
#if defined(__i386__) || defined(_M_IX86)
    c.allocate=[](void*,u32 f,u32 n,u32 a) noexcept {return rev_january_cdecl2(f,n,a);};
    c.construct=[](void*,u32 f,u32 s) noexcept {return rev_january_this0(f,s);};
    c.method0=[](void*,u32 f,u32 s) noexcept { (void)rev_january_this0(f,s); };
    c.method1=[](void*,u32 f,u32 s,u32 a) noexcept { (void)rev_january_this1(f,s,a); };
    c.singleton=[](void*,u32 f) noexcept {return rev_january_cdecl0(f);};
    c.contains=[](void*,u32 f,u32 s,u32 a) noexcept {return rev_january_this1(f,s,a);};
#endif
    return c;
}
}
