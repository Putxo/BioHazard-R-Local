#include "menu_owner.hpp"
namespace rev_menu {
namespace {
MenuOwnerRouter* Sink=nullptr;
constexpr u32 PadStride=0x2F8, StartPad=0x970;
constexpr u32 State198=0x198, State1A0=0x1A0, State1AC=0x1AC;
constexpr u32 ActorSerial=0xE3C, ThinkMode=0xE40;
}
bool MenuOwnerRouter::read(u32 address,u32& out) const noexcept {
    u32 value=0;
    if(!address || address>Invalid-3 || !access_.word ||
       !access_.word(access_.context,address,&value)) return false;
    out=value; return true;
}
bool MenuOwnerRouter::valid_sub(u32& out,u32& serial) const noexcept {
    out=0; serial=Invalid;
    if(!access_.local_active || !access_.sub0 ||
       !access_.local_active(access_.context)) return false;
    const u32 sub=access_.sub0(access_.context);
    u32 mode=0;
    if(!sub || sub>Invalid-ThinkMode-3 || !read(sub+ThinkMode,mode) || mode!=1 ||
       !read(sub+ActorSerial,serial) || serial>127) return false;
    out=sub; return true;
}
bool MenuOwnerRouter::validate_owner() noexcept {
    if(surface_==Surface::None) return false;
    u32 sub=0,serial=Invalid;
    if(!valid_sub(sub,serial) || sub!=actor_ || serial!=serial_) {
        clear(); return false;
    }
    return true;
}
void MenuOwnerRouter::observe_state(u32 cockpit,u32 state) noexcept {
    if(!cockpit || (cockpit_ && cockpit_!=cockpit)) clear();
    cockpit_=cockpit;
    if(!cockpit || state!=static_cast<u32>(surface_)) {
        clear(); return;
    }
    (void)validate_owner();
}
bool MenuOwnerRouter::pad_word(u32 pad,u32 member,u32 offset,u32& out) const noexcept {
    if(!pad || member>1 || offset>Invalid-member*PadStride ||
       pad>Invalid-(member*PadStride+offset)-3) return false;
    return read(pad+member*PadStride+offset,out);
}
u32 MenuOwnerRouter::stock_word(u32 pad,u32 offset) const noexcept {
    u32 member=0,value=0;
    if(!pad || pad>Invalid-StartPad-3 || !read(pad+StartPad,member) || member>1 ||
       !pad_word(pad,member,offset,value)) return 0;
    return value;
}
u32 MenuOwnerRouter::open_word(u32 pad,u32 mask,Surface surface) noexcept {
    const u32 stock=stock_word(pad,State1A0);
    u32 sub=0,serial=Invalid;
    if(!valid_sub(sub,serial)) {
        clear();
        return stock;
    }
    u32 p0=0,p1=0;
    if(!pad_word(pad,0,State1A0,p0) || !pad_word(pad,1,State1A0,p1)) {
        clear();
        return stock;
    }
    if(p0 & mask) {
        actor_=sub; serial_=serial;
        owner_=0; surface_=surface; return p0;
    }
    if(p1 & mask) {
        actor_=sub; serial_=serial;
        owner_=1; surface_=surface; return p1;
    }
    clear();
    return p0;
}
u32 MenuOwnerRouter::owner_word(u32 pad,Surface surface,u32 offset) noexcept {
    u32 value=0;
    // Validate even across surfaces so a stale owner cannot revive later.
    if(validate_owner() && surface_==surface) {
        if(pad_word(pad,owner_,offset,value)) return value;
        clear();
    }
    return stock_word(pad,offset);
}
u32 MenuOwnerRouter::pause_open_word(u32 pad) noexcept { return open_word(pad,0x8,Surface::Pause); }
u32 MenuOwnerRouter::pause_word_1a0(u32 pad) noexcept { return owner_word(pad,Surface::Pause,State1A0); }
u32 MenuOwnerRouter::pause_word_1ac(u32 pad) noexcept { return owner_word(pad,Surface::Pause,State1AC); }
u32 MenuOwnerRouter::submenu_open_word(u32 pad) noexcept { return open_word(pad,0x1,Surface::SubMenu); }
u32 MenuOwnerRouter::submenu_word_198(u32 pad) noexcept { return owner_word(pad,Surface::SubMenu,State198); }
u32 MenuOwnerRouter::submenu_word_1a0(u32 pad) noexcept { return owner_word(pad,Surface::SubMenu,State1A0); }
u32 MenuOwnerRouter::submenu_actor(u32 stock) noexcept {
    return validate_owner() && surface_==Surface::SubMenu && owner_==1 ? actor_ : stock;
}
bool bind_menu_owner_sink(MenuOwnerRouter& router) noexcept {
    if(Sink) return false;
    Sink=&router; return true;
}
}
extern "C" unsigned int rev_menu_pause_open_word(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->pause_open_word(pad) : 0; }
extern "C" unsigned int rev_menu_pause_word_1a0(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->pause_word_1a0(pad) : 0; }
extern "C" unsigned int rev_menu_pause_word_1ac(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->pause_word_1ac(pad) : 0; }
extern "C" unsigned int rev_menu_submenu_open_word(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->submenu_open_word(pad) : 0; }
extern "C" unsigned int rev_menu_submenu_word_198(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->submenu_word_198(pad) : 0; }
extern "C" unsigned int rev_menu_submenu_word_1a0(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->submenu_word_1a0(pad) : 0; }
extern "C" unsigned int rev_menu_submenu_actor(unsigned int stock) noexcept { return rev_menu::Sink ? rev_menu::Sink->submenu_actor(stock) : stock; }
extern "C" void rev_menu_observe_state(unsigned int cockpit,unsigned int state) noexcept {
    if(rev_menu::Sink) rev_menu::Sink->observe_state(cockpit,state);
}
extern "C" void rev_menu_reset_session() noexcept {
    if(rev_menu::Sink) rev_menu::Sink->reset_session();
}
