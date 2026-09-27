#include "../../patches/runtime/runtime.hpp"
#include "../hud_ownership/january_fixture.hpp"
using rev_runtime::Runtime;
using rev_runtime::Host;
constexpr u32 OWNER=0x900000,MAIN=0xA00000,SUB=0xA10000;
struct Fixture {
    Fake f;Registry registry;u32 thread=7;bool image=true;
    u32 action_draws=0,action_member=0,action_reads=0;
    Runtime r;
    Host host() {
        return {{this,[](void* p,u32 a,u32* v) noexcept {
            return Fake::read(&static_cast<Fixture*>(p)->f,a,v);
        }},[](void* p,u32 a,u32 v) noexcept {
            return Fake::write(&static_cast<Fixture*>(p)->f,a,v);
        },[](void* p) noexcept {return static_cast<Fixture*>(p)->thread;},
        [](void* p) noexcept {return static_cast<Fixture*>(p)->image;},
        [](void* p) noexcept {return static_cast<Fixture*>(p)->f.session.self.address;},f.calls(),
        {this,[](void* p,u32) noexcept {
            auto& x=*static_cast<Fixture*>(p);++x.action_reads;return x.action_member;
        },[](void* p,u32,u32) noexcept {++static_cast<Fixture*>(p)->action_draws;}},
        [](void*,u32 raw) noexcept {return raw;}};
    }
    Fixture(u32 count=LegacyWidgetKinds):f(count),r(registry,host(),count) {
        f.life=&r.lifecycle();
        f.m[0x055623C4]=OWNER;f.m[OWNER+0x28]=f.p[0];f.m[OWNER+0x2C]=f.p[1];
        f.m[f.p[0]+0x90]=f.original[0];f.m[f.p[0]+0x5C]=f.original[1];
        f.m[f.p[1]+0x30]=f.m[f.p[1]+0x40]=f.original[2];
        if(count==WidgetKinds) f.m[f.p[0]+0x64]=f.original[3];
        f.m[MAIN]=0x04E1642C;f.m[SUB]=0x04E1649C;
        f.m[MAIN+0x44]=f.session.self.address;f.m[SUB+0x44]=f.session.sub0.address;
        f.m[0x057D9188]=1;f.m[0x057D9184]=f.session.sub0.address;
        f.m[f.context]=0x04F79014;f.m[0x05799D3C]=0xB00000;
        f.m[0xB00000]=0x04EC9AA8;
        f.m[0xB00000+0x1D0]=0x03000101;f.m[0xB00000+0x1D4]=0;
        const u32 rect[]={0,360,1280,720};
        for(u32 i=0;i<4;++i)f.m[0xB00000+0x1D8+i*4]=f.m[f.context+0xBC+i*4]=rect[i];
    }
    void start(){C(r.start());C(!r.start());}
    void births() {
        for(u32 a:{f.session.self.address,f.session.sub0.address}){
            f.m[a]=0x04D9B25C;f.m[a+0xE3C]=a==f.session.self.address?0:1;f.m[a+0xE40]=1;
            C(r.source().event(0x0277D4FB,a));
        }
        C(r.source().event(0x02B47907,f.p[0]));C(r.source().event(0x02B67F51,f.p[1]));
        for(u32 pcs:{MAIN,SUB}){C(r.source().event(0x02DF4F65,pcs));C(r.source().event(0x02DF504B,pcs));}
    }
    void begin(){C(r.clock().event(PipelineBegin,0xC00000,0xD00000));C(r.activator().event(PipelineBegin,0xC00000,0xD00000));}
    void end(){C(r.clock().event(PipelineEnd,0xC00000,0xD00000));C(r.window().event(PipelineEnd,0xC00000,0xD00000));}
    void prepare(){start();births();begin();C(f.allocations.empty());end();C(r.lifecycle().state()==LifeState::Live);C(f.allocations.size()==(f.original[3]?WidgetKinds:LegacyWidgetKinds));}
};
int main(){
    {Fixture x;x.prepare();
     constexpr u32 icon=0xF00000,command=0xF10000,manager=0xF20000;
     x.f.m[icon]=0x04CDCA9C;x.f.m[icon+0x40]=command;x.f.m[command]=0x04CDA850;
     for(u32 o:{0x34u,0x38u,0x3Cu})x.f.m[command+o]=0;
     x.f.m[0x0556279C]=manager;x.f.m[manager]=0x04CDBDB4;x.f.m[manager+0x174]=1;
     x.f.m[x.f.context+0x158]=1;x.action_member=1;
     C(x.r.action().draw(icon,x.f.context)==rev_action::Result::Refused);
     x.begin();C(x.r.action().draw(icon,x.f.context)==rev_action::Result::Refused);
     constexpr u32 stack=0xF30000;
     x.f.m[stack-8]=manager;x.f.m[stack-0x100]=0;x.f.m[stack-0x10C]=0;x.f.m[stack-0x128]=0;
     x.r.priority().begin(stack);x.r.priority().prepare(stack,command);
     C(x.r.priority().decide(stack,true)==rev_action::Decision::Proceed);
     x.f.m[stack-0x100]=0x201;x.f.m[stack-0x10C]=3;
     C(x.r.priority().commit(stack));x.r.priority().finish(stack);
     C(x.r.action().draw(icon,x.f.context)==rev_action::Result::Drawn);C(x.action_draws==1);
     x.f.m[x.f.context+0x158]=0;C(x.r.action().draw(icon,x.f.context)==rev_action::Result::Skipped);
     x.thread=8;C(x.r.action().draw(icon,x.f.context)==rev_action::Result::Refused);C(x.action_reads==4);
     x.thread=7;C(x.r.source().event(0x0277DC70,x.f.session.sub0.address));
     C(x.r.action().draw(icon,x.f.context)==rev_action::Result::Refused);C(x.action_draws==1);
     x.f.m[0x057D9188]=0;C(x.r.action().draw(icon,x.f.context)==rev_action::Result::Stock);C(x.action_draws==2);
     x.end();++scenarios;}
    {Fixture x;x.image=false;C(!x.r.start());C(!x.r.bind());C(x.f.allocations.empty());++scenarios;}
    {Fixture x;x.thread=0;C(!x.r.start());C(!x.r.bind());++scenarios;}
    {Fixture x;x.start();x.begin();x.end();C(x.f.allocations.empty());C(x.r.lifecycle().state()==LifeState::Empty);++scenarios;}
    {Fixture x;x.start();x.births();C(!x.r.structural(1,0xC00000,0xD00000));C(x.f.allocations.empty());++scenarios;}
    {Fixture x;x.prepare();C(x.r.activator().attached_ticket()==0);x.begin();C(x.r.activator().attached_ticket()!=0);
     const auto original=x.f.m;
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.r.driver().event(0x02B6847B,x.f.p[1],0));
     C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));C(x.r.driver().event(0x02B685B3,x.f.p[1],0));
     C(x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));C(x.r.driver().event(0x02B68724,x.f.p[1],x.f.context));
     C(x.f.events.size()==12);
     for(u32 i=0;i<3;++i){const u32 unit=x.r.lifecycle().unit(i);C(x.registry.resolve(unit,static_cast<WidgetKind>(i)).actor==x.f.session.sub0.address);
         C(draw_view(x.f.m[unit+0xC])==draw_view(original.at(unit+0xC)));}
     for(u32 u:x.f.original)if(u)C(x.f.m[u+0xC]==original.at(u+0xC));
     x.end();C(x.f.allocations.size()==3);++scenarios;}
    {Fixture x;x.prepare();x.begin();x.f.m[0x055623C4]=0;x.end();C(x.r.lifecycle().state()==LifeState::Empty);
     C(x.f.deleted.size()==3);C(x.registry.widget_count()==0);x.f.m[0x055623C4]=OWNER;x.begin();x.end();
     C(x.r.lifecycle().state()==LifeState::Live);C(x.f.allocations.size()==6);++scenarios;}
    {Fixture x;x.prepare();const u32 old=x.r.lifecycle().unit(0);x.begin();C(x.r.source().event(0x0277DC70,x.f.session.sub0.address));
     C(x.registry.resolve(old,WidgetKind::Reticle).mode==Mode::Hidden);x.end();C(x.f.deleted.size()==3);++scenarios;}
    {Fixture x;x.start();x.births();x.f.m[OWNER+0x28]=0xBAD000;x.begin();x.end();C(x.f.allocations.empty());
     x.f.m[OWNER+0x28]=x.f.p[0];x.begin();x.end();C(x.f.allocations.size()==3);++scenarios;}
    {Fixture x;x.prepare();x.begin();x.f.m[OWNER+0x2C]=0;x.end();C(x.f.deleted.size()==3);++scenarios;}
    {Fixture x;x.prepare();x.begin();const auto count=x.f.events.size();x.thread=8;
     C(!x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.f.events.size()==count);x.thread=7;x.end();++scenarios;}
    {Fixture x;x.start();const u32 pad=0xE00000;x.f.m[pad+0x970]=0;x.f.m[pad+0x1A0]=0;x.f.m[pad+0x2F8+0x1A0]=1;
     x.f.m[x.f.session.sub0.address+0xE3C]=1;x.f.m[x.f.session.sub0.address+0xE40]=1;
     C(x.r.menu().submenu_open_word(pad)==1);C(x.r.menu().owner()==1);
     C(x.r.menu().submenu_actor(x.f.session.self.address)==x.f.session.sub0.address);++scenarios;}
    {Fixture x(WidgetKinds);x.prepare();x.begin();
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));
     C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));
     C(x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));
     const u32 u=x.r.lifecycle().unit(3);C(u!=0);
     C(x.registry.resolve(u,WidgetKind::SubEquipment).actor==x.f.session.sub0.address);
     C(x.f.events.size()==13);x.f.m[0x055623C4]=0;x.end();
     C(x.f.deleted.size()==4);C(x.registry.widget_count()==0);++scenarios;}
    {Fixture x(WidgetKinds);x.start();x.births();x.f.m[x.f.p[0]+0x64]=x.f.original[1];
     x.begin();C(x.r.clock().event(PipelineEnd,0xC00000,0xD00000));
     C(!x.r.window().event(PipelineEnd,0xC00000,0xD00000));C(x.f.allocations.empty());++scenarios;}
    {Fixture x(WidgetKinds);x.start();x.births();x.f.no_init=3;x.begin();
     C(x.r.clock().event(PipelineEnd,0xC00000,0xD00000));
     C(!x.r.window().event(PipelineEnd,0xC00000,0xD00000));
     C(x.f.deleted.size()==4);C(x.registry.widget_count()==0);++scenarios;}
    {Fixture x(WidgetKinds+1);C(!x.r.start());C(x.f.allocations.empty());++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"engine_calls_mocked\":true,\"gameplay_executed\":false}\n",scenarios,checks);
}
