#include "pipeline_clock.hpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using namespace rev_hud;
static unsigned scenarios=0,checks=0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::abort(); } } while(0)
struct Fixture {
    unsigned thread=7;
    PipelineClock clock;
    static unsigned tid(void* p) noexcept { return static_cast<Fixture*>(p)->thread; }
    Fixture() : clock({this,tid}) { CHECK(clock.start(7)); }
    bool begin(unsigned owner=0x110000,unsigned frame=0x880000) { return clock.event(PipelineBegin,owner,frame); }
    bool end(unsigned owner=0x110000,unsigned frame=0x880000) { return clock.event(PipelineEnd,owner,frame); }
};
int main() {
    { PipelineSequence s{5,9}; CHECK(advance_pipeline_sequence(&s,true)); CHECK(s.frame==6 && s.revision==10);
      CHECK(advance_pipeline_sequence(&s,false)); CHECK(s.frame==6 && s.revision==11);
      CHECK(!advance_pipeline_sequence(nullptr,true)); ++scenarios; }
    for (bool begin : {false,true}) {
        PipelineSequence s{3,~0u}; CHECK(!advance_pipeline_sequence(&s,begin)); CHECK(s.frame==3 && s.revision==~0u); ++scenarios;
    }
    { PipelineSequence s{~0u,12}; CHECK(!advance_pipeline_sequence(&s,true)); CHECK(s.frame==~0u && s.revision==12);
      CHECK(advance_pipeline_sequence(&s,false)); CHECK(s.frame==~0u && s.revision==13); ++scenarios; }
    { PipelineSequence s{~0u-1,~0u-1}; CHECK(advance_pipeline_sequence(&s,true)); CHECK(s.frame==~0u && s.revision==~0u);
      CHECK(!advance_pipeline_sequence(&s,false)); ++scenarios; }
    { PipelineClock c({}); PipelineStamp out{11,12,13,14}; CHECK(!c.start(7)); CHECK(!c.capture(&out));
      CHECK(!c.event(PipelineBegin,1,4)); CHECK(out.frame==11 && out.frame_base==14); ++scenarios; }
    { Fixture f; CHECK(!f.clock.start(7)); CHECK(!f.clock.capture(nullptr)); PipelineStamp out{11,12,13,14};
      CHECK(!f.clock.capture(&out)); CHECK(out.frame==11); CHECK(f.begin()); CHECK(f.clock.capture(&out));
      CHECK(out.frame==1 && out.owner==0x110000 && out.frame_base==0x880000);
      for (unsigned i=0;i<40;++i) { PipelineStamp repeat{}; CHECK(f.clock.capture(&repeat)); CHECK(repeat.frame==1); CHECK(f.clock.matches(out)); }
      CHECK(f.end()); CHECK(!f.clock.matches(out)); PipelineStamp sentinel{44,55,66,77};
      CHECK(!f.clock.capture(&sentinel)); CHECK(sentinel.frame==44 && sentinel.revision==55);
      CHECK(f.begin()); PipelineStamp next{}; CHECK(f.clock.capture(&next)); CHECK(next.frame==2); CHECK(!f.clock.matches(out)); CHECK(f.end()); ++scenarios; }
    // No simulation counter, per-manager call or presentation result affects the
    // clock: repeated producer cycles remain distinct even while simulation is paused.
    { Fixture f; unsigned simulation=80;
      for (unsigned frame=1;frame<=200;++frame) { CHECK(f.begin()); PipelineStamp p{}; CHECK(f.clock.capture(&p));
          CHECK(p.frame==frame); CHECK(simulation==80); CHECK(f.end()); } ++scenarios; }
    { Fixture f; CHECK(f.begin()); PipelineStamp p{}; CHECK(f.clock.capture(&p)); f.thread=8;
      CHECK(!f.end()); CHECK(!f.clock.capture(&p)); f.thread=7; CHECK(f.clock.matches(p)); CHECK(f.end()); ++scenarios; }
    { Fixture f; CHECK(!f.clock.event(0x033BFE56,0x110000,0x880000)); CHECK(f.clock.fault()==PipelineFault::None);
      CHECK(f.begin()); CHECK(!f.clock.event(0x1234,0x110000,0x880000)); CHECK(f.end()); ++scenarios; }
    { Fixture f; CHECK(!f.end()); CHECK(f.clock.fault()==PipelineFault::Protocol); CHECK(!f.begin()); ++scenarios; }
    { Fixture f; CHECK(f.begin()); CHECK(!f.begin()); CHECK(f.clock.fault()==PipelineFault::Protocol);
      PipelineStamp p{}; CHECK(!f.clock.capture(&p)); CHECK(!f.end()); ++scenarios; }
    for (unsigned bad=0;bad<4;++bad) {
      Fixture f; CHECK(f.begin()); CHECK(!f.end(bad==0 ? 0 : bad==1 ? 0x110004 : 0x110000,
                                            bad==2 ? 0x880004 : bad==3 ? 0x880001 : 0x880000));
      CHECK(f.clock.fault()==PipelineFault::Protocol); CHECK(!f.begin()); ++scenarios;
    }
    for (unsigned bad=0;bad<3;++bad) {
      Fixture f; CHECK(!f.begin(bad==0 ? 0 : 0x110000,bad==1 ? 0 : bad==2 ? 0x880001 : 0x880000));
      CHECK(f.clock.fault()==PipelineFault::Protocol); ++scenarios;
    }
    { Fixture f; CHECK(f.begin()); PipelineStamp p{}; CHECK(f.clock.capture(&p));
      PipelineStamp wrong=p; ++wrong.frame; CHECK(!f.clock.matches(wrong)); wrong=p; ++wrong.revision; CHECK(!f.clock.matches(wrong));
      wrong=p; ++wrong.owner; CHECK(!f.clock.matches(wrong)); wrong=p; ++wrong.frame_base; CHECK(!f.clock.matches(wrong)); CHECK(f.end()); ++scenarios; }
    { Fixture f; CHECK(f.begin()); CHECK(f.end()); CHECK(f.begin(0x220000,0x990000)); PipelineStamp s{};
      CHECK(f.clock.capture(&s)); CHECK(s.frame==2 && s.owner==0x220000); CHECK(f.end(0x220000,0x990000)); ++scenarios; }
    { Fixture f; CHECK(!rev_hud_pipeline_event(PipelineBegin,0x110000,0x880000));
      CHECK(bind_pipeline_sink(f.clock)); CHECK(!bind_pipeline_sink(f.clock));
      CHECK(rev_hud_pipeline_event(PipelineBegin,0x110000,0x880000)==1); CHECK(rev_hud_pipeline_event(PipelineEnd,0x110000,0x880000)==1); ++scenarios; }
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"engine_executed\":false,\"gameplay_executed\":false}\n",scenarios,checks);
}
