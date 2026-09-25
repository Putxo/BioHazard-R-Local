#include "january_backend.hpp"
using rev_hud::u32;
extern "C" {
extern u32 abi_kind,abi_self,abi_arg,abi_extra,abi_bad_alignment;
void abi_global();void abi_allocate();void abi_member0();void abi_member1();
}
static u32 address(void(*p)()) { return reinterpret_cast<u32>(p); }
extern "C" int january_abi_cases() {
    auto c=rev_hud::january_native_calls();
    if(!c.allocate || !c.construct || !c.method0 || !c.method1 || !c.singleton || !c.contains)return 1;
    const u32 values[]={1,0x12345678,0x80000000,0xFFFFFFFF,0x00500000};
    for(u32 v:values) {
        if(c.singleton(nullptr,address(abi_global))!=0x13579BDF || abi_kind!=0)return 2;
        if(c.allocate(nullptr,address(abi_allocate),v,16)!=(v^16) || abi_kind!=2 || abi_arg!=v || abi_extra!=16)return 3;
        if(c.construct(nullptr,address(abi_member0),v)!=v || abi_kind!=3 || abi_self!=v)return 4;
        c.method0(nullptr,address(abi_member0),v);
        if(abi_kind!=3 || abi_self!=v)return 5;
        c.method1(nullptr,address(abi_member1),v,0xDEAD1111);
        if(abi_kind!=4 || abi_self!=v || abi_arg!=0xDEAD1111)return 6;
        if(c.contains(nullptr,address(abi_member1),v,0x6789ABCD)!=(v^0x6789ABCD) || abi_kind!=4 || abi_arg!=0x6789ABCD)return 7;
    }
    return abi_bad_alignment?8:0;
}
