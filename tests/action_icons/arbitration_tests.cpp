#include "../../patches/action_icons/arbitration.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_action;
static int checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
    Arbitration a;Candidate c;u32 v=99;
    C(!a.result(0,&v));C(v==99);C(a.begin(1,2,3,4));
    C(a.prepare(0,&c));C(c.priority==0&&c.flags==0&&!c.stopped);
    C(a.decide(true)==Decision::Proceed);C(a.commit(0x208,3));
    // J1's exclusive selection must neither stop J2 nor clear J1's history.
    C(a.prepare(1,&c));C(c.priority==0&&c.flags==2&&!c.stopped);
    C(a.decide(true)==Decision::Proceed);C(a.commit(0x201,3));
    C(a.prepare(0,&c));C(c.priority==0x208&&c.stopped);
    C(a.decide(true)==Decision::Next);C(a.prepare(1,&c));C(c.stopped);
    C(a.decide(true)==Decision::Next);C(a.finish());
    C(a.result(0,&v)&&v==0x208);C(a.result(1,&v)&&v==0x201);
    C(a.matches(1,2,3,4));C(!a.matches(2,2,3,4));C(!a.matches(1,3,3,4));
    C(!a.matches(1,2,4,4));C(!a.matches(1,2,3,5));
    // A lower-ranked J1 entry skips only that member; later J2 work survives.
    C(a.begin(2,2,3,4));C(a.prepare(0,&c));C(a.decide(true)==Decision::Proceed);
    C(a.commit(0x208,2));C(a.prepare(0,&c));C(a.decide(false)==Decision::Next);
    C(a.prepare(1,&c));C(!c.stopped&&c.priority==0&&c.flags==2);
    C(a.decide(true)==Decision::Proceed);C(a.commit(0x202,2));C(a.finish());
    C(a.result(0,&v)&&v==0x208);C(a.result(1,&v)&&v==0x202);
    // Invalid protocol never exposes a partially selected priority.
    C(a.begin(3,2,3,4));C(!a.begin(3,2,3,4));C(!a.result(0,&v));
    C(a.begin(4,2,3,4));C(!a.prepare(2,&c));C(!a.matches(4,2,3,4));
    C(a.begin(5,2,3,4));C(a.prepare(0,&c));C(!a.finish());
    C(a.begin(6,2,3,4));C(!a.commit(1,0));
    C(a.begin(7,2,3,4));C(a.prepare(0,&c));C(a.decide(true)==Decision::Proceed);C(!a.commit(1,4));
    C(a.begin(8,2,3,4));C(a.prepare(0,&c));C(a.decide(true)==Decision::Proceed);C(a.commit(1,2));
    C(a.prepare(1,&c));C(a.decide(true)==Decision::Proceed);C(!a.commit(2,0));
    C(!a.begin(0,2,3,4));C(!a.begin(1,0,3,4));C(!a.begin(1,2,0,4));C(!a.begin(1,2,3,0));
    C(a.begin(9,2,3,4));C(a.finish());C(a.result(0,&v)&&v==0);C(a.result(1,&v)&&v==0);
    // Projection property: interleaving two sorted command streams must yield
    // exactly the priorities from evaluating each player's stream separately.
    // The shared history ring is cleared once, regardless of who selects first.
    u32 seed=0xABCD1234;
    for(u32 trial=0;trial<200;++trial){
        struct Input {u32 member,priority;bool select,exclusive,history;} inputs[40]{};
        for(u32 i=0;i<40;++i){
            seed=seed*1664525u+1013904223u;
            inputs[i]={seed>>31,40-i,(seed&15)<5,(seed&0x10)!=0,(seed&0x20)!=0};
        }
        u32 expected[2]{};
        for(u32 m=0;m<2;++m)for(const auto& in:inputs){
            if(in.member!=m)continue;
            if(expected[m]>in.priority)break;
            if(in.select){expected[m]=in.priority;if(in.exclusive)break;}
        }
        Arbitration x;C(x.begin(trial+1,1,1,1));u32 clears=0;bool any_history=false;
        for(const auto& in:inputs){
            C(x.prepare(in.member,&c));
            const auto d=x.decide(c.priority<=in.priority);
            if(d==Decision::Next)continue;
            C(d==Decision::Proceed);u32 p=c.priority,flags=c.flags;
            if(in.select){
                p=in.priority;
                if(in.history){if(!(flags&2))++clears;flags|=2;any_history=true;}
                if(in.exclusive)flags|=1;
            }
            C(x.commit(p,flags));
        }
        C(x.finish());C(x.result(0,&v)&&v==expected[0]);C(x.result(1,&v)&&v==expected[1]);
        C(clears==(any_history?1u:0u));
    }
    std::printf("Arbitration model: %d checks passed\n",checks);
}
