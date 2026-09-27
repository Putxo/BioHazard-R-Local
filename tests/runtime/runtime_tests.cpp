#include "../../patches/runtime/runtime.hpp"
#include "../hud_ownership/january_fixture.hpp"
using rev_runtime::Runtime;
using rev_runtime::Host;
constexpr u32 OWNER=0x900000,MAIN=0xA00000,SUB=0xA10000;
struct Fixture {
    Fake f;Registry registry;u32 thread=7;bool image=true;
    u32 action_draws=0,action_member=0,action_reads=0;
    u32 aim_queries=0,aim_query_kind=0,aim_writes=0,aim_fail_write=0;
    Runtime r;
    Host host() {
        return {{this,[](void* p,u32 a,u32* v) noexcept {
            return Fake::read(&static_cast<Fixture*>(p)->f,a,v);
        }},[](void* p,u32 a,u32 v) noexcept {
            auto& z=*static_cast<Fixture*>(p);
            if(z.aim_fail_write && ++z.aim_writes>=z.aim_fail_write)return false;
            return Fake::write(&z.f,a,v);
        },[](void* p) noexcept {return static_cast<Fixture*>(p)->thread;},
        [](void* p) noexcept {return static_cast<Fixture*>(p)->image;},
        [](void* p) noexcept {return static_cast<Fixture*>(p)->f.session.self.address;},f.calls(),
        {this,[](void* p,u32) noexcept {
            auto& x=*static_cast<Fixture*>(p);++x.action_reads;return x.action_member;
        },[](void* p,u32,u32) noexcept {++static_cast<Fixture*>(p)->action_draws;}},
        [](void*,u32 raw) noexcept {return raw;},
        [](void* p,u32 actor) noexcept {
            auto& z=*static_cast<Fixture*>(p);++z.aim_queries;C(actor==z.f.session.sub0.address);
            switch(z.aim_query_kind){
            case 1:return false;
            case 2:z.f.m[0xEC100C]=0;break;
            case 3:z.f.m[actor+0xE40]=3;break;
            case 4:z.f.m[0xE30074]=z.f.session.self.address;break;
            case 5:z.r.aim_visibility(actor,0);break;
            }
            return true;
        }};
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
        if(count>=8) f.m[f.p[0]+0x54]=f.original[7];
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
    void prepare(){start();births();begin();C(f.allocations.empty());end();C(r.lifecycle().state()==LifeState::Live);C(f.allocations.size()==(f.original[7]?8u:f.original[6]?7u:f.original[5]?6u:f.original[4]?5u:f.original[3]?4u:LegacyWidgetKinds));}
    void equipment_setup(){
        f.m[0x04D2D440]=0x01BFECCD;
        for(u32 i=0;i<2;++i){
            const u32 a=i?f.session.sub0.address:f.session.self.address,p=0xEC0000+i*0x1000;
            f.m[a+0x1524]=p;f.m[p]=0x04D2D42C;f.m[p+0xD4]=2;
            for(u32 j=0;j<15;++j)f.m[p+4+4*j]=0;
            f.m[p+12]=0xED0000+i*0x1000;
        }
        f.m[r.lifecycle().unit(6)+0x2FC]=0xED1000;
    }
    void detector_setup(){
        prepare();begin();
        f.m[0xB00000+0xCE0]=0xE20000;f.m[0xB00000+0xCE4]=0xE30000;
        f.m[0xE30000]=0x04CF2B1C;f.m[0xE30000+0x74]=f.session.sub0.address;
        f.m[0xEF0000]=0x04DA8570;f.m[0xEF0024]=1;f.m[0xEF0010]=0x40400000;
        C(r.genesis_targets().event(0x0281AE26,0xEF0000));
        f.m[0xEF1000]=0x3F800000;f.m[0xEF1004]=0xC0000000;f.m[0xEF1008]=0;
        // Fake constructors mark widgets active. Native January initialization
        // leaves Scanner inactive; activation is deliberately a separate task.
        f.detector_context=this;
    }
};
int main(){
    {Fixture x(8);x.prepare();x.begin();const u32 scope=x.r.lifecycle().unit(7);C(scope!=0);
     u32 out=99;C(x.r.scope_actor(x.f.original[7],&out)==Mode::Stock);C(out==99);
     C(x.r.scope_actor(scope,&out)==Mode::Hidden);C(out==0);
     x.f.phase_observer_context=&x;x.f.phase_observer=[](void* p,u32,u32 unit) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(unit!=z.r.lifecycle().unit(7))return;
         u32 value=0;C(z.r.scope_actor(unit,&value)==Mode::Local);C(value==z.f.session.sub0.address);++z.action_reads;
     };
     C(x.r.driver().event(0x02B497CA,x.f.p[0],0));C(x.r.driver().event(0x02B49DE3,x.f.p[0],x.f.context));
     C(x.action_reads==2);x.f.phase_observer=nullptr;x.end();
     x.f.m[0x057D9188]=0;x.begin();x.end();C(x.f.deleted.size()==8);++scenarios;}
    for(u32 fault=0;fault<4;++fault){Fixture x(8);x.prepare();x.begin();const u32 scope=x.r.lifecycle().unit(7);
     switch(fault){
     case 0:x.f.m[scope+0x2AC]=x.f.m[x.f.original[7]+0x2AC];break;
     case 1:x.f.m[scope+0x2A4]=x.f.m[x.f.original[7]+0x2A4];break;
     case 2:x.f.m[scope+0x6100]+=4;break;
     case 3:x.f.m[x.f.original[7]+0x6100]+=4;break;
     }
     const auto before=x.f.events.size();C(!x.r.driver().event(0x02B497CA,x.f.p[0],0));
     for(size_t i=before;i<x.f.events.size();++i)C(x.f.events[i].self!=scope);
     ++scenarios;
    }

    for(u32 query:{0u,1u,5u}){Fixture x(7);x.detector_setup();x.equipment_setup();x.aim_query_kind=query;
     const u32 actor=x.f.session.sub0.address,af=0xF3FFBEAD,wf=0xA5FF1234;
     x.f.m[actor+12]=af;x.f.m[0xED100C]=wf;x.f.m[x.f.session.self.address+12]=af;
     x.f.writes.clear();x.r.aim_visibility(actor,1);
     C(x.f.m[actor+12]==(af&~0x20000u));C(x.f.m[0xED100C]==(query==1?wf:(wf&~0x20000u)));
     C(x.f.m[x.f.session.self.address+12]==af);C(x.aim_queries==1);
     C(x.f.writes.size()==(query==1?1u:2u));
     x.r.aim_visibility(actor,0);C(x.f.m[actor+12]==af);C(x.f.m[0xED100C]==wf);
     x.r.aim_visibility(actor,1);x.r.aim_visibility(actor,255);C(x.f.m[actor+12]==af);C(x.f.m[0xED100C]==wf);
     x.end();++scenarios;}
    for(u32 fault=0;fault<9;++fault){Fixture x(7);x.detector_setup();x.equipment_setup();
     const u32 actor=x.f.session.sub0.address; x.f.m[actor+12]=0xA5FF1111;x.f.m[0xED100C]=0xB6FF2222;
     switch(fault){
     case 0:x.aim_query_kind=2;break;
     case 1:x.aim_query_kind=3;break;
     case 2:x.aim_query_kind=4;break;
     case 3:x.f.m[0xEC0008]=0xED1000;break;
     case 4:x.f.m[0xE30074]=x.f.session.self.address;break;
     case 5:x.thread=99;break;
     case 6:x.f.reader_fail=actor+12;break;
     case 7:x.f.write_fail=0xED100C;break; // Roll back first write.
     case 8:x.f.write_fail=actor+12;break;
     }
     x.r.aim_visibility(actor,1);C(x.f.m[actor+12]==0xA5FF1111);C(x.f.m[0xED100C]==0xB6FF2222);
     ++scenarios;
    }
    {Fixture x(7);x.detector_setup();x.equipment_setup();const auto before=x.f.m;
     x.r.aim_visibility(x.f.session.self.address,1);C(x.f.m==before);C(x.aim_queries==0);
     x.r.aim_visibility(0,1);C(x.f.m==before);C(x.aim_queries==0);x.end();
     x.r.aim_visibility(x.f.session.sub0.address,1);C(x.f.m==before);C(x.aim_queries==0);++scenarios;}
    {Fixture x(7);x.detector_setup();x.equipment_setup();const u32 actor=x.f.session.sub0.address;
     x.f.m[0xEC100C]=0;x.f.m[actor+12]=0xA5FF1111;x.r.aim_visibility(actor,1);
     C(x.f.m[actor+12]==0xA5FD1111);C(x.aim_queries==0);x.end();++scenarios;}
    {Fixture x(7);x.detector_setup();x.equipment_setup();const u32 actor=x.f.session.sub0.address;
     x.f.m[actor+12]=0xA5FF1111;x.f.m[0xED100C]=0xB6FF2222;x.aim_fail_write=2;
     x.r.aim_visibility(actor,1);C(x.r.lifecycle().state()!=LifeState::Live);
     const auto calls=x.aim_queries;x.r.aim_visibility(actor,0);C(x.aim_queries==calls);++scenarios;}

    {Fixture x(7);x.detector_setup();x.equipment_setup();const auto before=x.f.m;
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==2);C(x.f.m==before);
     C(x.r.driver().event(0x02B49C5B,x.f.p[0],0));x.end();++scenarios;}
    for(u32 fault=0;fault<5;++fault){Fixture x(7);x.detector_setup();x.equipment_setup();
     const u32 unit=x.r.lifecycle().unit(6);
     switch(fault){
     case 0:x.f.m[0xEC100C]=0;break; // Weapon removed.
     case 1:x.f.m[0xEC10D4]=15;break; // No equipped slot.
     case 2:x.f.m[0xEC0008]=0xED1000;break; // Shared with another P1 slot.
     case 3:x.f.m[x.f.session.sub0.address+0x1524]=0xEC0000;break;
     case 4:x.f.m[unit+0x2FC]=0xED0000;break; // Stock player's weapon.
     }
     const auto before=x.f.m;x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==1);C(x.f.m==before);
     const auto events=x.f.events.size();C(!x.r.driver().event(0x02B49C5B,x.f.p[0],0));
     for(size_t i=events;i<x.f.events.size();++i)C(x.f.events[i].self!=unit);
     C(x.r.lifecycle().state()!=LifeState::Live);++scenarios;
    }
    {Fixture x(7);x.detector_setup();x.equipment_setup();
     x.f.phase_observer_context=&x;x.f.phase_observer=[](void* p,u32,u32 unit) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(unit==z.r.lifecycle().unit(6))z.f.m[0xEC100C]=0;
     };
     C(!x.r.driver().event(0x02B49C5B,x.f.p[0],0));C(x.r.lifecycle().state()!=LifeState::Live);++scenarios;}
    {Fixture x(7);x.detector_setup();x.equipment_setup();
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(z.f.detector_calls==2)z.f.m[0xEC100C]=0;
     };
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==2);C(x.r.lifecycle().state()!=LifeState::Live);++scenarios;}

    for(u32 variant=0;variant<2;++variant){Fixture x(7);x.detector_setup();x.action_member=variant;x.f.m[0xED0004]=0x12345678;
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);
         if(z.f.detector_calls==1){C(!z.r.genesis_notify(0xEF0000,0xEC0000,0xED0000));Fake::method0(&z.f,0x01C315EC,0xED0000);return;}
         if(z.action_member)z.f.m[0xED0004]=0x23456789;
         C(z.r.genesis_notify(0xEF0000,z.action_member?0xEC0000:0xEC1000,0xED0000));
     };
     x.r.genesis_detect(0xEE0000);C(x.f.notification_calls==2);x.end();++scenarios;}
    {Fixture x(7);x.detector_setup();const u32 unit=x.r.lifecycle().unit(6);
     scanner_fixture::collections(x.f.m,unit,1);x.f.m[0xEF0008]=0;x.f.m[0xED0004]=0x12345678;
     x.f.notification_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);C(z.r.genesis_targets().event(0x02823A53,0xEF0000));
         z.r.genesis_remove_target(0xEF0000);C(z.f.removal_calls==1);
     };
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(z.f.detector_calls==1)return;
         C(z.r.genesis_notify(0xEF0000,0xEC0000,0xED0000));
         C(!z.r.genesis_candidate(0xEF0000));u32 v=0;
         C(z.r.genesis_detector_widget(&v)==Mode::Local);C(v==z.r.lifecycle().unit(6));
     };
     x.r.genesis_detect(0xEE0000);C(x.f.notification_calls==1);
     C(x.r.lifecycle().state()==LifeState::Live);x.end();++scenarios;}
    {Fixture x(7);x.detector_setup();x.f.m[0xED0004]=0x12345678;
     C(!x.r.genesis_notify(0xEF0000,0xEC0000,0xED0000));
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);
         const bool handled=z.r.genesis_notify(0xEF0000,0xEC0000,0xED0000);
         if(z.f.detector_calls%2==1){C(!handled);Fake::method0(&z.f,0x01C315EC,0xED0000);}
         else C(handled);
     };
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==2);C(x.f.notification_calls==1);
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==4);C(x.f.notification_calls==2);
     x.end();++scenarios;}
    {Fixture x(7);x.detector_setup();x.f.m[0xED0004]=0x12345678;
     x.f.notification_observer=[](void* p,u32 delegate) noexcept {
         auto& z=*static_cast<Fixture*>(p);C(delegate==0xED0000);u32 v=99;
         C(z.r.genesis_detector_actor(&v)==Mode::Stock);C(v==99);
         C(z.r.genesis_target_focus(0xEF0000,&v)==Mode::Stock);C(v==99);
         C(z.r.genesis_set_focus(0xEF0000,1)==Mode::Stock);
         z.r.genesis_detect(0xEE0000);
     };
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);u32 v=99;
         if(z.f.detector_calls!=2){C(z.r.genesis_detector_actor(&v)==Mode::Stock);return;}
         C(z.r.genesis_set_position(0xEF0000,0xEF1000)==Mode::Local);
         C(z.r.genesis_set_focus(0xEF0000,3)==Mode::Local);
         C(z.r.genesis_notify(0xEF0000,0xEC0000,0xED0000));C(z.f.notification_calls==1);
         C(z.r.genesis_detector_actor(&v)==Mode::Local);C(v==z.f.session.sub0.address);
         C(z.r.genesis_target_focus(0xEF0000,&v)==Mode::Local);C(v==3);
         C(z.r.genesis_notify(0xEF0000,0xEC0000,0xED0000));C(z.f.notification_calls==1);
         C(z.r.genesis_notify(0xDD0000,0xEC0000,0xED0000));C(z.f.notification_calls==1);
     };
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==3);C(x.f.notification_calls==1);
     x.end();++scenarios;}
    {Fixture x(7);x.detector_setup();x.f.m[0xED0004]=0x12345678;
     x.f.notification_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);C(z.r.source().event(0x0277DC70,z.f.session.sub0.address));
     };
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(z.f.detector_calls==1)return;
         C(z.r.genesis_notify(0xEF0000,0xEC0000,0xED0000));u32 v=99;
         C(z.r.genesis_detector_widget(&v)==Mode::Hidden);C(v==0);
         C(!z.r.genesis_candidate(0xEF0000));
     };
     x.r.genesis_detect(0xEE0000);C(x.f.notification_calls==1);C(x.r.lifecycle().state()!=LifeState::Live);
     x.end();++scenarios;}
    {Fixture x(7);x.detector_setup();const u32 unit=x.r.lifecycle().unit(6);
     scanner_fixture::collections(x.f.m,unit,3);scanner_fixture::collections(x.f.m,x.f.original[6],3);
     x.f.m[0xEF0008]=0;C(x.r.genesis_targets().event(0x02823A53,0xEF0000));
     x.f.m[unit+0xC]&=~0x4000u;x.f.m[unit+0x294]=1;
     x.end();const auto before=x.f.m;
     x.r.genesis_remove_target(0xEF0000);C(x.f.removal_calls==1);C(x.f.removal_unit==unit);
     rev_genesis::Collections after{};C(rev_genesis::capture_collections(x.host().memory,unit,&after));
     C(after.targets[0]==0);C(after.targets[1]==0xEF0100);C(after.targets[2]==0xEF0200);
     C(after.icon_targets[0]==0);C(after.active_targets==6);C(after.active_icons==6);
     const u32 stock=x.f.original[6];
     for(const auto& kv:before)if(kv.first>=stock && kv.first<stock+0xE000)C(x.f.m[kv.first]==kv.second);
     x.r.genesis_remove_target(0xEF0000);C(x.f.removal_calls==1);
     C(x.r.lifecycle().state()==LifeState::Live);++scenarios;}
    for(u32 fault=0;fault<5;++fault){Fixture x(7);x.detector_setup();const u32 unit=x.r.lifecycle().unit(6);
     scanner_fixture::collections(x.f.m,unit,1);x.f.m[0xEF0008]=0;
     C(x.r.genesis_targets().event(0x02823A53,0xEF0000));
     switch(fault){
     case 0:x.f.m[0xEF0008]=0xABCD0000;break;
     case 1:x.f.m[unit+0x324+0xC]=x.f.original[6]+0xD008;break;
     case 2:x.image=false;break;
     case 3:x.f.removal_noop=true;break;
     case 4:x.thread=9;break;
     }
     x.r.genesis_remove_target(0xEF0000);C(x.f.removal_calls==(fault==3?1u:0u));
     if(fault!=4){C(x.r.lifecycle().state()!=LifeState::Live);x.image=true;
         C(x.r.clock().event(PipelineEnd,0xC00000,0xD00000));
         C(!x.r.window().event(PipelineEnd,0xC00000,0xD00000));
         C(x.r.lifecycle().state()==LifeState::Quarantined);
     }
     ++scenarios;
    }
    {Fixture x(7);x.detector_setup();const auto before=x.f.m;
     x.f.detector_observer=[](void* p,u32 manager) noexcept {
         auto& z=*static_cast<Fixture*>(p);C(manager==0xEE0000);u32 v=99;
         if(z.f.detector_calls==1){
             C(z.r.genesis_detector_camera(0xB00000,&v)==Mode::Stock);C(v==99);
             C(z.r.genesis_detector_actor(&v)==Mode::Stock);C(z.r.genesis_detector_widget(&v)==Mode::Stock);
             C(z.r.genesis_set_position(0xEF0000,0xEF1000)==Mode::Stock);
             C(z.r.genesis_set_focus(0xEF0000,3)==Mode::Stock);C(z.r.genesis_candidate(0));return;
         }
         C(z.f.detector_calls==2);
         C(z.r.genesis_detector_camera(0xB00000,&v)==Mode::Local);C(v==0xE30000);
         C(z.r.genesis_detector_actor(&v)==Mode::Local);C(v==z.f.session.sub0.address);
         C(z.r.genesis_detector_widget(&v)==Mode::Local);C(v==z.r.lifecycle().unit(6));
         C(!z.r.genesis_candidate(0xEF0000));C(!z.r.genesis_candidate(0xDD0000));
         C(z.r.genesis_set_position(0xDD0000,0xEF1000)==Mode::Hidden);
         C(z.r.genesis_set_position(0xEF0000,0xEF1000)==Mode::Local);
         C(z.r.genesis_target_focus(0xEF0000,&v)==Mode::Local);C(v==0);
         C(z.r.genesis_set_focus(0xEF0000,3)==Mode::Local);
         C(z.r.genesis_target_focus(0xEF0000,&v)==Mode::Local);C(v==3);
         C(z.r.genesis_candidate(0xEF0000));
         C(z.r.genesis_set_position(0xEF0000,0xEF1000)==Mode::Local);
         C(z.r.genesis_target_focus(0xEF0000,&v)==Mode::Local);C(v==0);
         C(z.r.genesis_set_focus(0xEF0000,2)==Mode::Local);
     };
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==2);C(x.f.m==before);
     u32 v=99;C(x.r.genesis_target_focus(0xEF0000,&v)==Mode::Stock);C(v==99);
     rev_genesis::ViewSample out{};
     C(x.r.genesis_target_views().read(x.r.lifecycle().unit(6),x.r.genesis_targets().capture(0xEF0000),&out)==Mode::Local);
     C(out.focus==2);C(out.position[0]==0x3F800000);C(out.position[1]==0xC0000000);
     x.end();++scenarios;}
    for(u32 fault=0;fault<8;++fault){Fixture x(7);x.detector_setup();const u32 unit=x.r.lifecycle().unit(6);
     switch(fault){
     case 0:x.f.m[unit+0xC]&=~0x4000u;break;
     case 1:x.f.m[0xB00000+0xCE4]=0;break;
     case 2:x.thread=9;break;
     case 3:x.image=false;break;
     case 4:C(!x.r.genesis_targets().event(0x0281AE26,0xEF0000));break;
     case 5:x.f.m[unit+0x3B8]=x.f.m[x.f.original[6]+0x3B8];break;
     case 6:x.f.m[unit+0x294]=1;break;
     case 7:x.end();break;
     }
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);u32 v=99;
         C(z.r.genesis_detector_actor(&v)==Mode::Stock);C(v==99);
     };
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==1);++scenarios;
    }
    {Fixture x(7);x.detector_setup();
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);u32 v=99;
         if(z.f.detector_calls==2){
             C(z.r.genesis_set_position(0xEF0000,0xEF1000)==Mode::Local);
             C(z.r.genesis_set_focus(0xEF0000,3)==Mode::Local);
             z.r.genesis_detect(0xEE0000);
             C(z.r.genesis_detector_actor(&v)==Mode::Local);C(v==z.f.session.sub0.address);
             C(z.r.genesis_target_focus(0xEF0000,&v)==Mode::Local);C(v==3);
         }else{
             C(z.r.genesis_detector_actor(&v)==Mode::Stock);C(v==99);
             C(z.r.genesis_target_focus(0xEF0000,&v)==Mode::Stock);C(v==99);
         }
     };
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==3);x.end();++scenarios;}
    {Fixture x(7);x.detector_setup();
     x.f.detector_observer=[](void* p,u32) noexcept {
         auto& z=*static_cast<Fixture*>(p);if(z.f.detector_calls==1)return;
         C(z.r.genesis_set_position(0xEF0000,0xEF1000)==Mode::Local);
         C(z.r.genesis_set_position(0xEF0000,0xFFFFFFFF)==Mode::Hidden);
         u32 v=99;C(z.r.genesis_detector_widget(&v)==Mode::Hidden);C(v==0);
         C(!z.r.genesis_candidate(0xEF0000));
         C(z.r.genesis_set_focus(0xEF0000,2)==Mode::Hidden);
     };
     x.r.genesis_detect(0xEE0000);C(x.f.detector_calls==2);C(x.r.lifecycle().state()!=LifeState::Live);
     x.end();++scenarios;}
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
