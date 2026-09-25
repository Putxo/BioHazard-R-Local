#include "january_fixture.hpp"
#include "pipeline_frame_provider.hpp"
struct Fixture {
    Fake engine;
    Registry registry;
    u32 thread=7,reads=0,enters=0,leaves=0,scope_depth=0;
    int self_action=0,enter_action=0;
    bool enter_result=true,leave_result=true;
    PipelineClock clock;
    LifetimeSource source;
    PipelineFrameProvider provider;
    JanuaryBackend backend;
    Lifecycle life;
    ManagerDriver driver;
    static constexpr u32 Main=0x800000, Sub=0x810000, Owner=0x900000, Frame=0xA00000;
    Fixture() : clock({this,tid}), source(registry,{this,read,tid,self}),
        provider(clock,source,{{this,read},tid,enter,leave}), backend(engine.host(),engine.calls()),
        life(registry,backend.callbacks()), driver(life,provider.callbacks()) {
        engine.life=&life;
        C(clock.start(7)); C(source.start(7));
    }
    bool begin() {return clock.event(PipelineBegin,Owner,Frame);}
    bool end() {return clock.event(PipelineEnd,Owner,Frame);}
    static u32 tid(void* c) noexcept {return static_cast<Fixture*>(c)->thread;}
    static bool read(void* c,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fixture*>(c);++f.reads;return Fake::read(&f.engine,a,out);
    }
    static u32 self(void* c) noexcept {
        auto& f=*static_cast<Fixture*>(c);const int action=f.self_action;f.self_action=0;
        if(action==1 || action==2) {C(f.end());if(action==2)C(f.begin());}
        if(action==3) {ManagerFrame out{};out.frame=999;C(!f.provider.sample(&out));C(out.frame==999);}
        if(action==4) C(f.source.event(0x0277DC70,f.engine.session.sub0.address));
        return f.engine.session.self.address;
    }
    static bool enter(void* c,const ManagerSite&,const ManagerFrame& frame,u32) noexcept {
        auto& f=*static_cast<Fixture*>(c);++f.enters;C(frame.frame>0);
        if(!f.enter_result)return false;
        ++f.scope_depth;
        if(f.enter_action==1 || f.enter_action==2) {C(f.end());if(f.enter_action==2)C(f.begin());}
        if(f.enter_action==3) {ManagerFrame out{};out.frame=999;C(!f.provider.sample(&out));C(out.frame==999);}
        return true;
    }
    static bool leave(void* c) noexcept {
        auto& f=*static_cast<Fixture*>(c);++f.leaves;C(f.scope_depth==1);--f.scope_depth;return f.leave_result;
    }
    void births() {
        for(u32 u:{engine.session.self.address,engine.session.sub0.address}) {
            engine.m[u]=0x04D9B25C;engine.m[u+0xE3C]=u==engine.session.self.address?0:1;
            engine.m[u+0xE40]=1;C(source.event(0x0277D4FB,u));
        }
        engine.m[engine.session.self.address]=0x04D9CDF4;
        C(source.event(0x02B47907,engine.p[0]));C(source.event(0x02B67F51,engine.p[1]));
        engine.m[Main]=0x04E1642C;engine.m[Sub]=0x04E1649C;
        engine.m[Main+0x44]=engine.session.self.address;engine.m[Sub+0x44]=engine.session.sub0.address;
        engine.m[0x057D9188]=1;engine.m[0x057D9184]=engine.session.sub0.address;
        for(u32 p:{Main,Sub}) {C(source.event(0x02DF4F65,p));C(source.event(0x02DF504B,p));}
        C(source.select_managers(engine.p[0],engine.p[1]));
    }
    void ready() {
        births();LifeSnapshot s{};C(source.capture(&s));engine.session=s.session;
        C(backend.configure(engine.p,engine.original));const u32 t=life.prepare(s.session,engine.p,engine.original);
        C(t);C(life.publish(t,s.session));C(begin());C(driver.attach(t));engine.events.clear();
    }
    bool run(u32 site) {
        const auto* s=manager_site(site);C(s);
        return driver.event(site,engine.p[static_cast<u32>(s->manager)],s->phase==11?engine.context:0);
    }
};
int main() {
    {Fixture f;f.births();ManagerFrame out{};out.frame=99;const auto n=f.reads;C(!f.provider.sample(&out));
     C(f.reads==n && out.frame==99);C(f.begin());C(f.provider.sample(&out));C(out.frame==1);
     C(out.session.sub0.address==f.engine.session.sub0.address);C(out.parents[1]==f.engine.p[1]);
     C(out.parent_lifetimes[0] && out.parent_lifetimes[1]);
     for(int i=0;i<20;++i){ManagerFrame repeat{};C(f.provider.sample(&repeat));C(repeat.frame==out.frame);}
     C(f.end());C(!f.provider.sample(&out));C(out.frame==1);++scenarios;}
    {Fixture f;f.ready();const u32 sites[]={0x02B497CA,0x02B6847B,0x02B49C5B,0x02B685B3,0x02B49DE3,0x02B68724};
     for(u32 s:sites){C(f.run(s));}C(f.engine.events.size()==9);const auto n=f.engine.events.size();
     for(u32 s:sites){C(!f.run(s));}C(f.engine.events.size()==n);C(f.enters==f.leaves && f.scope_depth==0);
     C(f.end());C(f.begin());C(f.run(0x02B49DE3));C(f.run(0x02B68724));C(f.engine.events.size()==n+3);
     C(f.enters==f.leaves);C(f.end());++scenarios;}
    for(int action:{1,2,4}) {Fixture f;f.births();C(f.begin());f.self_action=action;
     ManagerFrame out{};out.frame=777;out.parents[0]=888;
     C(!f.provider.sample(&out));C(out.frame==777 && out.parents[0]==888);
     if(action==2){C(f.provider.sample(&out));C(out.frame==2);}++scenarios;}
    {Fixture f;f.births();C(f.begin());f.self_action=3;ManagerFrame out{};C(f.provider.sample(&out));C(out.frame==1);++scenarios;}
    for(int action:{1,2}) {Fixture f;f.ready();f.enter_action=action;
     C(!f.run(0x02B497CA));C(f.engine.events.empty());C(f.enters==1 && f.leaves==1 && !f.scope_depth);
     C(!f.provider.scope_fault());++scenarios;}
    {Fixture f;f.ready();f.enter_action=1;f.leave_result=false;
     C(!f.run(0x02B497CA));C(f.provider.scope_fault());C(f.engine.events.empty());C(f.leaves==1);
     C(f.begin());C(!f.run(0x02B497CA));C(f.life.state()==LifeState::Draining);C(f.leaves==1);++scenarios;}
    {Fixture f;f.ready();f.enter_result=false;C(!f.run(0x02B497CA));C(f.engine.events.empty());C(f.leaves==0);++scenarios;}
    {Fixture f;f.ready();f.leave_result=false;C(!f.run(0x02B497CA));C(f.provider.scope_fault());
     C(f.engine.events.size()==2);C(f.life.state()==LifeState::Draining);C(f.scope_depth==0);++scenarios;}
    {Fixture f;f.ready();f.enter_action=3;C(f.run(0x02B497CA));C(f.engine.events.size()==2);C(f.enters==f.leaves);++scenarios;}
    {Fixture f;f.births();C(f.begin());ManagerFrame out{};C(f.provider.sample(&out));auto h=f.provider.callbacks();
     ManagerFrame wrong=out;++wrong.session.sub0.serial;C(!h.enter_scope(h.memory.context,*manager_site(0x02B497CA),wrong,0));
     wrong=out;++wrong.parent_lifetimes[1];C(!h.enter_scope(h.memory.context,*manager_site(0x02B497CA),wrong,0));
     C(f.enters==0);C(h.enter_scope(h.memory.context,*manager_site(0x02B497CA),out,0));
     C(!h.enter_scope(h.memory.context,*manager_site(0x02B497CA),out,0));C(h.leave_scope(h.memory.context));
     C(!h.leave_scope(h.memory.context));C(f.enters==1 && f.leaves==1);++scenarios;}
    {Fixture f;f.births();C(f.begin());ManagerFrame old{};C(f.provider.sample(&old));C(f.end());C(f.begin());auto h=f.provider.callbacks();
     C(!h.enter_scope(h.memory.context,*manager_site(0x02B497CA),old,0));C(f.enters==0);++scenarios;}
    {Fixture f;f.ready();const auto reads=f.reads;f.thread=8;C(!f.run(0x02B497CA));C(f.reads==reads);C(f.engine.events.empty());
     f.thread=7;C(f.run(0x02B497CA));++scenarios;}
    for(u32 site:{0x02B47A30u,0x0277DC70u}) {Fixture f;f.ready();
     C(f.source.event(site,site==0x02B47A30?f.engine.p[0]:f.engine.session.sub0.address));
     C(!f.run(0x02B497CA));C(f.engine.events.empty());C(f.life.state()==LifeState::Draining);C(f.engine.deleted.empty());++scenarios;}
    {Fixture f;f.ready();C(f.end());C(!f.run(0x02B497CA));C(f.life.state()==LifeState::Draining);C(f.engine.events.empty());C(f.engine.deleted.empty());++scenarios;}
    {Fixture f;f.births();C(f.begin());PipelineFrameProvider missing(f.clock,f.source,{});
     auto h=missing.callbacks();ManagerFrame out{};C(h.sample(h.memory.context,&out));C(h.thread_id(h.memory.context)==0);
     u32 word=123;C(!h.memory.word(h.memory.context,1,&word));C(word==123);
     C(!h.enter_scope(h.memory.context,*manager_site(0x02B497CA),out,0));C(!h.leave_scope(h.memory.context));++scenarios;}
    {Fixture f;auto h=f.provider.callbacks();u32 out=0;C(h.memory.word(h.memory.context,f.engine.p[0],&out));
     C(out==manager_vtable(ManagerKind::Cockpit));C(!h.memory.word(h.memory.context,f.engine.p[0],nullptr));C(h.thread_id(h.memory.context)==7);++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"actual_lifetime_clock_provider_manager_lifecycle_backend\":true,\"engine_calls_mocked\":true,\"gameplay_executed\":false}\n",scenarios,checks);
}
