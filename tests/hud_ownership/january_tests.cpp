#include "january_fixture.hpp"
struct Fixture {
    Fake f;Registry registry;JanuaryBackend native;Lifecycle life;Backend b;
    Fixture():native(f.host(),f.calls()),life(registry,native.callbacks()),b(native.callbacks()) {f.life=&life;C(native.configure(f.p,f.original));}
    u32 start(){u32 t=life.prepare(f.session,f.p,f.original);C(t);C(life.publish(t,f.session));return t;}
};
static void end(Fixture& f){f.life.stop();C(f.life.collect());C(f.registry.widget_count()==0);}
int main(){
    {Fixture f;const auto original=f.f.m;u32 t=f.start();C(f.f.ctor_count==3 && f.f.init_count==3);
     for(u32 phase:{8u,9u,11u}){C(f.life.dispatch(t,f.f.session,1,phase,1,f.f.context));C(!f.life.dispatch(t,f.f.session,1,phase,1,f.f.context));}
     C(f.f.events.size()==12);C(f.f.m[f.life.unit(0)+0x1C]==0x3F800000);C(f.f.m[f.life.unit(2)+0x1C]==0x3F000000);
     for(const auto& kv:original)C(f.f.m[kv.first]==kv.second);
     for(u32 i=0;i<3;++i)C(draw_view(f.f.m[f.life.unit(i)+0xC])==3);
     const u32 last=f.life.unit(2);end(f);C(f.f.deleted.size()==3);C(f.f.deleted[0]==last);++scenarios;}
    for(int fail=0;fail<3;++fail){Fixture f;f.f.no_alloc=fail;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.deleted.size()==static_cast<u32>(fail));++scenarios;}
    for(int fail=0;fail<3;++fail){Fixture f;f.f.no_init=fail;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.deleted.size()==static_cast<u32>(fail+1));C(f.life.state()==LifeState::Empty);++scenarios;}
    for(u32 alias:{0x100000u,0x100010u,0x200000u,0x120000u}){Fixture f;f.f.allocation_override=alias;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(!f.f.ctor_count);C(f.f.deleted.empty());++scenarios;}
    {Fixture f;f.f.fresh=false;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(!f.f.ctor_count);C(f.native.retained(WidgetKind::Reticle)==0x600000);C(!f.native.configure(f.f.p,f.f.original));++scenarios;}
    {Fixture f;f.f.bad_ctor=0;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.init_count==0);C(f.f.deleted.empty());C(f.native.retained(WidgetKind::Reticle)!=0);++scenarios;}
    {Fixture f;f.f.attached=0x600000;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.init_count==0);C(f.f.deleted.empty());C(f.native.fault()==NativeFault::Attached);++scenarios;}
    {Fixture f;f.f.unit_available=false;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.init_count==0);C(f.f.deleted.empty());++scenarios;}
    for(u32 off:{0x14u,0x18u,0x290u}){Fixture f;u32 t=f.start();f.f.m[f.life.unit(0)+off]=1;C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(!f.life.collect());C(f.life.state()==LifeState::Quarantined);C(f.f.deleted.size()==2);++scenarios;}
    {Fixture f;u32 t=f.start();const u32 u=f.life.unit(0);f.f.attached=u;C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(!f.life.collect());C(f.life.unit(0)==u);C(f.registry.resolve(u,WidgetKind::Reticle).mode==Mode::Hidden);++scenarios;}
    {Fixture f;u32 t=f.start();f.f.m[f.life.unit(0)+0xC]&=~0x3F8u;C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(!f.life.collect());++scenarios;}
    {Fixture f;u32 t=f.start();const auto events=f.f.events.size();f.f.m[f.life.unit(0)+0x294]=0x123401;C(f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(f.f.events.size()==events+2);end(f);++scenarios;}
    {Fixture f;u32 t=f.start();const auto events=f.f.events.size();f.f.m[f.life.unit(0)+0x294]=0xABCDEF00;C(f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(f.f.events.size()==events+3);end(f);++scenarios;}
    {Fixture f;u32 t=f.start();auto n=f.f.events.size();f.f.m[f.life.unit(0)+0xC]&=~0x4000u;f.f.m[f.life.unit(2)+0xC]&=~0x4000u;
     C(f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(f.f.events.size()==n+2);n=f.f.events.size();
     C(f.life.dispatch(t,f.f.session,1,11,1,f.f.context));C(f.f.events.size()==n+1);end(f);++scenarios;}
    {Fixture f;u32 t=f.start();f.f.draw_admission=false;C(!f.life.dispatch(t,f.f.session,1,11,1,f.f.context));C(f.life.state()==LifeState::Draining);C(f.life.collect());++scenarios;}
    {Fixture f;u32 t=f.start();f.f.m[f.f.context+0x158]=0;C(!f.life.dispatch(t,f.f.session,1,11,1,f.f.context));C(f.life.collect());++scenarios;}
    {Fixture f;u32 t=f.start();f.f.m[f.f.p[0]+0x1C]=0x7FC00000;C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(f.life.collect());++scenarios;}
    {Fixture f;u32 t=f.start();f.f.m[0x04DE52C4+8*4]=0xDEADBEEF;C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(!f.life.collect());C(f.life.unit(0)!=0);++scenarios;}
    {Fixture f;f.start();const u32 u=f.life.unit(0);f.life.stop();f.f.deny_destroy=true;C(!f.life.collect());C(f.f.deleted.empty());C(f.life.unit(0)==u);C(f.registry.widget_count()==3);++scenarios;}
    {Fixture f;f.f.deny_init=true;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.init_count==0);C(f.f.deleted.size()==1);++scenarios;}
    {Fixture f;f.f.admission=false;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.allocations.empty());++scenarios;}
    {Fixture f;f.f.stop_in_init=true;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.deleted.size()==1);++scenarios;}
    {Fixture f;f.f.stop_in_construct=true;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.f.deleted.size()==1);C(f.f.init_count==0);++scenarios;}
    {Fixture f;u32 t=f.start();f.f.stop_in_phase=true;C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(f.f.events.size()==4);C(f.f.deleted.empty());C(f.life.collect());++scenarios;}
    {Fixture f;u32 t=f.start();const u32 u=f.life.unit(0);f.f.reader_fail=u+0x14;C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(!f.life.collect());C(f.life.unit(0)==u);++scenarios;}
    {Fixture f;u32 t=f.start();f.f.write_fail=f.life.unit(0)+0x1C;C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(draw_view(f.f.m[f.life.unit(0)+0xC])==3);C(f.life.collect());++scenarios;}
    {Fixture f;C(!f.b.write_word(f.b.memory.context,f.f.original[0]+0xC,0));C(f.f.writes.empty());++scenarios;}
    {Fixture f;u32 t=f.start();C(!f.native.configure(f.f.p,f.f.original));C(f.life.dispatch(t,f.f.session,1,8,1,f.f.context));end(f);++scenarios;}
    {Fixture f;u32 old=f.start();end(f);C(f.native.configure(f.f.p,f.f.original));u32 t=f.start();C(t!=old);auto dead=f.f.session;dead.active=false;
     C(!f.life.dispatch(old,dead,2,8,1,f.f.context));C(f.life.dispatch(t,f.f.session,2,8,1,f.f.context));end(f);++scenarios;}
    {Fake f;JanuaryBackend n(f.host(),{});C(!n.configure(f.p,f.original));C(f.events.empty());++scenarios;}
    {Fake f;f.m[0x04DE52C4+5*4]=0;JanuaryBackend n(f.host(),f.calls());C(!n.configure(f.p,f.original));C(f.allocations.empty());++scenarios;}
    {Fixture f;f.f.alias_tree=true;C(!f.life.prepare(f.f.session,f.f.p,f.f.original));C(f.life.state()==LifeState::Quarantined);C(f.f.deleted.empty());++scenarios;}
    {Fixture f;u32 t=f.start();const u32 u=f.life.unit(0);f.f.attach_in_phase=true;
     C(!f.life.dispatch(t,f.f.session,1,8,1,f.f.context));C(!f.life.collect());
     C(f.life.state()==LifeState::Quarantined);C(f.life.unit(0)==u);C(f.f.deleted.size()==2);
     C(f.registry.resolve(u,WidgetKind::Reticle).mode==Mode::Hidden);++scenarios;}
    {Fixture f;auto b=f.b;b.checked_initialize=nullptr;
     Lifecycle invalid(f.registry,b);C(!invalid.prepare(f.f.session,f.f.p,f.f.original));C(f.f.allocations.empty());++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"native_backend_source_executed\":true,\"engine_calls_mocked\":true,\"gameplay_executed\":false}\n",scenarios,checks);
}
