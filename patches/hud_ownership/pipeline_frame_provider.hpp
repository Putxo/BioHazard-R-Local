#pragma once
#include "pipeline_clock.hpp"
#include "lifetime_source.hpp"
#include "manager_driver.hpp"
namespace rev_hud {
// Reuses the existing validated memory/thread and actual render-scope services.
// This change supplies sample(), not fake success implementations of scopes.
struct PipelineServices {
    Reader memory{};
    u32 (*thread_id)(void*) noexcept = nullptr;
    bool (*enter_scope)(void*, const ManagerSite&, const ManagerFrame&, u32) noexcept = nullptr;
    bool (*leave_scope)(void*) noexcept = nullptr;
};
class PipelineFrameProvider {
public:
    PipelineFrameProvider(PipelineClock& clock, LifetimeSource& life, PipelineServices services)
        : clock_(clock), life_(life), services_(services) {}
    PipelineFrameProvider(const PipelineFrameProvider&) = delete;
    PipelineFrameProvider& operator=(const PipelineFrameProvider&) = delete;
    bool sample(ManagerFrame* out) noexcept;
    ManagerHost callbacks() noexcept;
    bool scope_fault() const noexcept { return scope_fault_; }
private:
    PipelineClock& clock_; LifetimeSource& life_; PipelineServices services_;
    PipelineStamp sampled_stamp_{};
    ManagerFrame sampled_frame_{};
    bool busy_ = false, sampled_ = false, entered_ = false, scope_fault_ = false;
    bool enter(const ManagerSite&, const ManagerFrame&, u32) noexcept;
    bool leave() noexcept;
};
}
