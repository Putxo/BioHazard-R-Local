#include "script_input.hpp"
namespace rev_script {
namespace {
constexpr u32 Max=~u32(0),ManagerSlot=0x05566570;
constexpr u32 InputVT=0x04E15EF4,ManagerVT=0x04E13248;
constexpr u32 MainVT=0x04DCBFAC,SubVT=0x04DCF41C;
InputRouter* Sink=nullptr;
struct Guard {bool& busy;explicit Guard(bool& b):busy(b){busy=true;}~Guard(){busy=false;}};
}
bool InputRouter::read(u32 a,u32& out) const noexcept {
    u32 value=0;
    if(!a || a>Max-3 || !host_.word || !host_.word(host_.context,a,&value))return false;
    out=value;return true;
}
u32 InputRouter::resolve(u32 input,const Owner& owner) const noexcept {
    u32 vt=0,scheduler=0,manager=0,main=0,subs[16]{};
    if(!input || input>Max-0x33 || !read(input,vt) || vt!=InputVT ||
       !read(input+0x30,scheduler) || !scheduler ||
       !read(ManagerSlot,manager) || !manager || manager>Max-0x27F ||
       !read(manager,vt) || vt!=ManagerVT || !read(manager+0x23C,main) ||
       !main || main>Max-(15*0x1070+0xF93))return 0;
    // Main/shared schedulers keep the native member0. The cached uPcs+34 is a
    // script group, not a player number, and is deliberately not used here.
    for(u32 group=0;group<16;++group){
        const u32 entry=main+group*0x1070;u32 found=0;
        if(!read(entry,vt) || vt!=MainVT || !read(entry+0xF90,found) || found==scheduler)return 0;
        if(!read(manager+0x240+group*4,subs[group]) || !subs[group] ||
           subs[group]>Max-(16*0x1080+0x107B))return 0;
    }
    u32 selected=0;
    for(u32 group=0;group<16;++group)for(u32 slot=0;slot<17;++slot){
        const u32 entry=subs[group]+slot*0x1080;u32 found=0,actor=0,serial=0;
        if(!read(entry,vt) || vt!=SubVT || !read(entry+0xF90,found))return 0;
        if(found!=scheduler)continue;
        if(selected || !read(entry+0x1078,actor) || actor!=owner.actor ||
           !read(entry+0x1074,serial) || serial!=owner.serial)return 0;
        selected=entry;
    }
    if(!selected)return 0;
    // No engine callbacks or allocations occur in the scan. Recheck the live
    // anchors before exposing member1; arbitrary foreign-thread writes are not
    // supported by this serialized read-only resolver.
    u32 value=0;
    if(!read(input,vt) || vt!=InputVT || !read(input+0x30,value) || value!=scheduler ||
       !read(ManagerSlot,value) || value!=manager || !read(manager,vt) || vt!=ManagerVT ||
       !read(manager+0x23C,value) || value!=main)return 0;
    for(u32 group=0;group<16;++group)
        if(!read(manager+0x240+group*4,value) || value!=subs[group])return 0;
    if(!read(selected,vt) || vt!=SubVT || !read(selected+0xF90,value) || value!=scheduler ||
       !read(selected+0x1078,value) || value!=owner.actor ||
       !read(selected+0x1074,value) || value!=owner.serial)return 0;
    Owner again{};
    return host_.capture(host_.context,&again) && again.actor==owner.actor &&
        again.serial==owner.serial && again.epoch==owner.epoch ? 1u : 0u;
}
u32 InputRouter::member(u32 input) noexcept {
    if(!host_.on_thread || !host_.on_thread(host_.context) || busy_ || !host_.capture)return 0;
    Guard guard(busy_);Owner owner{};
    if(!host_.capture(host_.context,&owner) || !owner.actor || owner.serial>0x7FFFFFFFu || !owner.epoch)return 0;
    return resolve(input,owner);
}
bool bind(InputRouter& router) noexcept {if(Sink)return false;Sink=&router;return true;}
}
extern "C" unsigned int rev_script_input_member(unsigned int input) noexcept {
    return rev_script::Sink?rev_script::Sink->member(input):0;
}
