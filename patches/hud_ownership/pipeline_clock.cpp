#include "pipeline_clock.hpp"
namespace rev_hud {
namespace { PipelineClock* sink = nullptr; }
bool advance_pipeline_sequence(PipelineSequence* s, bool begin) noexcept {
    if (!s || s->revision == ~0u || (begin && s->frame == ~0u)) return false;
    ++s->revision;
    if (begin) ++s->frame;
    return true;
}
bool PipelineClock::on_thread() const noexcept {
    return thread_ && access_.thread_id && access_.thread_id(access_.context) == thread_;
}
bool PipelineClock::start(unsigned int thread) noexcept {
    if (thread_ || !thread || !access_.thread_id ||
        access_.thread_id(access_.context) != thread) return false;
    thread_ = thread;
    return true;
}
bool PipelineClock::fail(PipelineFault fault) noexcept {
    fault_ = fault; open_ = false; owner_ = frame_base_ = 0;
    return false;
}
bool PipelineClock::event(unsigned int site, unsigned int owner,
                          unsigned int frame_base) noexcept {
    if ((site != PipelineBegin && site != PipelineEnd) || !on_thread() ||
        fault_ != PipelineFault::None) return false;
    if (!owner || !frame_base || (frame_base & 3)) return fail(PipelineFault::Protocol);
    const bool begin = site == PipelineBegin;
    if ((begin && open_) || (!begin && (!open_ || owner != owner_ || frame_base != frame_base_)))
        return fail(PipelineFault::Protocol);
    if (!advance_pipeline_sequence(&sequence_, begin)) return fail(PipelineFault::Exhausted);
    open_ = begin;
    owner_ = begin ? owner : 0;
    frame_base_ = begin ? frame_base : 0;
    return true;
}
bool PipelineClock::capture(PipelineStamp* out) const noexcept {
    if (!out || !on_thread() || !open_ || fault_ != PipelineFault::None) return false;
    *out = {sequence_.frame, sequence_.revision, owner_, frame_base_};
    return true;
}
bool PipelineClock::matches(const PipelineStamp& stamp) const noexcept {
    PipelineStamp now{};
    return capture(&now) && stamp.frame == now.frame && stamp.revision == now.revision &&
        stamp.owner == now.owner && stamp.frame_base == now.frame_base;
}
bool bind_pipeline_sink(PipelineClock& clock) noexcept {
    if (sink) return false;
    sink = &clock; return true;
}
}
extern "C" unsigned int rev_hud_pipeline_event(unsigned int site,
        unsigned int owner, unsigned int frame_base) noexcept {
    return rev_hud::sink && rev_hud::sink->event(site, owner, frame_base) ? 1u : 0u;
}
