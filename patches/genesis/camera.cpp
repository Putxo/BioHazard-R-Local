#include "camera.hpp"
namespace rev_genesis {
namespace {
bool equal(const Actor& a,const Actor& b) noexcept {
    return a.address==b.address && a.lifetime==b.lifetime &&
        a.serial==b.serial && a.think_mode==b.think_mode;
}
bool equal(const Owner& a,const Owner& b) noexcept {
    return a.actor==b.actor && a.session.active==b.session.active &&
        a.session.epoch==b.session.epoch && equal(a.session.self,b.session.self) &&
        equal(a.session.sub0,b.session.sub0);
}
bool valid(const Owner& o) noexcept {
    const auto& s=o.session;
    return s.active && s.epoch && o.actor && o.actor==s.sub0.address &&
        s.self.address && s.self.address!=s.sub0.address &&
        s.self.lifetime && s.sub0.lifetime && s.self.serial>=0 && s.sub0.serial>=0 &&
        s.self.serial!=s.sub0.serial && s.self.think_mode==1 && s.sub0.think_mode==1;
}
}
Mode camera(ProgressHost host,rev_hud::Reader memory,u32 widget,u32 manager,u32* out) noexcept {
    if(!host.resolve)return Mode::Stock;
    Owner owner{};const Mode mode=host.resolve(host.context,widget,&owner);
    if(mode==Mode::Stock)return mode;
    if(out)*out=0;
    if(mode!=Mode::Local || !out || !memory.word || !valid(owner) ||
       !manager || manager>rev_hud::Invalid-0xCE7)return Mode::Hidden;
    auto word=[&](u32 a,u32& v) noexcept {
        return a && a<=rev_hud::Invalid-3 && memory.word(memory.context,a,&v);
    };
    u32 global=0,primary=0,secondary=0,vt=0,target=0;
    if(!word(0x05799D3C,global) || global!=manager ||
       !word(manager+0xCE0,primary) || !primary ||
       !word(manager+0xCE4,secondary) || !secondary || secondary==primary ||
       secondary>rev_hud::Invalid-0x77 || !word(secondary,vt) || vt!=0x04CF2B1C ||
       !word(secondary+0x74,target) || target!=owner.actor)return Mode::Hidden;
    Owner after{};u32 again=0;
    if(host.resolve(host.context,widget,&after)!=Mode::Local || !equal(owner,after) ||
       !word(0x05799D3C,again) || again!=manager ||
       !word(manager+0xCE0,again) || again!=primary ||
       !word(manager+0xCE4,again) || again!=secondary ||
       !word(secondary,again) || again!=vt ||
       !word(secondary+0x74,again) || again!=target)return Mode::Hidden;
    *out=secondary;return Mode::Local;
}
}
