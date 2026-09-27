#include "priority_driver.hpp"
namespace rev_action {
namespace {
PriorityDriver* priority_sink=nullptr;
bool same_actor(const Actor& a,const Actor& b) noexcept {
    return a.address==b.address&&a.lifetime==b.lifetime&&a.serial==b.serial&&a.think_mode==b.think_mode;
}
bool same_frame(const Frame& a,const Frame& b) noexcept {
    return a.number==b.number&&a.session.active&&b.session.active&&a.session.epoch==b.session.epoch&&
        same_actor(a.session.self,b.session.self)&&same_actor(a.session.sub0,b.session.sub0);
}
bool local(const Frame& f) noexcept {
    const auto& s=f.session;
    return f.number&&s.active&&s.epoch&&s.self.address&&s.sub0.address&&s.self.address!=s.sub0.address&&
        s.self.lifetime&&s.sub0.lifetime&&s.self.serial>=0&&s.sub0.serial>=0&&
        s.self.serial!=s.sub0.serial&&s.self.think_mode==1&&s.sub0.think_mode==1;
}
}
bool PriorityDriver::ready() const noexcept {
    const auto& h=host_.routing;
    return h.memory.word&&h.write&&h.snapshot&&h.calls.member&&host_.owner_thread&&host_.rank;
}
bool PriorityDriver::owner() const noexcept {
    return ready()&&host_.owner_thread(host_.routing.memory.context);
}
bool PriorityDriver::word(u32 a,u32& v) const noexcept {
    return a&&a<=Invalid-3&&host_.routing.memory.word(host_.routing.memory.context,a,&v);
}
void PriorityDriver::abort() noexcept {aborted_=true;arbitration_.invalidate();}
void PriorityDriver::begin(u32 stack) noexcept {
    if(!owner())return;
    if(active_){abort();return;}
    arbitration_.invalidate();aborted_=false;manager_=stack_=0;
    const auto& h=host_.routing;Frame f{};
    if(h.snapshot(h.memory.context,&f)!=SnapshotMode::Local || !local(f) || stack<0x12C)return;
    u32 manager=0,global=0,vt=0,p=0,flags=0;
    if(!word(stack-8,manager)||!word(0x0556279C,global)||manager!=global||
       !word(manager,vt)||vt!=0x04CDBDB4||!word(stack-0x100,p)||p||
       !word(stack-0x10C,flags)||flags)return;
    if(!arbitration_.begin(f.number,f.session.epoch,manager,stack))return;
    frame_=f;manager_=manager;stack_=stack;active_=true;
}
bool PriorityDriver::matching(u32 stack) noexcept {
    const auto& h=host_.routing;Frame now{};u32 manager=0,global=0,vt=0;
    return !aborted_&&stack==stack_&&h.snapshot(h.memory.context,&now)==SnapshotMode::Local&&
        local(now)&&same_frame(frame_,now)&&word(stack-8,manager)&&manager==manager_&&
        word(0x0556279C,global)&&global==manager_&&word(manager_,vt)&&vt==0x04CDBDB4;
}
bool PriorityDriver::write_locals(u32 p,u32 ranked,u32 flags) noexcept {
    const u32 addresses[]={stack_-0x100,stack_-0x128,stack_-0x10C};
    const u32 values[]={p,ranked,flags};u32 old[3]{};
    for(u32 i=0;i<3;++i)if(!word(addresses[i],old[i]))return false;
    const auto& h=host_.routing;
    for(u32 i=0;i<3;++i)if(!h.write(h.memory.context,addresses[i],values[i])){
        // A failed write is contractually non-mutating. Restore only completed writes.
        for(u32 j=i;j>0;--j)(void)h.write(h.memory.context,addresses[j-1],old[j-1]);
        return false;
    }
    return true;
}
void PriorityDriver::prepare(u32 stack,u32 command) noexcept {
    if(!owner()||!active_)return;
    u32 vt=0,check=0;
    if(!matching(stack)||!command||command>Invalid-0x3F||!word(command,vt)||vt!=0x04CDA850){abort();return;}
    for(u32 o=0x34;o<=0x3C;o+=4)if(!word(command+o,check)){abort();return;}
    const auto& h=host_.routing;
    const u32 member=h.calls.member(h.calls.context,command);
    Candidate c{};
    if(!matching(stack)||!arbitration_.prepare(member,&c)){abort();return;}
    const u32 ranked=host_.rank(h.memory.context,c.priority);
    if(!matching(stack)||!write_locals(c.priority,ranked,c.flags))abort();
}
Decision PriorityDriver::decide(u32 stack,bool accepted) noexcept {
    if(!owner()||!active_)return accepted?Decision::Proceed:Decision::End;
    if(!matching(stack)){abort();return Decision::End;}
    return arbitration_.decide(accepted);
}
bool PriorityDriver::commit(u32 stack) noexcept {
    if(!owner()||!active_)return true;
    u32 p=0,flags=0;
    if(!matching(stack)||!word(stack-0x100,p)||!word(stack-0x10C,flags)||
       !arbitration_.commit(p,flags)){abort();return false;}
    const auto& h=host_.routing;
    // The native following bit-0 test must continue the shared iterator. Stop
    // state stays in this member's slot, while history bit 1 remains shared.
    if(!h.write(h.memory.context,stack-0x10C,flags&~1u)){abort();return false;}
    return true;
}
void PriorityDriver::finish(u32 stack) noexcept {
    if(!owner()||!active_)return;
    u32 p=0;
    if(!matching(stack)||!arbitration_.finish()||!arbitration_.result(0,&p))abort();
    else {
        const auto& h=host_.routing;
        if(!h.write(h.memory.context,stack-0x100,p))abort();
    }
    active_=false;
}
bool PriorityDriver::priority(const Frame& f,u32 manager,u32 member,u32* out) const noexcept {
    return owner()&&!active_&&!aborted_&&same_frame(f,frame_)&&manager==manager_&&
        arbitration_.result(member,out);
}
bool bind_priority(PriorityDriver& d) noexcept {
    if(priority_sink||!d.ready())return false;
    priority_sink=&d;return true;
}
}
extern "C" void rev_priority_begin(unsigned int s) noexcept {
    if(rev_action::priority_sink)rev_action::priority_sink->begin(s);
}
extern "C" void rev_priority_prepare(unsigned int s,unsigned int c) noexcept {
    if(rev_action::priority_sink)rev_action::priority_sink->prepare(s,c);
}
extern "C" unsigned int rev_priority_decide(unsigned int s,unsigned int a) noexcept {
    return static_cast<unsigned int>(rev_action::priority_sink?rev_action::priority_sink->decide(s,a!=0):
        (a?rev_action::Decision::Proceed:rev_action::Decision::End));
}
extern "C" unsigned int rev_priority_commit(unsigned int s) noexcept {
    return !rev_action::priority_sink||rev_action::priority_sink->commit(s);
}
extern "C" void rev_priority_finish(unsigned int s) noexcept {
    if(rev_action::priority_sink)rev_action::priority_sink->finish(s);
}
