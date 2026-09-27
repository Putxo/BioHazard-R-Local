#pragma once
#include "arbitration.hpp"
namespace rev_action {
struct PriorityHost {
    Host routing{};
    bool (*owner_thread)(void*) noexcept=nullptr;
    u32 (*rank)(void*,u32) noexcept=nullptr;
};
// Coordinates the audited stack locals of ONE sActionCommand update on its
// owning thread. It never allocates/copies/replaces the native command list.
class PriorityDriver {
public:
    explicit PriorityDriver(PriorityHost h):host_(h){}
    bool ready() const noexcept;
    void begin(u32 stack) noexcept;
    void prepare(u32 stack,u32 command) noexcept;
    Decision decide(u32 stack,bool stock_accepted) noexcept;
    bool commit(u32 stack) noexcept;
    void finish(u32 stack) noexcept;
    bool priority(const Frame&,u32 manager,u32 member,u32* out) const noexcept;
private:
    PriorityHost host_;
    Arbitration arbitration_;
    Frame frame_{};
    u32 manager_=0,stack_=0;
    bool active_=false,aborted_=false;
    bool owner() const noexcept;
    bool word(u32,u32&) const noexcept;
    bool matching(u32) noexcept;
    void abort() noexcept;
    bool write_locals(u32 priority,u32 ranked,u32 flags) noexcept;
};
bool bind_priority(PriorityDriver&) noexcept;
}
extern "C" {
void rev_priority_begin(unsigned int stack) noexcept;
void rev_priority_prepare(unsigned int stack,unsigned int command) noexcept;
unsigned int rev_priority_decide(unsigned int stack,unsigned int accepted) noexcept;
unsigned int rev_priority_commit(unsigned int stack) noexcept;
void rev_priority_finish(unsigned int stack) noexcept;
}
