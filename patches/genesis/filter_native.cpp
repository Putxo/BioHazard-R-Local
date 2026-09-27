#include "filter_resource.hpp"
namespace rev_genesis {
#if defined(__i386__)
namespace {
using NoArg=u32(__attribute__((thiscall))*)(u32);
using StreamCtor=u32(__attribute__((thiscall))*)(u32,u32,u32,u32);
using Transfer=unsigned char(__attribute__((thiscall))*)(u32,u32);
// Fixed-capacity borrowed stream: native write reports overflow; it cannot
// grow/free this buffer. Used only at the serialized initialization boundary.
alignas(16) unsigned char buffer[256*1024];
bool copying=false;
u32 create(void*) noexcept {return reinterpret_cast<NoArg>(0x02929680u)(0x0558CE40u);}
void release(void*,u32 resource) noexcept {reinterpret_cast<NoArg>(0x01C3C159u)(resource);}
bool transfer(void*,u32 source,u32 destination) noexcept {
    if(copying)return false;
    copying=true;
    alignas(16) u32 stream[7]{};
    const u32 address=reinterpret_cast<u32>(stream);
    reinterpret_cast<StreamCtor>(0x02FD8B90u)(address,reinterpret_cast<u32>(buffer),sizeof(buffer),3);
    bool ok=reinterpret_cast<Transfer>(0x01C4B4F1u)(source,address)!=0;
    const u32 bytes=stream[2];
    ok=ok && !(stream[5]&1) && bytes && bytes<=sizeof(buffer);
    if(ok) {
        stream[2]=0;stream[3]=bytes;
        ok=reinterpret_cast<Transfer>(0x01C12E2Bu)(destination,address)!=0;
        ok=ok && !(stream[5]&1) && stream[2]<=bytes;
    }
    reinterpret_cast<NoArg>(0x02FD8C20u)(address);
    copying=false;
    return ok;
}
}
FilterCopyHost january_filter_copy_host(Reader r) noexcept {return {r,nullptr,create,transfer,release};}
#else
FilterCopyHost january_filter_copy_host(Reader) noexcept {return {};}
#endif
}
