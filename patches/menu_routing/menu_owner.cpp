#include "menu_owner.hpp"
namespace rev_menu {
namespace {
MenuOwnerRouter* Sink=nullptr;
constexpr u32 PadStride=0x2F8, StartPad=0x970;
constexpr u32 State198=0x198, State1A0=0x1A0;
constexpr u32 ActorSerial=0xE3C, ThinkMode=0xE40;
}
bool MenuOwnerRouter::read(u32 address,u32& out) const noexcept {
    u32 value=0;
    if(!address || address>Invalid-3 || !access_.word ||
       !access_.word(access_.context,address,&value)) return false;
    out=value; return true;
}
bool MenuOwnerRouter::valid_sub(u32& out) const noexcept {
    out=0;
    if(!access_.local_active || !access_.sub0 ||
       !access_.local_active(access_.context)) return false;
    const u32 sub=access_.sub0(access_.context);
    u32 mode=0,serial=0;
    if(!sub || !read(sub+ThinkMode,mode) || mode!=1 ||
       !read(sub+ActorSerial,serial) || serial>127) return false;
    out=sub; return true;
}
bool MenuOwnerRouter::pad_word(u32 pad,u32 member,u32 offset,u32& out) const noexcept {
    if(!pad || member>1 || offset>Invalid-member*PadStride ||
       pad>Invalid-(member*PadStride+offset)-3) return false;
    return read(pad+member*PadStride+offset,out);
}
u32 MenuOwnerRouter::stock_word(u32 pad,u32 offset) const noexcept {
    u32 member=0,value=0;
    if(!pad || !read(pad+StartPad,member) || member>1 ||
       !pad_word(pad,member,offset,value)) return 0;
    return value;
}
u32 MenuOwnerRouter::open_word(u32 pad,u32 mask,Surface surface) noexcept {
    const u32 stock=stock_word(pad,State1A0);
    u32 sub=0;
    if(!valid_sub(sub)) {
        owner_=0; surface_=Surface::None;
        return stock;
    }
    // Local-coop gameplay already routes Self to logical Pad 0 and Sub0 to
    // logical Pad 1. Preserve that same mapping here; P1 wins simultaneous
    // presses so J2 can never steal an opening that P1 also requested.
    u32 p0=0,p1=0;
    if(!pad_word(pad,0,State1A0,p0) || !pad_word(pad,1,State1A0,p1)) {
        owner_=0; surface_=Surface::None;
        return stock;
    }
    if(p0 & mask) {
        owner_=0; surface_=surface; return p0;
    }
    if(p1 & mask) {
        owner_=1; surface_=surface; return p1;
    }
    owner_=0; surface_=Surface::None;
    return p0;
}
u32 MenuOwnerRouter::pause_open_word(u32 pad) noexcept {
    return open_word(pad,0x8,Surface::Pause);
}
u32 MenuOwnerRouter::submenu_open_word(u32 pad) noexcept {
    return open_word(pad,0x1,Surface::SubMenu);
}
u32 MenuOwnerRouter::submenu_word(u32 pad,u32 offset) noexcept {
    u32 sub=0,value=0;
    if(surface_==Surface::SubMenu && owner_==1 && valid_sub(sub) &&
       pad_word(pad,1,offset,value)) return value;
    return stock_word(pad,offset);
}
u32 MenuOwnerRouter::submenu_word_198(u32 pad) noexcept {
    return submenu_word(pad,State198);
}
u32 MenuOwnerRouter::submenu_word_1a0(u32 pad) noexcept {
    return submenu_word(pad,State1A0);
}
u32 MenuOwnerRouter::submenu_actor(u32 stock) noexcept {
    u32 sub=0;
    return surface_==Surface::SubMenu && owner_==1 && valid_sub(sub) ? sub : stock;
}
bool bind_menu_owner_sink(MenuOwnerRouter& router) noexcept {
    if(Sink) return false;
    Sink=&router; return true;
}
}
extern "C" unsigned int rev_menu_pause_open_word(unsigned int pad) noexcept {
    return rev_menu::Sink ? rev_menu::Sink->pause_open_word(pad) : 0;
}
extern "C" unsigned int rev_menu_submenu_open_word(unsigned int pad) noexcept {
    return rev_menu::Sink ? rev_menu::Sink->submenu_open_word(pad) : 0;
}
extern "C" unsigned int rev_menu_submenu_word_198(unsigned int pad) noexcept {
    return rev_menu::Sink ? rev_menu::Sink->submenu_word_198(pad) : 0;
}
extern "C" unsigned int rev_menu_submenu_word_1a0(unsigned int pad) noexcept {
    return rev_menu::Sink ? rev_menu::Sink->submenu_word_1a0(pad) : 0;
}
extern "C" unsigned int rev_menu_submenu_actor(unsigned int stock) noexcept {
    return rev_menu::Sink ? rev_menu::Sink->submenu_actor(stock) : stock;
}
