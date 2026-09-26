#include "begin_activator.hpp"
#include <cstdio>
#include <cstdlib>

using namespace rev_hud;

static unsigned checks=0, scenarios=0;
#define CHECK(x) do { ++checks; if(!(x)) {     std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::abort(); } } while(0)

struct Fixture {
    u32 thread=7;
    u32 ticket=0;
    bool attach_ok=true;
    u32 attach_calls=0, stop_calls=0, last_ticket=0;
    PipelineClock clock;
    BeginActivator activator;

    static u32 tid(void* p) noexcept {
        return static_cast<Fixture*>(p)->thread;
    }
    static u32 get_ticket(void* p) noexcept {
        return static_cast<Fixture*>(p)->ticket;
    }
    static bool attach(void* p,u32 t) noexcept {
        auto& f=*static_cast<Fixture*>(p);
        ++f.attach_calls; f.last_ticket=t; return f.attach_ok;
    }
    static void stop(void* p) noexcept {
        ++static_cast<Fixture*>(p)->stop_calls;
    }

    Fixture()
        : clock({this,tid}),
          activator(clock,{this,get_ticket,attach,stop}) {
        CHECK(clock.start(7));
    }

    void begin(u32 owner=0x110000,u32 frame_base=0x880000) {
        CHECK(clock.event(PipelineBegin,owner,frame_base));
    }
    void end(u32 owner=0x110000,u32 frame_base=0x880000) {
        CHECK(clock.event(PipelineEnd,owner,frame_base));
    }
};

int main() {
    {
        Fixture f; f.ticket=7; f.begin();
        CHECK(f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.attach_calls==1 && f.last_ticket==7);
        CHECK(f.activator.attached_ticket()==7);
        CHECK(f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.attach_calls==1);
        f.end(); ++scenarios;
    }
    {
        Fixture f; f.ticket=7; f.begin();
        CHECK(f.activator.event(PipelineBegin,0x110000,0x880000));
        f.end(); f.begin();
        CHECK(f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.attach_calls==1);
        f.end(); ++scenarios;
    }
    {
        Fixture f; f.begin();
        CHECK(f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.attach_calls==0 && f.activator.attached_ticket()==0);
        f.end(); ++scenarios;
    }
    {
        Fixture f; f.ticket=7; f.attach_ok=false; f.begin();
        CHECK(!f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.stop_calls==1 && f.activator.rejected_ticket()==7);
        CHECK(f.activator.fault()==BeginActivationFault::Attach);
        CHECK(!f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.attach_calls==1 && f.stop_calls==1);
        f.end();

        // Structural END later removed the rejected ticket.
        f.ticket=0; f.begin();
        CHECK(f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.activator.fault()==BeginActivationFault::None);
        f.end();

        // A later structural publication may be attached normally.
        f.ticket=8; f.attach_ok=true; f.begin();
        CHECK(f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.attach_calls==2 && f.last_ticket==8);
        CHECK(f.activator.attached_ticket()==8);
        f.end(); ++scenarios;
    }
    {
        Fixture f; f.ticket=7; f.begin();
        CHECK(!f.activator.event(PipelineBegin,0x110004,0x880000));
        CHECK(f.activator.fault()==BeginActivationFault::Clock);
        CHECK(f.attach_calls==0);
        f.end(); ++scenarios;
    }
    {
        Fixture f; f.ticket=7; f.begin();
        CHECK(!f.activator.event(PipelineEnd,0x110000,0x880000));
        CHECK(f.activator.fault()==BeginActivationFault::Config);
        CHECK(f.attach_calls==0);
        f.end(); ++scenarios;
    }
    {
        Fixture f; f.ticket=7;
        CHECK(!f.activator.event(PipelineBegin,0x110000,0x880000));
        CHECK(f.activator.fault()==BeginActivationFault::Clock);
        CHECK(f.attach_calls==0);
        ++scenarios;
    }
    {
        Fixture f; f.ticket=7; f.begin();
        CHECK(!f.activator.event(PipelineBegin,0x110000,0x880004));
        CHECK(f.stop_calls==0 && f.attach_calls==0);
        f.end(); ++scenarios;
    }

    std::printf(
        "{\"status\":\"PASS\",\"scenarios\":%u,"
        "\"assertions\":%u,\"driver_mocked\":true,"
        "\"gameplay_executed\":false}\n",
        scenarios,checks);
}
