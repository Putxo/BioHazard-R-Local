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
        if(count>=4) f.m[f.p[0]+0x64]=f.original[3];
        if(count>=5) f.m[f.p[0]+0x3C]=f.original[4];
        if(count>=6) f.m[f.p[0]+0x44]=f.original[5];
        if(count>=7) f.m[f.p[0]+0x48]=f.original[6];
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
    void prepare(){start();births();begin();C(f.allocations.empty());end();C(r.lifecycle().state()==LifeState::Live);C(f.allocations.size()==(f.original[6]?7u:f.original[5]?6u:f.original[4]?5u:f.original[3]?4u:LegacyWidgetKinds));}
};
int main(){
    {Fixture x(7);x.prepare();x.begin();constexpr u32 target=0xEF0000,dest=0xEF1000;
     x.f.m[target]=0x04DA8570;x.f.m[target+0x24]=1;x.f.m[target+0x10]=0x40400000;
     for(u32 i=0;i<4;++i)x.f.m[dest+i*4]=0xCCCCCCCC;
     C(x.r.genesis_targets().event(0x0281AE26,target));const auto key=x.r.genesis_targets().capture(target);
     const u32 widget=x.r.lifecycle().unit(6);rev_genesis::ViewSample sample{{0x3F800000,0xC0000000,0},3};
     C(x.r.genesis_target_views().publish(widget,key,sample)==Mode::Local);
     u32 focus=99;C(x.r.genesis_target_focus(target,&focus)==Mode::Stock);C(focus==99);
     C(x.r.genesis_target_position(target,dest)==Mode::Stock);C(x.f.m[dest]==0xCCCCCCCC);
     x.f.phase_observer_context=&x;x.f.phase_observer=[](void* p,u32,u32 unit) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(unit!=z.r.lifecycle().unit(6))return;
         u32 v=99;C(z.r.genesis_target_focus(0xEF0000u,&v)==Mode::Local);C(v==3);
         C(z.r.genesis_target_position(0xEF0000u,0xEF1000u)==Mode::Local);
         C(z.f.m[0xEF1000u]==0x3F800000);C(z.f.m[0xEF1000u+4]==0xC0000000);C(z.f.m[0xEF1000u+8]==0);C(z.f.m[0xEF1000u+12]==0);
         C(z.f.m[0xEF0000u+0x24]==1);C(z.f.m[0xEF0000u+0x10]==0x40400000);
     };
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));
     C(x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));
     focus=99;C(x.r.genesis_target_focus(target,&focus)==Mode::Stock);C(focus==99);
     x.f.phase_observer=nullptr;x.end();++scenarios;}
    {Fixture x(7);x.prepare();x.begin();constexpr u32 dest=0xEF1000;
     for(u32 i=0;i<4;++i)x.f.m[dest+i*4]=0xCCCCCCCC;
     x.f.phase_observer_context=&x;x.f.phase_observer=[](void* p,u32,u32 unit) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(unit!=z.r.lifecycle().unit(6))return;
         u32 v=99;C(z.r.genesis_target_focus(0xEF0000u,&v)==Mode::Hidden);C(v==0);
         C(z.r.genesis_target_position(0xEF0000u,0xEF1000u)==Mode::Hidden);
         for(u32 i=0;i<4;++i)C(z.f.m[0xEF1000u+i*4]==0);
     };
     C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));x.f.phase_observer=nullptr;x.end();++scenarios;}
    {Fixture x(7);x.prepare();x.begin();constexpr u32 target=0xEF0000,dest=0xEF1000;
     for(u32 i=0;i<4;++i)x.f.m[dest+i*4]=0xCCCCCCCC;
     x.f.write_fail=dest+8;x.f.phase_observer_context=&x;x.f.phase_observer=[](void* p,u32,u32 unit) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(unit!=z.r.lifecycle().unit(6))return;
         C(z.r.genesis_target_position(0xEF0000u,0xEF1000u)==Mode::Hidden);
         for(u32 i=0;i<4;++i)C(z.f.m[0xEF1000u+i*4]==0xCCCCCCCC);
     };
     C(!x.r.driver().event(0x02B49C5B,x.f.p[0],0));x.f.phase_observer=nullptr;x.f.write_fail=0;
     u32 v=99;C(x.r.genesis_target_focus(target,&v)==Mode::Stock);C(v==99);
     C(x.r.clock().event(PipelineEnd,0xC00000,0xD00000));
     C(!x.r.window().event(PipelineEnd,0xC00000,0xD00000));
     C(x.r.lifecycle().state()==LifeState::Quarantined);++scenarios;}

    {Fixture x(7);x.prepare();x.begin();constexpr u32 target=0xEF0000;x.f.m[target]=0x04DA8570;
     C(x.r.genesis_targets().event(0x0281AE26,target));const auto key=x.r.genesis_targets().capture(target);
     const u32 widget=x.r.lifecycle().unit(6);rev_genesis::ViewSample sample{{0x3F800000,0,0},2},out{};
     const auto before=x.f.m;
     C(x.r.genesis_target_views().publish(widget,key,sample)==Mode::Local);
     C(x.r.genesis_target_views().read(widget,key,&out)==Mode::Local);C(out.focus==2);C(out.position[0]==0x3F800000);C(x.f.m==before);
     C(x.r.genesis_target_views().read(x.f.original[6],key,&out)==Mode::Stock);
     x.thread=9;C(x.r.genesis_target_views().read(widget,key,&out)==Mode::Hidden);C(out.focus==0);x.thread=7;
     C(x.r.genesis_targets().event(0x02823A53,target));C(x.r.genesis_target_views().read(widget,key,&out)==Mode::Hidden);
     x.end();++scenarios;}

    {Fixture x;x.start();constexpr u32 target=0xEF0000;x.f.m[target]=0x04DA8570;
     C(x.r.genesis_targets().event(0x0281AE26,target));auto key=x.r.genesis_targets().capture(target);
     C(key.valid());C(x.r.genesis_targets().live(key));x.f.m.erase(target);
     C(x.r.genesis_targets().event(0x02823A53,target));C(!x.r.genesis_targets().live(key));++scenarios;}

    {Fixture x(7);x.prepare();x.begin();const u32 widget=x.r.lifecycle().unit(6);
     constexpr u32 camera_manager=0xE10000,camera_self=0xE20000,camera_sub=0xE30000;
     x.f.m[0x05799D3C]=camera_manager;x.f.m[camera_manager+0xCE0]=camera_self;
     x.f.m[camera_manager+0xCE4]=camera_sub;x.f.m[camera_sub]=0x04CF2B1C;
     x.f.m[camera_sub+0x74]=x.f.session.sub0.address;
     const auto before=x.f.m;u32 out=0;
     C(x.r.genesis_camera(widget,camera_manager,&out)==Mode::Local);C(out==camera_sub);C(x.f.m==before);
     C(x.r.genesis_camera(x.f.original[6],camera_manager,&out)==Mode::Stock);
     x.thread=9;C(x.r.genesis_camera(widget,camera_manager,&out)==Mode::Hidden);C(out==0);x.thread=7;
     x.f.m[camera_sub+0x74]=x.f.session.self.address;
     C(x.r.genesis_camera(widget,camera_manager,&out)==Mode::Hidden);C(out==0);
     x.f.m[camera_sub+0x74]=x.f.session.sub0.address;
     C(x.r.source().event(0x0277DC70,x.f.session.sub0.address));
     C(x.r.genesis_camera(widget,camera_manager,&out)==Mode::Hidden);C(out==0);x.end();++scenarios;}

    {Fixture x(6);x.prepare();x.begin();const u32 p2=x.f.session.sub0.address;
     C(x.r.heal_event(x.f.session.self.address));C(!x.r.heal_event(p2));C(!x.r.heal_event(p2));
     C(x.f.heal_calls==0);C(x.r.driver().event(0x02B497CA,x.f.p[0],0));
     C(x.f.heal_calls==1);C(x.f.heal_unit==x.r.lifecycle().unit(5));
     C(x.f.heal_unit!=x.f.original[5]);
     C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));
     C(x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));C(x.f.heal_calls==1);
     C(!x.r.heal_event(p2));x.end();x.begin();
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.f.heal_calls==2);
     x.f.m[0x055623C4]=0;x.end();C(x.f.deleted.size()==6);++scenarios;}
    {Fixture x(6);x.prepare();x.begin();
     C(!x.r.heal_event(0x123456));C(!x.r.heal_event(0));
     x.thread=9;C(!x.r.heal_event(x.f.session.sub0.address));x.thread=7;
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.f.heal_calls==0);
     C(!x.r.heal_event(x.f.session.sub0.address));
     C(x.r.source().event(0x0277DC70,x.f.session.sub0.address));
     C(!x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.f.heal_calls==0);
     x.end();++scenarios;}
    {Fixture x;x.start();x.f.m[0x057D9188]=0;
     C(x.r.heal_event(0x12345));C(x.f.heal_calls==0);++scenarios;}

    {Fixture x(5);x.prepare();x.begin();const auto original=x.f.m;
     const u32 unit=x.r.lifecycle().unit(4);C(unit!=0);
     C(x.registry.resolve(unit,WidgetKind::Damage).actor==x.f.session.sub0.address);
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));
     C(x.f.damage_calls==1);C(x.f.damage_unit==unit);C(x.f.damage_actor==x.f.session.sub0.address);
     C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));
     C(x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));C(x.f.damage_calls==1);
     for(u32 u:x.f.original)if(u)C(x.f.m[u+0xC]==original.at(u+0xC));
     C(!x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.f.damage_calls==1);
     x.f.m[0x055623C4]=0;x.end();C(x.f.deleted.size()==5);++scenarios;}
    {Fixture x(5);x.prepare();x.begin();x.f.damage_ok=false;
     C(!x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.f.damage_calls==1);
     C(!x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));
     x.end();++scenarios;}
    {Fixture x(5);x.prepare();x.begin();x.thread=8;
     C(!x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.f.damage_calls==0);
     x.thread=7;C(x.r.source().event(0x0277DC70,x.f.session.sub0.address));
     C(!x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.f.damage_calls==0);
     x.end();++scenarios;}

    {Fixture x;x.prepare();C(x.r.draw_schedule(x.f.context,0)==0);x.begin();
     for(u32 view:{0u,1u}){x.f.m[x.f.context+0x158]=view;C(x.r.draw_schedule(x.f.context,0)==1);}
     x.f.m[x.f.context+0x158]=2;C(x.r.draw_schedule(x.f.context,0)==0);
     x.f.m[x.f.context+0x158]=1;x.thread=8;C(x.r.draw_schedule(x.f.context,0)==0);
     x.thread=7;C(x.r.source().event(0x0277DC70,x.f.session.sub0.address));
     C(x.r.draw_schedule(x.f.context,0)==0);C(x.r.draw_schedule(x.f.context,0xAB000001)==1);
     x.end();++scenarios;}
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
    {Fixture x(4);x.prepare();x.begin();
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));
     C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));
     C(x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));
     const u32 u=x.r.lifecycle().unit(3);C(u!=0);
     C(x.registry.resolve(u,WidgetKind::SubEquipment).actor==x.f.session.sub0.address);
     C(x.f.events.size()==13);x.f.m[0x055623C4]=0;x.end();
     C(x.f.deleted.size()==4);C(x.registry.widget_count()==0);++scenarios;}
    {Fixture x(4);x.start();x.births();x.f.m[x.f.p[0]+0x64]=x.f.original[1];
     x.begin();C(x.r.clock().event(PipelineEnd,0xC00000,0xD00000));
     C(!x.r.window().event(PipelineEnd,0xC00000,0xD00000));C(x.f.allocations.empty());++scenarios;}
    {Fixture x(4);x.start();x.births();x.f.no_init=3;x.begin();
     C(x.r.clock().event(PipelineEnd,0xC00000,0xD00000));
     C(!x.r.window().event(PipelineEnd,0xC00000,0xD00000));
     C(x.f.deleted.size()==4);C(x.registry.widget_count()==0);++scenarios;}
    {Fixture x(WidgetKinds+1);C(!x.r.start());C(x.f.allocations.empty());++scenarios;}
    {Fixture x(7);x.prepare();x.begin();
     const u32 scanner=x.r.lifecycle().unit(6);C(scanner!=0);
     C(x.registry.resolve(scanner,WidgetKind::Scanner).actor==x.f.session.sub0.address);
     using O=rev_genesis::Operation;u32 v=0;
     C(x.r.genesis_progress().access(scanner,O::Add,38,&v)==Mode::Local);C(v==38);
     C(x.r.genesis_progress().access(x.f.original[6],O::Set,0,&v)==Mode::Stock);
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));
     C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));
     C(x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));
     x.thread=8;C(x.r.genesis_progress().access(scanner,O::Add,1,&v)==Mode::Hidden);
     x.thread=7;C(x.r.genesis_progress().access(scanner,O::Read,0,&v)==Mode::Local);C(v==38);
     C(x.r.source().event(0x0277DC70,x.f.session.sub0.address));
     C(x.r.genesis_progress().access(scanner,O::Read,0,&v)==Mode::Hidden);
     x.end();C(x.f.deleted.size()==7);++scenarios;}
    {Fixture x(7);x.prepare();x.begin();const u32 scanner=x.r.lifecycle().unit(6);
     x.f.m[scanner+0x3b8]=x.f.m[x.f.original[6]+0x3b8];
     const auto before=x.f.events.size();C(!x.r.driver().event(0x02B49C5B,x.f.p[0],0));
     for(size_t i=before;i<x.f.events.size();++i)C(x.f.events[i].self!=scanner);
     C(x.r.clock().event(PipelineEnd,0xC00000,0xD00000));
     C(!x.r.window().event(PipelineEnd,0xC00000,0xD00000));
     C(x.r.lifecycle().state()==LifeState::Quarantined);++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"engine_calls_mocked\":true,\"gameplay_executed\":false}\n",scenarios,checks);
}
