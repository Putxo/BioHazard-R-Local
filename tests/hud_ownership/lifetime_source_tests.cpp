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
    u32 input_calls=0,input_kind=Invalid,input_slot=Invalid,input_fault=0,input_restores=0;
    bool input_preserved=false;
    LifetimeSource source;
    Fixture():source(registry,{this,read,tid,get_self,update_input}) {
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
    static void update_input(void* c,u32 pad,u32 kind) noexcept {
        if(kind==2){auto& f=*static_cast<Fixture*>(c);++f.input_restores;
            CHECK(f.source.input_index(pad,9)==1);
            if(f.mem[pad+0x738]==6)f.mem[pad+0x738]=f.mem[pad+0x73C];
            return;
        }
        auto& f=*static_cast<Fixture*>(c);++f.input_calls;f.input_kind=kind;
        f.input_slot=f.source.input_index(pad,f.mem[pad+0x970]);
        if(f.input_fault==1)CHECK(f.source.event(0x0277DC70,NP));
        if(f.input_fault==2)f.source.input_update(pad,kind);
        if(f.input_fault==3){++f.thread;CHECK(f.source.input_index(pad,9)==9);--f.thread;}
        CHECK(f.source.input_index(pad,f.mem[pad+0x970])==f.input_slot);
        CHECK(f.source.input_index(pad+4,9)==9);
        f.input_preserved=f.source.preserve_other_pad(pad,pad+0x490,0,0x2C);
        // Synthetic poll -> clear -> keyboard-write transport; no game code.
        if(kind==1){
            for(u32 slot=0;slot<2;++slot){const u32 dest=pad+0x198+slot*0x2F8;
                if(!f.source.preserve_other_pad(pad,dest,0,0x2C))f.mem[dest]=0;}
            f.mem[pad+0x198+f.input_slot*0x2F8]=0x1234;
        }
    }
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
    // Native Invalid0 is suspension, not loss of a previously observed pair.
    for(u32 p1:{0u,1u})for(u32 p2:{0u,1u})for(bool nested:{false,true}){
        Fixture f;f.births();f.bind(MAIN);f.bind(SUB);const u32 epoch=f.source.epoch();
        f.mem[PL+0xE40]=p1;f.mem[NP+0xE40]=p2;const auto before=f.mem;
        if(nested){CHECK(f.source.event(BEGIN,MAIN));CHECK(f.source.event(BEGIN,SUB));
            CHECK(f.source.event(END,SUB));CHECK(f.source.event(END,MAIN));}
        else {f.bind(MAIN);f.bind(SUB);}
        CHECK(f.source.epoch()==epoch);CHECK(f.source.local_think_mode(NP,3)==1);
        CHECK(f.source.local_think_mode(NP,0)==0);CHECK(f.mem==before);
        f.mem[PL+0xE40]=f.mem[NP+0xE40]=1;
        CHECK(f.source.select_managers(CO,MM));LifeSnapshot s{};CHECK(f.source.capture(&s));
        CHECK(s.session.sub0.address==NP && s.session.self.address==PL);++scenarios;
    }
    for(u32 a:{PL,NP}){Fixture f;f.ready();f.mem[a+0xE40]=0;f.bind(a==PL?MAIN:SUB);
        LifeSnapshot s{};CHECK(!f.source.capture(&s));CHECK(f.source.local_think_mode(NP,3)==1);
        f.mem[a+0xE40]=1;CHECK(f.source.capture(&s));++scenarios;}
    for(u32 a:{PL,NP}){Fixture f;f.births();f.mem[a+0xE40]=0;f.bind(MAIN);f.bind(SUB);
        CHECK(f.source.local_think_mode(NP,3)==3);
        f.mem[a+0xE40]=1;CHECK(f.source.local_think_mode(NP,3)==3);++scenarios;}
    for(u32 bad=0;bad<12;++bad){Fixture f;f.ready();f.mem[NP+0xE40]=0;
        switch(bad){
        case 0:f.mem[ACTIVE]=0;break;case 1:f.mem[TRACK]=PL;break;
        case 2:f.mem[NP+0xE3C]=2;break;case 3:f.mem[NP]=PLAYER;break;
        case 4:f.mem[PL+0xE3C]=5;break;case 5:f.mem[PL+0xE40]=2;break;
        case 6:f.mem[NP+0xE40]=2;break;case 7:f.mem[NP+0xE40]=3;break;
        case 8:CHECK(f.source.event(0x0277DC70,NP));CHECK(f.source.event(0x0277D4FB,NP));break;
        case 9:CHECK(f.source.local_think_mode(NP,2)==2);break;
        case 10:f.mem[MAIN+0x44]=NP;break;case 11:f.mem.erase(NP+0xE3C);break;
        }
        f.bind(SUB);CHECK(f.source.local_think_mode(NP,3)==3);
        // A later raw mode change cannot repair ownership after rejected rebind.
        f.mem[NP+0xE40]=1;f.mem[NP+0xE3C]=1;f.mem[NP]=NPC;f.mem[PL+0xE40]=1;
        f.mem[PL+0xE3C]=0;f.mem[ACTIVE]=1;f.mem[TRACK]=NP;f.mem[MAIN+0x44]=PL;
        CHECK(f.source.local_think_mode(NP,3)==3);++scenarios;
    }
    {Fixture f;f.ready();f.mem[NP+0xE40]=0;f.mem[SUB+0x10000]=f.mem[SUB];
        f.mem[SUB+0x10044]=NP;f.bind(SUB+0x10000);f.mem[NP+0xE40]=1;
        CHECK(f.source.local_think_mode(NP,3)==3);++scenarios;}
    for(u32 at:{ACTIVE,TRACK,NP+0xE40,PL+0xE3C}){Fixture f;f.ready();f.mem[NP+0xE40]=0;
        CHECK(f.source.event(BEGIN,SUB));f.trigger=at;f.event_site=0x0277DC70;f.event_object=NP;
        CHECK(!f.source.event(END,SUB));CHECK(f.source.local_think_mode(NP,3)==3);
        CHECK(f.source.binder_depth()==0);++scenarios;
    }
    constexpr u32 GP=0x160000,GP_GLOBAL=0x057A7480;
    auto input=[=](Fixture& f,u32 primary){
        f.mem[GP_GLOBAL]=GP;f.mem[GP]=0x04E11D30;
        f.mem[GP+0x970]=primary;f.mem[GP+0x974]=1;
    };
    for(u32 primary:{0u,1u})for(u32 kind:{0u,1u})for(u32 fault=0;fault<4;++fault){
        Fixture f;f.births();f.bind(MAIN);f.bind(SUB);input(f,primary);f.input_fault=fault;
        f.mem[GP+0x198]=0x1111;f.mem[GP+0x490]=0x2222;
        CHECK(f.source.input_index(GP,9)==9);f.source.input_update(GP,kind);
        CHECK(f.input_calls==1 && f.input_kind==kind && f.input_slot==0 && f.input_preserved);
        CHECK(f.input_restores==(kind==0?1u:0u));
        CHECK(f.mem[GP+0x970]==primary);CHECK(f.mem[GP+0x490]==0x2222);
        CHECK(f.mem[GP+0x198]==(kind?0x1234u:0x1111u));CHECK(f.source.input_index(GP,9)==9);
        if(fault==1){f.source.input_update(GP,0);CHECK(f.input_slot==primary);}
        ++scenarios;
    }
    for(u32 bad=0;bad<10;++bad){
        Fixture f;f.births();f.bind(MAIN);f.bind(SUB);input(f,1);
        switch(bad){case 0:f.mem[GP_GLOBAL]=GP+4;break;case 1:f.mem[GP]=0;break;
          case 2:f.mem[GP+0x970]=2;break;case 3:f.mem[ACTIVE]=0;break;
          case 4:f.mem[NP+0xE40]=2;break;case 5:f.mem[NP+0xE40]=3;break;
          case 6:f.thread=8;break;case 7:f.mem[NP+0xE3C]=2;break;
          case 8:CHECK(f.source.event(BEGIN,SUB));break;case 9:f.mem.erase(GP_GLOBAL);break;}
        f.source.input_update(GP,0);CHECK(f.input_calls==1);CHECK(f.input_slot==f.mem[GP+0x970]);
        CHECK(f.source.input_index(GP,9)==9);++scenarios;
    }
    for(u32 at:{GP_GLOBAL,GP,GP+0x970}){Fixture f;f.ready();input(f,1);f.trigger=at;
        f.event_site=0x0277DC70;f.event_object=NP;f.source.input_update(GP,0);
        CHECK(f.input_calls==1 && f.input_slot==1);++scenarios;}
    {Fixture f;input(f,1);f.source.input_update(GP,0);CHECK(f.input_calls==1 && f.input_slot==1);
     f.source.input_update(GP,2);CHECK(f.input_calls==1);++scenarios;}
    for(u32 layout:{0u,1u,5u,6u}){Fixture f;f.births();f.bind(MAIN);f.bind(SUB);input(f,1);
      f.mem[GP+0x738]=layout;f.mem[GP+0x73C]=4;f.source.input_update(GP,0);
      CHECK(f.mem[GP+0x738]==(layout==6?4u:layout));CHECK(f.mem[GP+0x970]==1);++scenarios;}
    for(u32 primary:{0u,1u})for(u32 mode:{0u,1u}){
        Fixture f;f.births();f.bind(MAIN);f.bind(SUB);input(f,primary);
        f.mem[PL+0xE40]=f.mem[NP+0xE40]=mode;const auto before=f.mem;
        CHECK(f.source.preserve_other_pad(GP,GP+0x198+(1-primary)*0x2F8,0,0x2C));
        CHECK(!f.source.preserve_other_pad(GP,GP+0x198+primary*0x2F8,0,0x2C));
        CHECK(f.mem==before);++scenarios;
    }
    for(u32 bad=0;bad<14;++bad){
        Fixture f;f.ready();input(f,0);
        u32 gp=GP,dest=GP+0x490,value=0,size=0x2C;
        switch(bad){case 0:gp=0;break;case 1:gp=0xFFFFF800;break;
          case 2:++dest;break;case 3:value=1;break;case 4:--size;break;
          case 5:f.mem[GP_GLOBAL]=GP+4;break;case 6:f.mem[GP]=0;break;
          case 7:f.mem[GP+0x970]=2;break;case 8:f.mem[GP+0x974]=0;break;
          case 9:f.mem.erase(GP+0x970);break;case 10:f.mem[ACTIVE]=0;break;
          case 11:f.mem[NP+0xE40]=2;break;case 12:f.mem[NP+0xE40]=3;break;
          case 13:f.thread=8;break;}
        const auto before=f.mem;CHECK(!f.source.preserve_other_pad(gp,dest,value,size));
        CHECK(f.mem==before);++scenarios;
    }
    for(u32 at:{GP_GLOBAL,GP,GP+0x970,GP+0x974}){
        Fixture f;f.ready();input(f,0);f.trigger=at;
        f.event_site=0x0277DC70;f.event_object=NP;
        CHECK(!f.source.preserve_other_pad(GP,GP+0x490,0,0x2C));++scenarios;
    }
    {Fixture f;input(f,0);CHECK(!f.source.preserve_other_pad(GP,GP+0x490,0,0x2C));
     f.births();f.bind(MAIN);f.bind(SUB);CHECK(f.source.event(BEGIN,SUB));
     CHECK(!f.source.preserve_other_pad(GP,GP+0x490,0,0x2C));++scenarios;}

    // Control ownership is established by two observed PCS binds, before HUD.
    for(u32 p1:{0u,1u})for(u32 p2:{0u,1u}){
        Fixture f;f.births();f.bind(MAIN);f.bind(SUB);
        f.mem[PL+0xE40]=p1;f.mem[NP+0xE40]=p2;const auto before=f.mem;
        CHECK(f.source.local_think_mode(NP,3)==1);CHECK(f.mem==before);
        CHECK(f.source.local_think_mode(PL,3)==3);
        CHECK(f.source.local_think_mode(NP,0)==0);
        CHECK(f.source.local_think_mode(NP,1)==1);
        CHECK(f.source.local_think_mode(NP,0xFFFFFFFFu)==0xFFFFFFFFu);++scenarios;
    }
    for(u32 mode:{2u,3u,4u,0xFFFFFFFFu})for(u32 actor:{NP,PL}){
        Fixture f;f.ready();f.mem[actor+0xE40]=mode;
        CHECK(f.source.local_think_mode(NP,3)==3);++scenarios;
    }
    {Fixture f;f.births();f.bind(MAIN);f.bind(SUB);
     CHECK(f.source.local_think_mode(NP,2)==2);
     CHECK(f.source.local_think_mode(NP,3)==3); // request itself revoked lease
     f.bind(SUB);CHECK(f.source.local_think_mode(NP,3)==1);++scenarios;}
    {Fixture f;f.births();CHECK(f.source.local_think_mode(NP,3)==3);
     f.bind(SUB);CHECK(f.source.local_think_mode(NP,3)==3);
     f.bind(MAIN);CHECK(f.source.local_think_mode(NP,3)==1);++scenarios;}
    for(u32 site:{0x0277DC70u,0x027B5D00u,0x02DF5970u,0x02DF5F90u}){
        Fixture f;f.ready();const u32 object=site==0x0277DC70?NP:site==0x027B5D00?PL:site==0x02DF5970?MAIN:SUB;
        CHECK(f.source.event(site,object));CHECK(f.source.local_think_mode(NP,3)==3);++scenarios;
    }
    {Fixture f;f.ready();CHECK(f.source.event(0x0277DC70,NP));
     CHECK(f.source.event(0x0277D4FB,NP));CHECK(f.source.local_think_mode(NP,3)==3);
     f.bind(SUB);CHECK(f.source.local_think_mode(NP,3)==1);++scenarios;}
    for(int bad=0;bad<10;++bad){Fixture f;f.ready();
     switch(bad){case 0:f.mem[ACTIVE]=0;break;case 1:f.mem[TRACK]=PL;break;
       case 2:f.mem[SUB+0x44]=PL;break;case 3:f.mem[MAIN+0x44]=NP;break;
       case 4:f.mem[NP+0xE3C]=8;break;case 5:f.mem[PL+0xE3C]=8;break;
       case 6:f.mem[SUB]=0;break;case 7:f.mem[MAIN]=0;break;
       case 8:f.mem[NP]=PLAYER;break;case 9:f.mem.erase(NP+0xE40);break;}
     CHECK(f.source.local_think_mode(NP,3)==3);++scenarios;}
    {Fixture f;f.ready();const auto reads=f.reads;f.thread=8;
     CHECK(f.source.local_think_mode(NP,3)==3);CHECK(f.reads==reads);
     f.thread=7;CHECK(f.source.event(BEGIN,SUB));
     CHECK(f.source.local_think_mode(NP,3)==3);CHECK(f.source.event(END,SUB));
     CHECK(f.source.local_think_mode(NP,3)==1);++scenarios;}
    for(u32 trigger:{ACTIVE,TRACK,MAIN,MAIN+0x44,PL,PL+0xE3C,PL+0xE40,SUB,SUB+0x44,NP,NP+0xE3C,NP+0xE40}){
        Fixture f;f.ready();f.trigger=trigger;f.event_site=0x0277DC70;f.event_object=NP;
        CHECK(f.source.local_think_mode(NP,3)==3);++scenarios;
    }
    {Fixture f;f.ready();CHECK(f.source.event(0x02B47A30,CO));CHECK(f.source.event(0x02B68040,MM));
     CHECK(f.source.local_think_mode(NP,3)==1);++scenarios;}
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
