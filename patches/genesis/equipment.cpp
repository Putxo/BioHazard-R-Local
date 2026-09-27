#include "equipment.hpp"
namespace rev_genesis {
namespace {
bool equal(const Actor& a,const Actor& b) noexcept {
    return a.address==b.address && a.lifetime==b.lifetime &&
        a.serial==b.serial && a.think_mode==b.think_mode;
}
bool valid(const Owner& o) noexcept {
    const auto& s=o.session;
    return s.active && s.epoch && o.actor && o.actor==s.sub0.address &&
        s.self.address && s.self.address!=s.sub0.address &&
        s.self.lifetime && s.sub0.lifetime && s.self.serial>=0 && s.sub0.serial>=0 &&
        s.self.serial!=s.sub0.serial && s.self.think_mode==1 && s.sub0.think_mode==1;
}
bool equal(const Owner& a,const Owner& b) noexcept {
    return a.actor==b.actor && a.session.active==b.session.active &&
        a.session.epoch==b.session.epoch && equal(a.session.self,b.session.self) &&
        equal(a.session.sub0,b.session.sub0);
}
struct Pack {u32 address=0,index=0,slots[15]{};};
bool capture(rev_hud::Reader memory,u32 actor,Pack& p) noexcept {
    if(!memory.word || !actor || actor>rev_hud::Invalid-0x1527)return false;
    auto word=[&](u32 a,u32& v) noexcept {return memory.word(memory.context,a,&v);};
    u32 vt=0,method=0;
    if(!word(actor+0x1524,p.address) || !p.address || p.address>rev_hud::Invalid-0xD7 ||
       !word(p.address,vt) || vt!=0x04D2D42C ||
       !word(vt+0x14,method) || method!=0x01BFECCD ||
       !word(p.address+0xD4,p.index))return false;
    for(u32 i=0;i<15;++i)if(!word(p.address+4+4*i,p.slots[i]))return false;
    return true;
}
bool equal(const Pack& a,const Pack& b) noexcept {
    if(a.address!=b.address || a.index!=b.index)return false;
    for(u32 i=0;i<15;++i)if(a.slots[i]!=b.slots[i])return false;
    return true;
}
}
Mode equipment(ProgressHost host,rev_hud::Reader memory,u32 widget,u32* weapon) noexcept {
    if(!host.resolve)return Mode::Stock;
    Owner owner{};const auto mode=host.resolve(host.context,widget,&owner);
    if(mode==Mode::Stock)return mode;
    if(weapon)*weapon=0;
    if(mode!=Mode::Local || !weapon || !valid(owner))return Mode::Hidden;
    Pack primary{},secondary{},again{};
    if(!capture(memory,owner.session.self.address,primary) ||
       !capture(memory,owner.actor,secondary) ||
       (primary.address<=secondary.address+0xD7 && secondary.address<=primary.address+0xD7))return Mode::Hidden;
    // Stock virtual getter returns null for every index >= 15 (including -1).
    const u32 selected=secondary.index<15?secondary.slots[secondary.index]:0;
    if(selected)for(u32 item:primary.slots)if(item==selected)return Mode::Hidden;
    Owner after{};
    if(host.resolve(host.context,widget,&after)!=Mode::Local || !equal(owner,after) ||
       !capture(memory,owner.session.self.address,again) || !equal(primary,again) ||
       !capture(memory,owner.actor,again) || !equal(secondary,again))return Mode::Hidden;
    *weapon=selected;return Mode::Local;
}
}
