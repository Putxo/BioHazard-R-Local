#include "draw_schedule.hpp"
namespace rev_runtime {
namespace {
rev_action::Host schedule_host{};bool bound=false;
bool valid_frame(const rev_action::Frame& f) noexcept {
    const auto& s=f.session;
    return f.number && s.active && s.epoch && s.self.address && s.sub0.address &&
        s.self.address!=s.sub0.address && s.self.lifetime && s.sub0.lifetime &&
        s.self.serial>=0 && s.sub0.serial>=0 && s.self.serial!=s.sub0.serial &&
        s.self.think_mode==1 && s.sub0.think_mode==1;
}
bool same_frame(const rev_action::Frame& a,const rev_action::Frame& b) noexcept {
    const auto same=[](const rev_hud::Actor& x,const rev_hud::Actor& y) noexcept {
        return x.address==y.address && x.lifetime==y.lifetime && x.serial==y.serial && x.think_mode==y.think_mode;
    };
    return a.number==b.number && a.session.active==b.session.active && a.session.epoch==b.session.epoch &&
        same(a.session.self,b.session.self) && same(a.session.sub0,b.session.sub0);
}
}
unsigned int choose_draw_schedule(const rev_action::Host& h,unsigned int context,unsigned int stock) noexcept {
    using namespace rev_action;
    stock&=255;
    if(stock || !h.memory.word || !h.snapshot || !context || context>Invalid-0x15B)return stock;
    Frame f{};
    if(h.snapshot(h.memory.context,&f)!=SnapshotMode::Local)return stock;
    if(!valid_frame(f))return stock;
    u32 vt=0,view=0;
    if(!h.memory.word(h.memory.context,context,&vt)||vt!=0x04F79014||
       !h.memory.word(h.memory.context,context+0x158,&view)||(view&255)>1)return stock;
    return 1;
}
unsigned int hunter_uncached(const rev_action::Host& h,unsigned int model,unsigned int context,unsigned int stock) noexcept {
    using namespace rev_action;
    stock&=255;
    if(stock || !h.memory.word || !h.snapshot || !model || model>Invalid-3 ||
       !context || context>Invalid-0x15B)return stock;
    Frame before{},after{};
    // snapshot certifies the owning thread before any model/context access.
    if(h.snapshot(h.memory.context,&before)!=SnapshotMode::Local || !valid_frame(before))return stock;
    u32 type=0,vt=0,view=0,again=0;
    const auto read=[&](u32 a,u32& out) noexcept {return h.memory.word(h.memory.context,a,&out);};
    if(!read(model,type) || type!=0x04D09FBC || !read(context,vt) || vt!=0x04F79014 ||
       !read(context+0x158,view) || (view&255)>1 ||
       !read(model,again) || again!=type || !read(context,again) || again!=vt ||
       !read(context+0x158,again) || again!=view ||
       h.snapshot(h.memory.context,&after)!=SnapshotMode::Local || !same_frame(before,after))return stock;
    // The audited caller maps true to kind 5, its existing uncached path.
    // Cached command objects remain owned by the engine and are not overwritten.
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
extern "C" unsigned int rev_runtime_hunter_uncached(unsigned int model,unsigned int context,unsigned int stock) noexcept {
    return rev_runtime::hunter_uncached(rev_runtime::schedule_host,model,context,stock);
}
