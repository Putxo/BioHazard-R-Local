#include "heal_queue.hpp"
namespace rev_hud {
namespace {
bool same(const Actor& a,const Actor& b) noexcept {
    return a.address==b.address && a.lifetime==b.lifetime &&
        a.serial==b.serial && a.think_mode==b.think_mode;
}
}
bool HealQueue::session_valid(const Session& s) noexcept {
    return s.active && s.epoch && s.self.address && s.sub0.address &&
        s.self.lifetime && s.sub0.lifetime && s.self.serial>=0 && s.sub0.serial>=0 &&
        s.self.think_mode==1 && s.sub0.think_mode==1 &&
        s.self.address!=s.sub0.address && s.self.serial!=s.sub0.serial;
}
bool HealQueue::request(const Session& s,u32 frame,u32 actor) noexcept {
    if(!session_valid(s) || !frame || actor!=s.sub0.address)return false;
    session_=s;frame_=frame;pending_=true;return true;
}
bool HealQueue::consume(const Session& s,u32 frame,u32 actor) noexcept {
    if(!pending_)return false;
    if(!session_valid(s) || s.epoch!=session_.epoch || !same(s.self,session_.self) ||
        !same(s.sub0,session_.sub0)) {clear();return false;}
    if(!frame || frame<frame_ || actor!=s.sub0.address)return false;
    const bool fresh=frame-frame_<=1;clear();return fresh;
}
}
