#include "draw_schedule.hpp"
namespace rev_runtime {
namespace {rev_action::Host schedule_host{};bool bound=false;}
unsigned int choose_draw_schedule(const rev_action::Host& h,unsigned int context,unsigned int stock) noexcept {
    using namespace rev_action;
    stock&=255;
    if(stock || !h.memory.word || !h.snapshot || !context || context>Invalid-0x15B)return stock;
    Frame f{};
    if(h.snapshot(h.memory.context,&f)!=SnapshotMode::Local)return stock;
    const auto& s=f.session;
    if(!f.number||!s.active||!s.epoch||!s.self.address||!s.sub0.address||
       s.self.address==s.sub0.address||!s.self.lifetime||!s.sub0.lifetime||
       s.self.serial<0||s.sub0.serial<0||s.self.serial==s.sub0.serial||
       s.self.think_mode!=1||s.sub0.think_mode!=1)return stock;
    u32 vt=0,view=0;
    if(!h.memory.word(h.memory.context,context,&vt)||vt!=0x04F79014||
       !h.memory.word(h.memory.context,context+0x158,&view)||(view&255)>1)return stock;
    return 1;
}
bool bind_draw_schedule(rev_action::Host h) noexcept {
    if(bound||!h.memory.word||!h.snapshot)return false;
    schedule_host=h;bound=true;return true;
}
}
extern "C" unsigned int rev_runtime_draw_schedule(unsigned int context,unsigned int stock) noexcept {
    return rev_runtime::choose_draw_schedule(rev_runtime::schedule_host,context,stock);
}
