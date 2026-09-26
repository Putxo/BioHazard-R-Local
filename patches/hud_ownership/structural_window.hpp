#pragma once
#include "pipeline_clock.hpp"
namespace rev_hud {
enum class StructuralFault : unsigned int { None, Protocol, Task, Changed };
struct StructuralAccess {
    void* context=nullptr;
    unsigned int (*thread_id)(void*) noexcept=nullptr;
};
struct StructuralTask {
    void* context=nullptr;
    bool (*run)(void*,unsigned int frame,unsigned int owner,
                unsigned int frame_base) noexcept=nullptr;
};
class StructuralWindow {
public:
    StructuralWindow(PipelineClock& clock, StructuralAccess access, StructuralTask task)
        : clock_(clock),access_(access),task_(task) {}
    StructuralWindow(const StructuralWindow&)=delete;
    StructuralWindow& operator=(const StructuralWindow&)=delete;
    bool start(unsigned int owning_thread) noexcept;
    bool event(unsigned int site,unsigned int owner,unsigned int frame_base) noexcept;
    bool safe() const noexcept;
    bool active() const noexcept { return active_; }
    StructuralFault fault() const noexcept { return fault_; }
private:
    PipelineClock& clock_;
    StructuralAccess access_{};
    StructuralTask task_{};
    unsigned int thread_=0,last_frame_=0;
    bool active_=false,busy_=false;
    StructuralFault fault_=StructuralFault::None;
    bool on_thread() const noexcept;
    bool fail(StructuralFault f) noexcept {active_=false;fault_=f;return false;}
};
bool bind_structural_sink(StructuralWindow&) noexcept;
}
extern "C" unsigned int rev_hud_structural_event(unsigned int site,
    unsigned int owner,unsigned int frame_base) noexcept;
