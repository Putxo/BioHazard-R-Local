#include "lifetime_source.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks=0,scenarios=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
constexpr u32 NP=0x100000,PL=0x110000,CO=0x120000,MM=0x130000,MAIN=0x140000,SUB=0x150000;
constexpr u32 NPC=0x04D9B25C,PLAYER=0x04D9CDF4,ACTIVE=0x057D9188,TRACK=0x057D9184;
constexpr u32 BEGIN=0x02DF4F65,END=0x02DF504B;
struct Fixture {
    Registry registry;
    std::map<u32,u32> mem;
    u32 thread=7,self=PL,reads=0,self_calls=0,trigger=0,event_site=0,event_object=0;
    bool reenter=false;
    LifetimeSource source;
    Fixture():source(registry,{this,read,tid,get_self}) {
        mem[PL]=mem[NP]=NPC;mem[CO]=manager_vtable(ManagerKind::Cockpit);mem[MM]=manager_vtable(ManagerKind::MiniMap);
        mem[PL+0xE3C]=0;mem[NP+0xE3C]=1;mem[PL+0xE40]=mem[NP+0xE40]=1;
        mem[MAIN]=0x04E1642C;mem[SUB]=0x04E1649C;mem[MAIN+0x44]=PL;mem[SUB+0x44]=NP;
        mem[ACTIVE]=1;mem[TRACK]=NP;
        CHECK(source.start(7));CHECK(!source.start(7));
    }
    static bool read(void* c,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fixture*>(c);++f.reads;
        if(f.trigger==a){f.trigger=0;f.source.event(f.event_site,f.event_object);}
        const auto it=f.mem.find(a);if(it==f.mem.end())return false;*out=it->second;return true;
    }
    static u32 tid(void* c) noexcept {return static_cast<Fixture*>(c)->thread;}
    static u32 get_self(void* c) noexcept {
        auto& f=*static_cast<Fixture*>(c);++f.self_calls;
        if(f.reenter){f.reenter=false;CHECK(f.source.event(0x027B5D00,PL));}
        return f.self;
    }
    void bind(u32 pcs) {CHECK(source.event(BEGIN,pcs));CHECK(source.event(END,pcs));}
    void births() {
        CHECK(source.event(0x0277D4FB,NP));CHECK(source.event(0x0277D4FB,PL));mem[PL]=PLAYER;
        CHECK(source.event(0x02B47907,CO));CHECK(source.event(0x02B67F51,MM));
    }
    LifeSnapshot ready() {
        births();bind(MAIN);bind(SUB);CHECK(source.select_managers(CO,MM));
        LifeSnapshot s{};CHECK(source.capture(&s));CHECK(s.session.active);return s;
    }
    Token clone() {
        const auto t=registry.open_manager(CO,mem[CO],ManagerKind::Cockpit);CHECK(t.valid());
        const auto w=registry.bind_widget(t,0x900000,kind_info(WidgetKind::Reticle)->vtable,
            WidgetKind::Reticle,1,registry.session().sub0,1);CHECK(w.valid());return w;
    }
    void hidden() {CHECK(registry.resolve(0x900000,WidgetKind::Reticle).mode==Mode::Hidden);}
    void unchanged(LifeSnapshot& s) {s.revision=0xDEAD;CHECK(!source.capture(&s));CHECK(s.revision==0xDEAD);}
};
int main(){
    {Fixture f;LifeSnapshot s{};f.unchanged(s);f.bind(MAIN);f.bind(SUB);f.unchanged(s);CHECK(!f.source.select_managers(CO,MM));++scenarios;}
    {Fixture f;const auto a=f.ready();f.clone();f.bind(MAIN);f.bind(SUB);LifeSnapshot b{};CHECK(f.source.capture(&b));
     CHECK(a.session.epoch==b.session.epoch);CHECK(a.session.sub0.lifetime==b.session.sub0.lifetime);
     CHECK(f.registry.resolve(0x900000,WidgetKind::Reticle).mode==Mode::Local);++scenarios;}
    {Fixture f;auto a=f.ready();f.clone();const auto reads=f.reads;CHECK(f.source.event(0x027B5D00,PL));
     CHECK(f.reads==reads);f.hidden();const auto epoch=f.source.epoch();CHECK(f.source.event(0x0277DC70,PL));
     CHECK(epoch==f.source.epoch());CHECK(f.reads==reads);f.unchanged(a);++scenarios;}
    {Fixture f;auto a=f.ready();f.clone();CHECK(f.source.event(0x0277DC70,NP));f.bind(SUB);f.unchanged(a);f.hidden();
     CHECK(f.source.event(0x0277D4FB,NP));f.bind(SUB);LifeSnapshot b{};CHECK(f.source.capture(&b));
     CHECK(a.session.sub0.lifetime!=b.session.sub0.lifetime);f.hidden();++scenarios;}
    for(u32 site:{0x02B47A30u,0x02B68040u}){Fixture f;auto a=f.ready();f.clone();
     const bool mini=site==0x02B68040;const u32 ptr=mini?MM:CO;const auto n=f.reads;
     CHECK(f.source.event(site,ptr));CHECK(f.reads==n);f.hidden();f.unchanged(a);
     CHECK(f.source.event(mini?0x02B67F51:0x02B47907,ptr));CHECK(f.source.select_managers(CO,MM));
     LifeSnapshot b{};CHECK(f.source.capture(&b));CHECK(a.parent_lifetimes[mini]!=b.parent_lifetimes[mini]);++scenarios;}
    for(u32 site:{0x02DF5970u,0x02DF5F90u}){Fixture f;auto s=f.ready();f.clone();const auto n=f.reads;
     CHECK(f.source.event(site,site==0x02DF5970?MAIN:SUB));CHECK(f.reads==n);f.hidden();f.unchanged(s);++scenarios;}
    {Fixture f;auto a=f.ready();CHECK(f.source.event(BEGIN,SUB));LifeSnapshot s{};f.unchanged(s);
     CHECK(f.source.event(END,SUB));CHECK(f.source.capture(&s));CHECK(s.session.epoch==a.session.epoch);++scenarios;}
    {Fixture f;f.ready();CHECK(f.source.event(BEGIN,MAIN));CHECK(f.source.event(BEGIN,SUB));
     CHECK(f.source.event(END,SUB));CHECK(f.source.event(END,MAIN));LifeSnapshot s{};CHECK(f.source.capture(&s));++scenarios;}
    for(int invalid=0;invalid<8;++invalid){Fixture f;auto s=f.ready();f.clone();
     switch(invalid){case 0:f.mem[ACTIVE]=0;break;case 1:f.mem[TRACK]=PL;break;case 2:f.self=NP;break;
      case 3:f.mem[NP+0xE40]=3;break;case 4:f.mem[NP+0xE3C]=0xFFFFFFFF;break;
      case 5:f.mem[NP]=0xCAFEBABE;break;case 6:f.mem[MM]=0;break;case 7:f.mem.erase(NP+0xE3C);break;}
     f.unchanged(s);f.hidden();++scenarios;}
    {Fixture f;auto s=f.ready();f.clone();f.mem[ACTIVE]=0;f.unchanged(s);f.mem[ACTIVE]=1;
     LifeSnapshot b{};CHECK(f.source.capture(&b));CHECK(b.session.epoch!=s.session.epoch);f.hidden();++scenarios;}
    {Fixture f;auto a=f.ready();f.clone();f.mem[NP+0xE3C]=5;f.unchanged(a);f.bind(SUB);LifeSnapshot b{};
     CHECK(f.source.capture(&b));CHECK(b.session.sub0.serial==5);CHECK(b.session.epoch!=a.session.epoch);f.hidden();++scenarios;}
    {Fixture f;auto s=f.ready();f.clone();f.reenter=true;f.unchanged(s);f.hidden();++scenarios;}
    {Fixture f;auto s=f.ready();f.clone();f.trigger=NP+0xE3C;f.event_site=0x0277DC70;f.event_object=NP;
     f.unchanged(s);f.hidden();++scenarios;}
    {Fixture f;f.ready();LifeSnapshot s{};const auto reads=f.reads;f.thread=8;f.unchanged(s);
     CHECK(!f.source.event(0x0277DC70,NP));CHECK(f.reads==reads);f.thread=7;CHECK(f.source.capture(&s));++scenarios;}
    {Fixture f;f.ready();const auto n=f.reads;CHECK(!f.source.event(123,NP));CHECK(f.reads==n);
     CHECK(f.source.event(0x0277DC70,0xDEADBEEF));CHECK(f.reads==n);++scenarios;}
    {Fixture f;f.ready();f.mem[0x160000]=0x04E1650C;f.bind(0x160000);LifeSnapshot s{};CHECK(f.source.capture(&s));++scenarios;}
    {Fixture f;auto s=f.ready();f.clone();CHECK(!f.source.event(END,SUB));CHECK(f.source.fault()==SourceFault::Protocol);
     f.hidden();CHECK(!f.source.event(BEGIN,SUB));f.unchanged(s);++scenarios;}
    {Fixture f;f.ready();f.clone();CHECK(!f.source.event(0x0277D4FB,NP));CHECK(f.source.fault()==SourceFault::Protocol);f.hidden();++scenarios;}
    {Fixture f;f.ready();f.clone();for(u32 i=0;i<LifetimeSource::BindCapacity;++i)CHECK(f.source.event(BEGIN,SUB));
     CHECK(!f.source.event(BEGIN,SUB));CHECK(f.source.fault()==SourceFault::Capacity);f.hidden();++scenarios;}
    {Fixture f;f.ready();f.clone();for(u32 i=4;i<LifetimeSource::Capacity;++i){u32 p=0x200000+i*0x1000;f.mem[p]=NPC;CHECK(f.source.event(0x0277D4FB,p));}
     f.mem[0x700000]=NPC;CHECK(!f.source.event(0x0277D4FB,0x700000));CHECK(f.source.fault()==SourceFault::Capacity);f.hidden();++scenarios;}
    {Fixture f;auto s=f.ready();f.clone();f.mem[SUB+0x44]=0;f.bind(SUB);f.hidden();f.unchanged(s);
     f.mem[SUB+0x44]=NP;f.bind(SUB);CHECK(f.source.capture(&s));f.hidden();++scenarios;}
    // The former site must not remain accepted as a second begin notification.
    {Fixture f;auto before=f.ready();const auto reads=f.reads;
     CHECK(!f.source.event(0x02DF4F99,SUB));CHECK(f.reads==reads);
     CHECK(f.source.binder_depth()==0);CHECK(f.source.fault()==SourceFault::None);
     LifeSnapshot after{};CHECK(f.source.capture(&after));CHECK(before.session.epoch==after.session.epoch);++scenarios;}
    // Model both paths around +50. The gateway/byte auditor verify the actual
    // branch; the source must preserve identity for the early no-rebind path.
    for(u32 skip:{0u,1u,255u}){Fixture f;auto before=f.ready();f.clone();f.mem[SUB+0x50]=skip;
     CHECK(f.source.event(BEGIN,SUB));CHECK(f.source.binder_depth()==1);
     LifeSnapshot s{};f.unchanged(s);if(!skip)f.mem[SUB+0x44]=NP;
     CHECK(f.source.event(END,SUB));CHECK(f.source.binder_depth()==0);
     CHECK(f.source.fault()==SourceFault::None);CHECK(f.source.capture(&s));
     CHECK(before.session.epoch==s.session.epoch);CHECK(before.session.sub0.lifetime==s.session.sub0.lifetime);
     CHECK(f.registry.resolve(0x900000,WidgetKind::Reticle).mode==Mode::Local);++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"engine_calls_mocked\":true,\"gameplay_executed\":false}\n",scenarios,checks);
}
