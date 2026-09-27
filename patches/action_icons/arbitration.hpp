#pragma once
#include "router.hpp"
namespace rev_action {
// The native ordered loop has a per-member priority and stop bit, but one
// history-clear bit for its shared recent-command ring. Keep those distinct.
enum class Decision : u32 { Proceed=0, Next=1, End=2 };
struct Candidate {u32 priority=0,flags=0;bool stopped=false;};
class Arbitration {
public:
    bool begin(u32 frame,u32 epoch,u32 manager,u32 stack) noexcept;
    bool prepare(u32 member,Candidate*) noexcept;
    Decision decide(bool priority_accepted) noexcept;
    bool commit(u32 native_priority,u32 native_flags) noexcept;
    bool finish() noexcept;
    bool result(u32 member,u32* out) const noexcept;
    bool matches(u32 frame,u32 epoch,u32 manager,u32 stack) const noexcept;
    void invalidate() noexcept;
private:
    enum class Phase {Empty,Loop,Prepared,Executing,Published,Fault};
    Phase phase_=Phase::Empty;
    u32 frame_=0,epoch_=0,manager_=0,stack_=0,member_=Invalid;
    u32 priorities_[2]{},history_=0;
    bool stopped_[2]{};
};
}
