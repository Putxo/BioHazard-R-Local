#include "structural_window.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks=0,scenarios=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Fixture {
    unsigned thread=7,calls=0;bool fail_task=false,reopen=false,saw_safe=false;
    PipelineClock clock;
    StructuralWindow window;
    Fixture():clock({this,tid}),window(clock,{this,tid},{this,task}){
        CHECK(clock.start(7));CHECK(window.start(7));
    }
    static unsigned tid(void* c) noexcept{return static_cast<Fixture*>(c)->thread;}
    static bool task(void* c,unsigned frame,unsigned owner,unsigned base) noexcept{
        auto& f=*static_cast<Fixture*>(c);++f.calls;
        CHECK(frame==f.clock.completed_frame());CHECK(owner==0x550000);CHECK(base==0x660000);
        f.saw_safe=f.window.safe();
        if(f.reopen)CHECK(f.clock.event(PipelineBegin,owner,base));
        return !f.fail_task;
    }
    void cycle(){
        CHECK(clock.event(PipelineBegin,0x550000,0x660000));
        CHECK(clock.event(PipelineEnd,0x550000,0x660000));
    }
};
int main(){
 {Fixture f;f.cycle();CHECK(f.clock.quiescent());CHECK(!f.window.safe());
  CHECK(f.window.event(PipelineEnd,0x550000,0x660000));CHECK(f.saw_safe);CHECK(!f.window.safe());++scenarios;}
 {Fixture f;f.cycle();f.fail_task=true;CHECK(!f.window.event(PipelineEnd,0x550000,0x660000));
  CHECK(f.window.fault()==StructuralFault::Task);++scenarios;}
 {Fixture f;f.cycle();f.reopen=true;CHECK(!f.window.event(PipelineEnd,0x550000,0x660000));
  CHECK(f.saw_safe);CHECK(f.window.fault()==StructuralFault::Changed);++scenarios;}
 {Fixture f;CHECK(!f.window.event(PipelineEnd,0x550000,0x660000));CHECK(f.window.fault()==StructuralFault::Protocol);++scenarios;}
 {Fixture f;f.cycle();f.thread=8;CHECK(!f.window.event(PipelineEnd,0x550000,0x660000));++scenarios;}
 {Fixture f;f.cycle();CHECK(!f.window.event(PipelineBegin,0x550000,0x660000));++scenarios;}
 {Fixture f;f.cycle();CHECK(f.window.event(PipelineEnd,0x550000,0x660000));f.cycle();
  CHECK(f.window.event(PipelineEnd,0x550000,0x660000));CHECK(f.calls==2);++scenarios;}
 std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false}\n",scenarios,checks);
}
