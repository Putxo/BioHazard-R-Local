#include "../../patches/genesis/progress.hpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using namespace rev_genesis;
unsigned checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Fake {
    Owner owner{{true,4,{0x1000,1,0,1},{0x2000,2,1,1}},0x2000};
    Mode mode=Mode::Local;
    u32 unit=0x3000;
    bool recurse=false;
    Progress* progress=nullptr;
    ProgressHost host(){return {this,[](void* p,u32 w,Owner* o) noexcept {
        auto& f=*static_cast<Fake*>(p);
        if(w!=f.unit)return Mode::Stock;
        if(f.recurse){u32 x=9;C(f.progress->access(w,Operation::Add,100,&x)==Mode::Hidden);C(x==0);}
        *o=f.owner;return f.mode;
    }};}
};
int main(){
    u32 out=7;C(rev_genesis_progress(0x3000,0,0,&out)==0);C(out==0);
    Fake f;Progress p(f.host());f.progress=&p;
    auto use=[&](Operation op,u32 n,u32 expected){out=~0u;C(p.access(f.unit,op,n,&out)==Mode::Local);C(out==expected);};
    for(u32 value=0;value<=100;++value)for(u32 add: {0u,1u,25u,100u,101u,~0u,~0u-50}) {
        use(Operation::Set,value,value);
        const u32 native=value+add;
        use(Operation::Add,add,native>100?100:native);
        use(Operation::Read,0,native>100?100:native);
    }
    use(Operation::Set,~0u,100);
    f.mode=Mode::Hidden;C(p.access(f.unit,Operation::Set,0,&out)==Mode::Hidden);C(out==0);
    f.mode=Mode::Stock;C(p.access(f.unit,Operation::Add,99,&out)==Mode::Stock);C(out==0);
    f.mode=Mode::Local;use(Operation::Read,0,100);
    C(p.access(0x9999,Operation::Set,0,&out)==Mode::Stock);use(Operation::Read,0,100);
    // A replacement GUI with the same certified actors keeps collected progress.
    f.unit+=0x100;use(Operation::Read,0,100);
    const Owner original=f.owner;
    for(u32 field=0;field<9;++field) {
        f.owner=original;use(Operation::Set,73,73);
        if(field==0)++f.owner.session.epoch;
        if(field==1)++f.owner.session.self.address;
        if(field==2)++f.owner.session.self.lifetime;
        if(field==3)f.owner.session.self.serial=9;
        if(field==4)++f.owner.session.sub0.address;
        if(field==5)++f.owner.session.sub0.lifetime;
        if(field==6)f.owner.session.sub0.serial=8;
        if(field==7){f.owner.session.self.lifetime+=2;f.owner.session.sub0.lifetime+=3;}
        if(field==8)f.owner.session.epoch+=7;
        f.owner.actor=f.owner.session.sub0.address;
        use(Operation::Read,0,0);
    }
    for(u32 field=0;field<12;++field) {
        f.owner=original;
        if(field==0)f.owner.session.active=false;
        if(field==1)f.owner.session.epoch=0;
        if(field==2)f.owner.session.self.address=0;
        if(field==3)f.owner.session.sub0.address=f.owner.session.self.address;
        if(field==4)f.owner.session.self.lifetime=0;
        if(field==5)f.owner.session.sub0.lifetime=0;
        if(field==6)f.owner.session.self.serial=-1;
        if(field==7)f.owner.session.sub0.serial=f.owner.session.self.serial;
        if(field==8)f.owner.session.self.think_mode=0;
        if(field==9)f.owner.session.sub0.think_mode=3;
        if(field==10)f.owner.actor=f.owner.session.self.address;
        if(field==11)f.owner.actor=0;
        C(p.access(f.unit,Operation::Add,99,&out)==Mode::Hidden);C(out==0);
    }
    f.owner=original;f.recurse=true;use(Operation::Set,18,18);f.recurse=false;
    C(p.access(0,Operation::Read,0,&out)==Mode::Hidden);
    C(p.access(f.unit,static_cast<Operation>(3),0,&out)==Mode::Hidden);
    C(p.access(f.unit,Operation::Read,0,nullptr)==Mode::Hidden);
    p.reset();use(Operation::Read,0,0);
    C(bind_progress(p));C(bind_progress(p));Progress other(f.host());C(!bind_progress(other));
    C(rev_genesis_progress(f.unit,1,31,&out)==1);C(out==31);
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"gameplay_executed\":false}\n",checks);
}
