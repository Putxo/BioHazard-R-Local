#include "structural_window.hpp"
namespace rev_hud {
namespace { StructuralWindow* sink=nullptr; }
bool StructuralWindow::on_thread() const noexcept {
    return thread_ && access_.thread_id &&
        access_.thread_id(access_.context)==thread_;
}
bool StructuralWindow::start(unsigned int thread) noexcept {
    if(thread_ || !thread || !access_.thread_id || !task_.run ||
       access_.thread_id(access_.context)!=thread) return false;
    thread_=thread;return true;
}
bool StructuralWindow::safe() const noexcept {
    return active_ && busy_ && on_thread() &&
        fault_==StructuralFault::None && clock_.quiescent() &&
        clock_.completed_frame()==last_frame_;
}
bool StructuralWindow::event(unsigned int site,unsigned int owner,
                             unsigned int frame_base) noexcept {
    if(site!=PipelineEnd || !owner || !frame_base || (frame_base&3) ||
       !on_thread() || busy_ || active_ || fault_!=StructuralFault::None ||
       !clock_.quiescent()) return fail(StructuralFault::Protocol);
    const unsigned int frame=clock_.completed_frame();
    if(!frame || frame<=last_frame_) return fail(StructuralFault::Protocol);
    last_frame_=frame;busy_=active_=true;
    const bool result=task_.run(task_.context,frame,owner,frame_base);
    const bool unchanged=clock_.quiescent() && clock_.completed_frame()==frame &&
        on_thread() && fault_==StructuralFault::None;
    active_=busy_=false;
    if(!result) return fail(StructuralFault::Task);
    if(!unchanged) return fail(StructuralFault::Changed);
    return true;
}
bool bind_structural_sink(StructuralWindow& window) noexcept {
    if (sink) return false;
    sink=&window;
    return true;
}
}
extern "C" unsigned int rev_hud_structural_event(unsigned int site,
        unsigned int owner,unsigned int frame_base) noexcept {
    return rev_hud::sink && rev_hud::sink->event(site,owner,frame_base)?1u:0u;
}
