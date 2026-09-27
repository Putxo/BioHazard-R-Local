#include "progress.hpp"
namespace rev_genesis {
namespace {
Progress* Bound=nullptr;
bool same(const Actor& a,const Actor& b) noexcept {
    return a.address==b.address && a.lifetime==b.lifetime &&
        a.serial==b.serial && a.think_mode==b.think_mode;
}
bool valid(const Session& s) noexcept {
    return s.active && s.epoch && s.self.address && s.sub0.address &&
        s.self.address!=s.sub0.address && s.self.lifetime && s.sub0.lifetime &&
        s.self.serial>=0 && s.sub0.serial>=0 && s.self.serial!=s.sub0.serial &&
        s.self.think_mode==1 && s.sub0.think_mode==1;
}
struct Busy {bool& b;explicit Busy(bool& v):b(v){b=true;}~Busy(){b=false;}};
}
void Progress::reset() noexcept {session_={};value_=0;bound_=false;}
Mode Progress::access(u32 widget,Operation op,u32 argument,u32* result) noexcept {
    if(result)*result=0;
    if(!result || !widget || busy_ || !host_.resolve ||
        static_cast<u32>(op)>static_cast<u32>(Operation::Add))return Mode::Hidden;
    Busy guard(busy_);
    Owner owner{};
    const auto mode=host_.resolve(host_.context,widget,&owner);
    if(mode!=Mode::Local)return mode==Mode::Stock?Mode::Stock:Mode::Hidden;
    if(!valid(owner.session) || owner.actor!=owner.session.sub0.address)return Mode::Hidden;
    // GUI recreation keeps progress while the complete actor/session identity
    // stays valid. Reusing an actor address or changing scene epoch resets it.
    if(!bound_ || session_.epoch!=owner.session.epoch ||
       !same(session_.self,owner.session.self) || !same(session_.sub0,owner.session.sub0)) {
        session_=owner.session;value_=0;bound_=true;
    }
    if(op==Operation::Set)value_=argument>100?100:argument;
    if(op==Operation::Add) {
        const u32 sum=value_+argument; // native 32-bit addition wraps before clamp
        value_=sum>100?100:sum;
    }
    *result=value_;return Mode::Local;
}
bool bind_progress(Progress& p) noexcept {
    if(Bound && Bound!=&p)return false;
    Bound=&p;return true;
}
}
extern "C" unsigned int rev_genesis_progress(unsigned int widget,
    unsigned int operation,unsigned int argument,unsigned int* result) noexcept {
    if(result)*result=0;
    return static_cast<unsigned int>(rev_genesis::Bound?
        rev_genesis::Bound->access(widget,static_cast<rev_genesis::Operation>(operation),argument,result):
        rev_hud::Mode::Stock);
}
