#include "router.hpp"
namespace rev_action {
namespace {
constexpr u32 IconVtable=0x04CDCA9C,CommandVtable=0x04CDA850;
constexpr u32 ManagerGlobal=0x0556279C,ManagerVtable=0x04CDBDB4;
Router* sink=nullptr;
bool same_actor(const Actor& a,const Actor& b) noexcept {
    return a.address==b.address && a.lifetime==b.lifetime &&
           a.serial==b.serial && a.think_mode==b.think_mode;
}
bool same(const Frame& a,const Frame& b) noexcept {
    return a.number==b.number && a.session.epoch==b.session.epoch &&
        same_actor(a.session.self,b.session.self) && same_actor(a.session.sub0,b.session.sub0);
}
bool valid(const Frame& f) noexcept {
    const auto& s=f.session;
    return f.number && s.active && s.epoch && s.self.address && s.sub0.address &&
        s.self.address!=s.sub0.address && s.self.lifetime && s.sub0.lifetime &&
        s.self.serial>=0 && s.sub0.serial>=0 && s.self.serial!=s.sub0.serial &&
        s.self.think_mode==1 && s.sub0.think_mode==1;
}
struct Busy {bool& flag;explicit Busy(bool& f):flag(f){flag=true;}~Busy(){flag=false;}};
}
bool Router::ready() const noexcept {
    return host_.memory.word && host_.write && host_.snapshot &&
        host_.calls.member && host_.calls.draw;
}
bool Router::word(u32 a,u32& v) const noexcept {
    return a && a<=Invalid-3 && host_.memory.word &&
        host_.memory.word(host_.memory.context,a,&v);
}
bool Router::manager(u32& out) const noexcept {
    u32 vt=0;
    return word(ManagerGlobal,out) && out && out<=Invalid-0x177 &&
        word(out,vt) && vt==ManagerVtable;
}
Result Router::draw(u32 icon,u32 context) noexcept {
    if(!ready())return Result::Refused;
    Frame frame{};const auto mode=host_.snapshot(host_.memory.context,&frame);
    if(mode==SnapshotMode::Stock){
        host_.calls.draw(host_.calls.context,icon,context);return Result::Stock;
    }
    if(mode!=SnapshotMode::Local || !valid(frame) || !icon || icon>Invalid-0x43 ||
       !context || context>Invalid-0x15B)return Result::Refused;
    // A Local snapshot certifies the owner thread before mutable router state.
    if(busy_ || faulted_)return Result::Refused;
    Busy lock(busy_);
    u32 vt=0,command=0,view=0,owner=0,check=0;
    if(!word(icon,vt) || vt!=IconVtable || !word(icon+0x40,command) ||
       !command || command>Invalid-0x3F || !word(command,vt) || vt!=CommandVtable ||
       !word(context+0x158,view) || !manager(owner))return Result::Refused;
    view&=255;
    if(view>1)return Result::Skipped;
    // Check all delegate words before the audited native member accessor.
    for(u32 off=0x34;off<=0x3C;off+=4)if(!word(command+off,check))return Result::Refused;
    const u32 member=host_.calls.member(host_.calls.context,command);
    Frame after{};
    if(host_.snapshot(host_.memory.context,&after)!=SnapshotMode::Local ||
       !valid(after) || !same(frame,after) || !word(icon+0x40,check) || check!=command)
        return Result::Refused;
    if(member>1 || member!=view)return Result::Skipped;
    u32 current_manager=0;
    if(!manager(current_manager) || current_manager!=owner)return Result::Refused;
    if(!same(current_,frame) || manager_!=owner){
        current_=frame;manager_=owner;claims_[0]=claims_[1]=0;
    }
    u32 saved=0;
    if(!word(owner+0x174,saved))return Result::Refused;
    const u32 scoped=(saved&~255u)|claims_[view];
    if(!host_.write(host_.memory.context,owner+0x174,scoped))return Result::Refused;
    host_.calls.draw(host_.calls.context,icon,context);
    u32 now=0;
    // Do not overwrite a replacement manager or restore neighboring flag bytes.
    if(!manager(current_manager) || current_manager!=owner || !word(owner+0x174,now) ||
       !host_.write(host_.memory.context,owner+0x174,(now&~255u)|(saved&255u))){
        faulted_=true;return Result::RestoreFailed;
    }
    claims_[view]=now&255u;
    return Result::Drawn;
}
u32 Router::mask(u32 unit,u32 original) noexcept {
    u32 vt=0;
    if(!word(unit,vt) || vt!=IconVtable)return original;
    if(!ready())return 0;
    Frame frame{};const auto mode=host_.snapshot(host_.memory.context,&frame);
    if(mode==SnapshotMode::Stock)return original;
    if(mode!=SnapshotMode::Local || !valid(frame))return 0;
    if(busy_ || faulted_)return 0;
    Busy lock(busy_);
    // Preserve explicit hiding and masks restricted to auxiliary views.
    if(!(original&3))return original;
    u32 command=0,check=0;
    if(unit>Invalid-0x43 || !word(unit+0x40,command) || !command ||
       command>Invalid-0x3F || !word(command,vt) || vt!=CommandVtable)return 0;
    for(u32 off=0x34;off<=0x3C;off+=4)if(!word(command+off,check))return 0;
    const u32 member=host_.calls.member(host_.calls.context,command);
    Frame after{};
    if(member>1 || host_.snapshot(host_.memory.context,&after)!=SnapshotMode::Local ||
       !valid(after) || !same(frame,after) || !word(unit+0x40,check) || check!=command)return 0;
    return (original&~3u)|(1u<<member);
}
bool bind(Router& r) noexcept {
    if(sink || !r.ready())return false;
    sink=&r;return true;
}
}
extern "C" void rev_action_draw(unsigned int icon,unsigned int context) noexcept {
    if(rev_action::sink)(void)rev_action::sink->draw(icon,context);
}
extern "C" unsigned int rev_action_mask(unsigned int unit,unsigned int original) noexcept {
    return rev_action::sink?rev_action::sink->mask(unit,original):original;
}
