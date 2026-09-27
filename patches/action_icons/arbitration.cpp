#include "arbitration.hpp"
namespace rev_action {
void Arbitration::invalidate() noexcept {phase_=Phase::Fault;}
bool Arbitration::begin(u32 frame,u32 epoch,u32 manager,u32 stack) noexcept {
    if(!frame || !epoch || !manager || !stack || phase_==Phase::Loop ||
       phase_==Phase::Prepared || phase_==Phase::Executing){invalidate();return false;}
    frame_=frame;epoch_=epoch;manager_=manager;stack_=stack;member_=Invalid;
    priorities_[0]=priorities_[1]=history_=0;stopped_[0]=stopped_[1]=false;
    phase_=Phase::Loop;return true;
}
bool Arbitration::matches(u32 frame,u32 epoch,u32 manager,u32 stack) const noexcept {
    return phase_!=Phase::Empty && phase_!=Phase::Fault && frame==frame_ &&
        epoch==epoch_ && manager==manager_ && stack==stack_;
}
bool Arbitration::prepare(u32 member,Candidate* out) noexcept {
    if(phase_!=Phase::Loop || member>1 || !out){invalidate();return false;}
    member_=member;
    *out={priorities_[member],history_,stopped_[member]};
    phase_=Phase::Prepared;return true;
}
Decision Arbitration::decide(bool accepted) noexcept {
    if(phase_!=Phase::Prepared){invalidate();return Decision::End;}
    if(stopped_[member_] || !accepted){
        stopped_[member_]=true;phase_=Phase::Loop;return Decision::Next;
    }
    phase_=Phase::Executing;return Decision::Proceed;
}
bool Arbitration::commit(u32 priority,u32 flags) noexcept {
    if(phase_!=Phase::Executing || (flags&~3u) || (history_ && !(flags&2))){
        invalidate();return false;
    }
    priorities_[member_]=priority;stopped_[member_]=(flags&1)!=0;
    history_=flags&2;phase_=Phase::Loop;return true;
}
bool Arbitration::finish() noexcept {
    if(phase_!=Phase::Loop){invalidate();return false;}
    phase_=Phase::Published;return true;
}
bool Arbitration::result(u32 member,u32* out) const noexcept {
    if(phase_!=Phase::Published || member>1 || !out)return false;
    *out=priorities_[member];return true;
}
}
