#pragma once
// A CPU update/draw-cycle clock, NOT a GPU fence or successful-Present count.
// Events come only from the two audited sSkeletonMain instruction windows.
namespace rev_hud {
static_assert(sizeof(unsigned int) == 4, "January PE32 clock width");
constexpr unsigned int PipelineBegin = 0x02F506A0;
constexpr unsigned int PipelineEnd = 0x02F50F4B;
struct PipelineAccess {
    void* context = nullptr;
    unsigned int (*thread_id)(void*) noexcept = nullptr;
};
struct PipelineSequence { unsigned int frame = 0, revision = 1; };
// Failure leaves the sequence unchanged; counters never wrap to an old stamp.
bool advance_pipeline_sequence(PipelineSequence*, bool begin) noexcept;
struct PipelineStamp {
    unsigned int frame = 0, revision = 0, owner = 0, frame_base = 0;
};
enum class PipelineFault { None, Protocol, Exhausted };
class PipelineClock {
public:
    explicit PipelineClock(PipelineAccess access) : access_(access) {}
    PipelineClock(const PipelineClock&) = delete;
    PipelineClock& operator=(const PipelineClock&) = delete;
    bool start(unsigned int thread) noexcept;
    bool event(unsigned int site, unsigned int owner, unsigned int frame_base) noexcept;
    // These never read game memory; closed/invalid intervals leave out intact.
    bool capture(PipelineStamp* out) const noexcept;
    bool matches(const PipelineStamp&) const noexcept;
    PipelineFault fault() const noexcept { return fault_; }
private:
    PipelineAccess access_;
    PipelineSequence sequence_{};
    unsigned int thread_ = 0, owner_ = 0, frame_base_ = 0;
    bool open_ = false;
    PipelineFault fault_ = PipelineFault::None;
    bool on_thread() const noexcept;
    bool fail(PipelineFault) noexcept;
};
// Bind before either gateway is reachable. Process lifetime, no hot-unload.
bool bind_pipeline_sink(PipelineClock&) noexcept;
}
extern "C" unsigned int rev_hud_pipeline_event(unsigned int site,
    unsigned int owner, unsigned int frame_base) noexcept;
