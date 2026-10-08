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
bool MenuOwnerRouter::valid_sub(u32& out,u32* serial_out) const noexcept {
    out=0;
    if(serial_out) *serial_out=Invalid;
    if(!access_.local_active || !access_.sub0 ||
       !access_.local_active(access_.context)) return false;
    const u32 sub=access_.sub0(access_.context);
    u32 mode=0,serial=0;
    if(!sub || sub>Invalid-ThinkMode-3 || !read(sub+ThinkMode,mode) || mode!=1 ||
       !read(sub+ActorSerial,serial) || serial>127) return false;
    out=sub;
    if(serial_out) *serial_out=serial;
    return true;
}
bool MenuOwnerRouter::bound_sub_valid() const noexcept {
    if(owner_!=1 || !owner_actor_ || owner_serial_==Invalid) return false;
    u32 sub=0,serial=Invalid;
    return valid_sub(sub,&serial) && sub==owner_actor_ && serial==owner_serial_;
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
    if(!valid_sub(sub,&serial)) {
        clear();
        return stock;
    }
    u32 p0=0,p1=0;
    if(!pad_word(pad,0,State1A0,p0) || !pad_word(pad,1,State1A0,p1)) {
        clear();
        return stock;
    }
    if(p0 & mask) {
        options_valid_=false;
        owner_=0; surface_=surface;
        owner_actor_=0; owner_serial_=Invalid;
        return p0;
    }
    if(p1 & mask) {
        options_valid_=false;
        owner_=1; surface_=surface;
        owner_actor_=sub; owner_serial_=serial;
        return p1;
    }
    clear();
    return p0;
}
u32 MenuOwnerRouter::owner_word(u32 pad,Surface surface,u32 offset) noexcept {
    u32 value=0;
    // Any routed read may observe invalidation, even on a different surface.
    // Revoke immediately so restoring the actor cannot resurrect ownership.
    if(owner_==1 && !bound_sub_valid()) {
        clear();
        return stock_word(pad,offset);
    }
    if(surface_!=surface) return stock_word(pad,offset);
    if(owner_==0) {
        // Local opening selected logical pad0, even if the stock primary is 1.
        u32 sub=0;
        if(valid_sub(sub) && pad_word(pad,0,offset,value)) return value;
        return stock_word(pad,offset);
    }
    if(pad_word(pad,1,offset,value)) return value;
    clear();
    return stock_word(pad,offset);
}
u32 MenuOwnerRouter::pause_open_word(u32 pad) noexcept { return open_word(pad,0x8,Surface::Pause); }
u32 MenuOwnerRouter::pause_word_198(u32 pad) noexcept { return owner_word(pad,Surface::Pause,State198); }
u32 MenuOwnerRouter::pause_word_1a0(u32 pad) noexcept { return owner_word(pad,Surface::Pause,State1A0); }
u32 MenuOwnerRouter::pause_word_1ac(u32 pad) noexcept { return owner_word(pad,Surface::Pause,State1AC); }
u32 MenuOwnerRouter::submenu_open_word(u32 pad) noexcept { return open_word(pad,0x1,Surface::SubMenu); }
u32 MenuOwnerRouter::submenu_word_198(u32 pad) noexcept { return owner_word(pad,Surface::SubMenu,State198); }
u32 MenuOwnerRouter::submenu_word_1a0(u32 pad) noexcept { return owner_word(pad,Surface::SubMenu,State1A0); }
u32 MenuOwnerRouter::submenu_actor(u32 stock) noexcept {
    if(owner_==1 && !bound_sub_valid()) {
        clear();
        return stock;
    }
    if(surface_!=Surface::SubMenu || owner_!=1) return stock;
    return owner_actor_;
}
void MenuOwnerRouter::state_transition(u32 next_state) noexcept {
    if(surface_==Surface::None) return;
    if(owner_==1 && !bound_sub_valid()) {
        clear();
        return;
    }
    if(next_state==6 || next_state==7) return;
    if(next_state==5 && surface_==Surface::Pause) return;
    if(next_state==8 && surface_==Surface::SubMenu) return;
    clear();
}
bool MenuOwnerRouter::options_current() const noexcept {
    const auto& a=access_.options;
    u32 actor=0,serial=Invalid,pad=0,vt=0;
    return options_valid_ && options_manager_ && options_member_<2 &&
        a.thread && a.thread(a.context)==options_thread_ && surface_==Surface::Pause &&
        owner_==options_member_ && valid_sub(actor,&serial) && actor==options_actor_ &&
        serial==options_serial_ && (owner_!=1 || bound_sub_valid()) &&
        read(options_manager_,vt) && vt==0x04DF5AE4 &&
        read(0x057A7480,pad) && pad==options_pad_ && read(pad,vt) && vt==0x04E11D30;
}
bool MenuOwnerRouter::options_values(u32* out) const noexcept {
    if(!out || !options_current() || options_pad_>Invalid-0x82B)return false;
    const u32 data=options_pad_+0x668+options_member_*0xC0;
    const u32 offsets[]={0x10,0xC,0x18,0x20,0x1C};
    for(u32 i=0;i<5;++i)if(!read(data+offsets[i],out[i]))return false;
    if(out[0]==6 && !read(data+0x14,out[0]))return false;
    return options_current();
}
bool MenuOwnerRouter::options_store(u32 address,u32 value) noexcept {
    const auto& a=access_.options;
    return options_current() && a.write && a.write(a.context,address,value) && options_current();
}
void MenuOwnerRouter::options_read(u32 config,u32 reference) noexcept {
    auto& a=access_.options;
    if(options_busy_ || !a.read_native)return;
    if(reference==0){
        options_valid_=false;options_reference_=false;options_member_=Invalid;
        options_manager_=config>=0x4C?config-0x4C:0;
        u32 sub=0,serial=Invalid,vt=0,pad=0;
        const bool local=surface_==Surface::Pause;
        if(local){
            // Keep a local tombstone even if later validation/read fails: never
            // apply that dialog through the primary-player fallback.
            options_member_=owner_;
            if(valid_sub(sub,&serial) && a.thread && a.write && a.apply_native && a.publish_native &&
               options_manager_ && options_manager_<=Invalid-0x254 &&
               read(options_manager_,vt) && vt==0x04DF5AE4 &&
               read(0x057A7480,pad) && pad && pad<=Invalid-0x994){
                options_actor_=sub;options_serial_=serial;
                options_pad_=pad;options_thread_=a.thread(a.context);
                options_valid_=options_thread_!=0;
            }
        }
    }
    if(options_member_!=Invalid &&
       (reference>1 || config!=options_manager_+(reference?0x150:0x4C) || !options_current())){
        options_valid_=false;return;
    }
    options_busy_=true;
    a.read_native(a.context,config);
    options_busy_=false;
    if(options_member_==Invalid)return;
    if(reference>1 || config!=options_manager_+(reference?0x150:0x4C)){
        options_valid_=false;return;
    }
    u32 values[5]{};
    if(!options_values(values)){options_valid_=false;return;}
    for(u32 i=0;i<5;++i)if(!options_store(config+4+i*4,values[i])){
        options_valid_=false;return;
    }
    if(reference)options_reference_=true;
}
u32 MenuOwnerRouter::options_index(u32 pad,u32 stock) const noexcept {
    const auto& a=access_.options;
    // The synchronous native apply owns this pin. Revocation blocks subsequent
    // transactions; it must not redirect the remainder of this one into J1.
    return options_scope_ && options_busy_ && options_member_<2 && pad==options_pad_ &&
        a.thread && a.thread(a.context)==options_thread_ ? options_member_ : stock;
}
u32 MenuOwnerRouter::options_apply(u32 config,u32 save,u32 flag) noexcept {
    auto& a=access_.options;
    if(options_busy_ || !a.apply_native)return 0;
    if(options_member_==Invalid){
        u32 sub=0;
        if(surface_==Surface::Pause && valid_sub(sub))return 0;
        return a.apply_native(a.context,config,save,flag);
    }
    if(!options_reference_ || config!=options_manager_+0x4C || !options_current()){
        options_valid_=false;return 0;
    }
    u32 copy[65]{},global=0;
    const bool publish=(save&0xFF)==1; // Native compares the low byte to exactly 1.
    if(publish){
        if(!read(0x055926F4,global) || (global && global>Invalid-0x140))return 0;
        if(global){
            for(u32 i=0;i<65;++i)if(!read(config+i*4,copy[i]))return 0;
            if(options_member_==1){
                // Shared video/audio/etc can still be applied, but J2's five
                // controller values must never replace J1's global profile.
                for(u32 i=1;i<=5;++i)if(!read(global+0x3C+i*4,copy[i]))return 0;
            }
        }
    }
    if(!options_current()){options_valid_=false;return 0;}
    options_busy_=true;
    if(publish && global)a.publish_native(a.context,global,copy);
    if(!options_current()){options_busy_=false;options_valid_=false;return 0;}
    options_scope_=true;
    const u32 result=a.apply_native(a.context,config,0,flag);
    options_scope_=false;options_busy_=false;
    if(!options_current())options_valid_=false;
    return result;
}
bool bind_menu_owner_sink(MenuOwnerRouter& router) noexcept {
    if(Sink) return false;
    Sink=&router; return true;
}
}
extern "C" unsigned int rev_menu_pause_open_word(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->pause_open_word(pad) : 0; }
extern "C" unsigned int rev_menu_pause_word_198(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->pause_word_198(pad) : 0; }
extern "C" unsigned int rev_menu_pause_word_1a0(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->pause_word_1a0(pad) : 0; }
extern "C" unsigned int rev_menu_pause_word_1ac(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->pause_word_1ac(pad) : 0; }
extern "C" unsigned int rev_menu_submenu_open_word(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->submenu_open_word(pad) : 0; }
extern "C" unsigned int rev_menu_submenu_word_198(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->submenu_word_198(pad) : 0; }
extern "C" unsigned int rev_menu_submenu_word_1a0(unsigned int pad) noexcept { return rev_menu::Sink ? rev_menu::Sink->submenu_word_1a0(pad) : 0; }
extern "C" unsigned int rev_menu_submenu_actor(unsigned int stock) noexcept { return rev_menu::Sink ? rev_menu::Sink->submenu_actor(stock) : stock; }
extern "C" void rev_menu_state_transition(unsigned int next_state) noexcept { if(rev_menu::Sink) rev_menu::Sink->state_transition(next_state); }

extern "C" void rev_menu_options_read(unsigned int config,unsigned int reference) noexcept {
    if(rev_menu::Sink)rev_menu::Sink->options_read(config,reference);
}
extern "C" unsigned int rev_menu_options_apply(unsigned int config,unsigned int save,unsigned int flag) noexcept {
    return rev_menu::Sink?rev_menu::Sink->options_apply(config,save,flag):0;
}
extern "C" unsigned int rev_menu_options_index(unsigned int gamepad,unsigned int stock) noexcept {
    return rev_menu::Sink?rev_menu::Sink->options_index(gamepad,stock):stock;
}
