#include "manager_driver.hpp"
#include <map>
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks = 0, scenarios = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); std::abort(); } } while(0)
struct Fixture {
    std::map<u32,u32> mem;
    struct Call { u32 unit, kind, phase, context; };
    std::vector<Call> calls;
    std::vector<u32> destroyed;
    Registry registry;
    u32 parents[2]{0x10000,0x20000}, originals[3]{0x30000,0x40000,0x50000};
    u32 next = 0x100000, thread = 7, reads = 0, scopes = 0, leaves = 0, sample_calls = 0;
    bool pinned = true, scoped = false, deny_scope = false, bad_restore = false;
    bool reenter = false, stop_phase = false, stop_enter = false, fail_phase = false;
    u32 scope_context = 0, scope_phase = 0; ManagerKind scope_kind = ManagerKind::Cockpit;
    ManagerFrame frame{1,{true,1,{0x60000,1,0,1},{0x70000,1,1,1}}, {0x10000,0x20000}, {10,11}};
    Lifecycle life; ManagerDriver driver;
    Fixture(): life(registry,backend()), driver(life,host()) {
        mem[parents[0]]=manager_vtable(ManagerKind::Cockpit);
        mem[parents[1]]=manager_vtable(ManagerKind::MiniMap);
        for (u32 i=0;i<3;++i) fill(originals[i],static_cast<WidgetKind>(i));
        mem[0x80000+0x158]=1; mem[0x90000+0x158]=0xABCD0101;
    }
    void fill(u32 u,WidgetKind k) {
        const u32 resource=0xA00000+static_cast<u32>(k)*0x1000;
        mem[u]=kind_info(k)->vtable;mem[u+0xC]=0xABFD4567;
        mem[u+0xF0]=resource;mem[u+0xF4]=u+0x1000;mem[u+0xF8]=u+0x2000;
        mem[resource+0x68]=resource+0x100;mem[resource+0x144]=2;mem[u+0x106C]=u;
        for(u32 i=0;i<2;++i){mem[u+0x2000+4*i]=u+0x3000+0x100*i;mem[u+0x306C+0x100*i]=u;}
    }
    static bool read(void* v,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fixture*>(v);++f.reads;
        auto it=f.mem.find(a);if(it==f.mem.end())return false;*out=it->second;return true;
    }
    Backend backend() {
        Backend b{};b.memory={this,read};
        b.safe_point=[](void* v) noexcept {return !static_cast<Fixture*>(v)->scoped;};
        b.construct=[](void* v,WidgetKind k) noexcept {
            auto& f=*static_cast<Fixture*>(v);u32 u=f.next;f.next+=0x10000;f.fill(u,k);return u;
        };
        b.initialize=[](void*,u32,WidgetKind) noexcept {};
        b.destroy=[](void* v,u32 u,WidgetKind) noexcept {
            auto& f=*static_cast<Fixture*>(v);CHECK(!f.scoped);
            for(auto p:f.originals)CHECK(u!=p);
            f.destroyed.push_back(u);
        };
        b.write_word=[](void* v,u32 a,u32 val) noexcept {
            auto& f=*static_cast<Fixture*>(v);auto it=f.mem.find(a);
            if(it==f.mem.end())return false;
            it->second=val;return true;
        };
        b.checked_phase=[](void* v,u32 u,WidgetKind k,u32 phase,u32 ctx) noexcept {
            auto& f=*static_cast<Fixture*>(v);
            CHECK(f.scoped);CHECK(kind_info(k)->manager==f.scope_kind);
            CHECK(phase==f.scope_phase);CHECK(ctx==f.scope_context);
            CHECK(f.driver.phase_scope(k,u,phase,ctx));
            CHECK(!f.driver.phase_scope(k,u+4,phase,ctx));
            CHECK(!f.driver.phase_scope(k,u,phase,ctx+4));
            CHECK(draw_view(f.mem[u+0xC])==2);
            f.calls.push_back({u,static_cast<u32>(k),phase,ctx});
            f.mem[u+0xC]^=0x80;
            if(f.reenter)CHECK(!f.driver.event(0x02B685B3,f.parents[1],0));
            if(f.stop_phase){f.driver.stop();CHECK(!f.driver.phase_scope(k,u,phase,ctx));CHECK(!f.life.collect());}
            return !f.fail_phase;
        };return b;
    }
    ManagerHost host() {
        return {{this,read},
            [](void* v) noexcept {return static_cast<Fixture*>(v)->thread;},
            [](void* v,ManagerFrame* out) noexcept {auto& f=*static_cast<Fixture*>(v);++f.sample_calls;if(!f.pinned)return false;*out=f.frame;return true;},
            [](void* v,const ManagerSite& site,const ManagerFrame&,u32 ctx) noexcept {
                auto& f=*static_cast<Fixture*>(v);CHECK(!f.scoped);if(f.deny_scope)return false;
                f.scoped=true;f.scope_kind=site.manager;f.scope_phase=site.phase;f.scope_context=ctx;++f.scopes;
                if(f.stop_enter)f.driver.stop();
                return true;
            },
            [](void* v) noexcept {auto& f=*static_cast<Fixture*>(v);CHECK(f.scoped);f.scoped=false;++f.leaves;return !f.bad_restore;}};
    }
    u32 start() {
        u32 ticket=life.prepare(frame.session,parents,originals);CHECK(ticket);CHECK(life.publish(ticket,frame.session));
        CHECK(driver.attach(ticket));return ticket;
    }
    void cleanup() {driver.stop();CHECK(life.collect());CHECK(destroyed.size()==3);CHECK(!scoped);}
};
int main(){
    // One frame contains two managers, not one all-widget event.
    for(bool reverse:{false,true}){
        Fixture f;f.start();const auto original=f.mem;
        for(u32 phase:{8u,9u,11u}){
            u32 a=phase==8?0x02B497CA:phase==9?0x02B49C5B:0x02B49DE3;
            u32 b=phase==8?0x02B6847B:phase==9?0x02B685B3:0x02B68724;
            u32 ca=phase==11?0x80000:0,cb=phase==11?0x90000:0;
            CHECK(f.driver.event(reverse?b:a,f.parents[reverse?1:0],reverse?cb:ca));
            CHECK(f.calls.size()==(phase==8?0u:phase==9?3u:6u)+(reverse?1u:2u));
            CHECK(f.driver.event(reverse?a:b,f.parents[reverse?0:1],reverse?ca:cb));
            CHECK(!f.driver.event(a,f.parents[0],ca));CHECK(!f.driver.event(b,f.parents[1],cb));
        }
        CHECK(f.calls.size()==9);CHECK(f.scopes==f.leaves);
        for(const auto& c:f.calls) CHECK(c.kind==2 ? c.context==(c.phase==11?0x90000u:0u) : c.context==(c.phase==11?0x80000u:0u));
        for(const auto& p:original) if(p.first<0x100000)CHECK(f.mem[p.first]==p.second);
        for(u32 i=0;i<3;++i)CHECK(draw_view(f.mem[f.life.unit(i)+0xC])==draw_view(0xABFD4567));
        f.cleanup();++scenarios;
    }
    // Invalid requests are side-effect free and cannot consume the other event.
    for(u32 bad:{0u,0x02B687F0u,0x02B49CDAu,0x02B4985Cu,0x02B6870Bu,0xFFFFFFFFu}){
        Fixture f;f.start();u32 reads=f.reads,samples=f.sample_calls;
        CHECK(!f.driver.event(bad,f.parents[0],0));CHECK(f.reads==reads);CHECK(f.sample_calls==samples);
        CHECK(f.driver.event(0x02B497CA,f.parents[0],0));f.cleanup();++scenarios;
    }
    for(int variant=0;variant<5;++variant){
        Fixture f;f.start();u32 reads=f.reads;
        switch(variant){
        case 0:CHECK(!f.driver.event(0x02B497CA,f.parents[1],0));break;
        case 1:CHECK(!f.driver.event(0x02B497CA,0,0));break;
        case 2:CHECK(!f.driver.event(0x02B497CA,f.parents[0],0x80000));break;
        case 3:f.thread=9;CHECK(!f.driver.event(0x02B497CA,f.parents[0],0));f.thread=7;break;
        case 4:CHECK(!f.driver.attach(0));break;
        }
        CHECK(f.reads==reads);CHECK(f.calls.empty());f.cleanup();++scenarios;
    }
    for(u32 ctx:{0u,0xFFFFFF00u,0xDEAD0000u}){
        Fixture f;f.start();CHECK(!f.driver.event(0x02B68724,f.parents[1],ctx));CHECK(f.scopes==0);
        CHECK(f.driver.event(0x02B68724,f.parents[1],0x80000));f.cleanup();++scenarios;
    }
    for(u32 view:{0u,2u,9u,255u}){
        Fixture f;f.start();f.mem[0x80158]=view;
        CHECK(!f.driver.event(0x02B49DE3,f.parents[0],0x80000));CHECK(f.calls.empty());
        f.mem[0x80158]=1;CHECK(f.driver.event(0x02B49DE3,f.parents[0],0x80000));f.cleanup();++scenarios;
    }
    for(int variant=0;variant<6;++variant){
        Fixture f;f.start();
        switch(variant){case 0:f.pinned=false;break;case 1:++f.frame.parent_lifetimes[0];break;
            case 2:++f.frame.parents[1];break;case 3:f.mem[f.parents[0]]=0;break;
            case 4:f.frame.session.sub0.think_mode=3;break;case 5:++f.frame.session.sub0.lifetime;break;}
        CHECK(!f.driver.event(0x02B497CA,f.parents[0],0));CHECK(f.life.state()==LifeState::Draining);
        CHECK(f.calls.empty());f.cleanup();++scenarios;
    }
    {Fixture f;f.frame.frame=3;f.start();f.frame.frame=2;f.frame.session.active=false;
        CHECK(!f.driver.event(0x02B497CA,f.parents[0],0));CHECK(f.life.state()==LifeState::Live);
        f.frame.frame=3;f.frame.session.active=true;CHECK(f.driver.event(0x02B497CA,f.parents[0],0));f.cleanup();++scenarios;}
    for(int variant=0;variant<5;++variant){
        Fixture f;f.start();switch(variant){case 0:f.deny_scope=true;break;case 1:f.stop_enter=true;break;
            case 2:f.stop_phase=true;break;case 3:f.bad_restore=true;break;case 4:f.fail_phase=true;break;}
        CHECK(!f.driver.event(0x02B497CA,f.parents[0],0));CHECK(!f.scoped);CHECK(f.scopes==f.leaves);
        if(variant==0){CHECK(f.calls.empty());CHECK(f.life.state()==LifeState::Live);f.deny_scope=false;CHECK(f.driver.event(0x02B497CA,f.parents[0],0));}
        if(variant==1)CHECK(f.calls.empty());
        if(variant==2||variant==4)CHECK(f.calls.size()==1);
        CHECK(f.destroyed.empty());f.cleanup();++scenarios;
    }
    {Fixture f;f.start();f.reenter=true;CHECK(f.driver.event(0x02B497CA,f.parents[0],0));CHECK(f.calls.size()==2);
        CHECK(f.driver.event(0x02B685B3,f.parents[1],0));f.cleanup();++scenarios;}
    {Fixture f;auto h=f.host();h.enter_scope=nullptr;ManagerDriver denied(f.life,h);
        CHECK(!denied.attach(1));CHECK(!denied.event(0x02B497CA,f.parents[0],0));++scenarios;}
    {Fixture f;u32 ticket=f.start();f.driver.stop();CHECK(f.life.collect());
        CHECK(!f.driver.attach(ticket));CHECK(!f.driver.event(0x02B497CA,f.parents[0],0));++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"engine_mocked\":true,\"gameplay_executed\":false}\n",scenarios,checks);
}
