#include "target_views.hpp"
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
bool valid(const ViewFrame& f) noexcept {
    const auto& s=f.owner.session;
    return f.number && s.active && s.epoch && f.owner.actor==s.sub0.address &&
        s.self.address && s.sub0.address && s.self.address!=s.sub0.address &&
        s.self.lifetime && s.sub0.lifetime && s.self.serial>=0 && s.sub0.serial>=0 &&
        s.self.serial!=s.sub0.serial && s.self.think_mode==1 && s.sub0.think_mode==1;
}
bool equal(TargetKey a,TargetKey b) noexcept {
    return a.address==b.address && a.generation==b.generation;
}
struct Busy {bool& flag;explicit Busy(bool& f):flag(f){flag=true;}~Busy(){flag=false;}};
}
bool TargetViews::same(const ViewFrame& a,const ViewFrame& b) const noexcept {
    return a.number==b.number && equal(a.owner,b.owner);
}
bool TargetViews::prepare(const ViewFrame& f) noexcept {
    if(!valid(f))return false;
    if(!bound_ || !equal(owner_,f.owner)){
        for(auto& e:entries_)e={};
        owner_=f.owner;frame_=f.number;bound_=true;
    }
    if(f.number<frame_)return false;
    frame_=f.number;return true;
}
TargetViews::Entry* TargetViews::find(TargetKey key,bool create) noexcept {
    const u32 index=life_.slot(key);
    if(index>=TargetLifetime::Capacity)return nullptr;
    auto& e=entries_[index];
    if(equal(e.target,key))return &e;
    if(!create)return nullptr;
    e={};e.target=key;return &e;
}
Mode TargetViews::publish(u32 widget,TargetKey key,const ViewSample& sample) noexcept {
    if(!host_.resolve)return Mode::Stock;
    if(!host_.on_thread || !host_.on_thread(host_.context) || busy_)return Mode::Hidden;
    Busy lock(busy_);
    ViewFrame f{};const auto mode=host_.resolve(host_.context,widget,&f);
    if(mode==Mode::Stock)return mode;
    if(mode!=Mode::Local)return Mode::Hidden;
    if(!prepare(f) || !life_.live(key) || sample.focus>3)return Mode::Hidden;
    // Reject nonfinite coordinates; bit patterns are retained without casting.
    for(u32 v:sample.position)if((v&0x7F800000u)==0x7F800000u)return Mode::Hidden;
    ViewFrame after{};
    if(host_.resolve(host_.context,widget,&after)!=Mode::Local || !same(f,after) ||
       !life_.live(key))return Mode::Hidden;
    Entry* e=find(key,true);
    if(!e)return Mode::Hidden;
    e->sample=sample;e->frame=f.number;return Mode::Local;
}
Mode TargetViews::read(u32 widget,TargetKey key,ViewSample* out) noexcept {
    if(!host_.resolve)return Mode::Stock;
    if(!host_.on_thread || !host_.on_thread(host_.context) || busy_){
        if(out)*out={};return Mode::Hidden;
    }
    Busy lock(busy_);
    ViewFrame f{};const auto mode=host_.resolve(host_.context,widget,&f);
    if(mode==Mode::Stock)return mode;
    if(out)*out={};
    if(mode!=Mode::Local || !out)return Mode::Hidden;
    if(!prepare(f) || !life_.live(key))return Mode::Hidden;
    Entry* e=find(key,false);
    // Current/previous frame supports ordinary update ordering, but does not
    // retain target positions indefinitely after a producer stops updating.
    if(!e || !e->frame || e->frame>f.number || f.number-e->frame>1)return Mode::Hidden;
    const auto sample=e->sample;ViewFrame after{};
    if(host_.resolve(host_.context,widget,&after)!=Mode::Local || !same(f,after) ||
       !life_.live(key))return Mode::Hidden;
    *out=sample;return Mode::Local;
}
}
