#include "pipeline_frame_provider.hpp"
namespace rev_hud {
namespace {
struct Busy {
    bool& value;
    explicit Busy(bool& v) : value(v) { value = true; }
    ~Busy() { value = false; }
};
bool same_actor(const Actor& a, const Actor& b) {
    return a.address == b.address && a.lifetime == b.lifetime &&
        a.serial == b.serial && a.think_mode == b.think_mode;
}
bool same_frame(const ManagerFrame& a, const ManagerFrame& b) {
    if (a.frame != b.frame || a.session.active != b.session.active ||
        a.session.epoch != b.session.epoch || !same_actor(a.session.self,b.session.self) ||
        !same_actor(a.session.sub0,b.session.sub0)) return false;
    for (u32 i=0; i<2; ++i)
        if (a.parents[i] != b.parents[i] || a.parent_lifetimes[i] != b.parent_lifetimes[i]) return false;
    return true;
}
}
bool PipelineFrameProvider::sample(ManagerFrame* out) noexcept {
    if (!out || busy_ || entered_ || scope_fault_) return false;
    Busy lock(busy_); sampled_ = false;
    PipelineStamp stamp{}; LifeSnapshot life{};
    if (!clock_.capture(&stamp) || !life_.capture(&life) || !clock_.matches(stamp)) return false;
    ManagerFrame frame{};
    frame.frame = stamp.frame; frame.session = life.session;
    for (u32 i=0; i<2; ++i) {
        frame.parents[i] = life.parents[i];
        frame.parent_lifetimes[i] = life.parent_lifetimes[i];
    }
    sampled_stamp_ = stamp; sampled_frame_ = frame; sampled_ = true;
    *out = frame; return true;
}
bool PipelineFrameProvider::enter(const ManagerSite& site, const ManagerFrame& frame,
                                  u32 context) noexcept {
    if (busy_ || entered_ || scope_fault_ || !sampled_ || !services_.enter_scope ||
        !services_.leave_scope || !same_frame(frame,sampled_frame_) ||
        !clock_.matches(sampled_stamp_)) return false;
    Busy lock(busy_); sampled_ = false;
    if (!services_.enter_scope(services_.memory.context,site,frame,context)) return false;
    // A successful scope must be unwound even if an event closed the interval
    // inside enter_scope. ManagerDriver will not call leave after false.
    if (!clock_.matches(sampled_stamp_)) {
        if (!services_.leave_scope(services_.memory.context)) scope_fault_ = true;
        return false;
    }
    entered_ = true; return true;
}
bool PipelineFrameProvider::leave() noexcept {
    if (busy_ || !entered_ || !services_.leave_scope) return false;
    Busy lock(busy_);
    const bool restored = services_.leave_scope(services_.memory.context);
    entered_ = false; sampled_ = false;
    if (!restored) scope_fault_ = true;
    return restored;
}
ManagerHost PipelineFrameProvider::callbacks() noexcept {
    ManagerHost h{};
    h.memory = {this, [](void* c,u32 address,u32* out) noexcept {
        auto& p=*static_cast<PipelineFrameProvider*>(c);
        return out && p.services_.memory.word &&
            p.services_.memory.word(p.services_.memory.context,address,out);
    }};
    h.thread_id = [](void* c) noexcept -> u32 {
        auto& p=*static_cast<PipelineFrameProvider*>(c);
        return p.services_.thread_id ? p.services_.thread_id(p.services_.memory.context) : 0;
    };
    h.sample = [](void* c,ManagerFrame* out) noexcept {
        return static_cast<PipelineFrameProvider*>(c)->sample(out);
    };
    h.enter_scope = [](void* c,const ManagerSite& site,const ManagerFrame& frame,u32 context) noexcept {
        return static_cast<PipelineFrameProvider*>(c)->enter(site,frame,context);
    };
    h.leave_scope = [](void* c) noexcept { return static_cast<PipelineFrameProvider*>(c)->leave(); };
    return h;
}
}
