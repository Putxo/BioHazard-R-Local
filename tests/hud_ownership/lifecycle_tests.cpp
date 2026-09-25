#include "lifecycle.hpp"
#include <map>
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); std::abort(); } } while(0)
struct Mock {
    std::map<u32,u32> memory;
    std::vector<u32> allocated, initialized, destroyed, invoked;
    Registry registry;
    Lifecycle* life = nullptr;
    bool safe = true, stop_init = false, stop_phase = false;
    int fail_construct = -1, fail_initialize = -1, alias_construct = -1, wrong_class = -1;
    u32 writes = 0, fail_write = 0;
    u32 next = 0x500000, phase_calls = 0;
    u32 originals[3] = {0x100000,0x110000,0x120000};
    u32 parents[2] = {0x200000,0x210000};
    Session session{true,1,{0x300000,1,0,1},{0x310000,1,1,1}};
    Mock() {
        for (u32 i=0;i<3;++i) fill(originals[i],static_cast<WidgetKind>(i));
        memory[parents[0]]=manager_vtable(ManagerKind::Cockpit);
        memory[parents[1]]=manager_vtable(ManagerKind::MiniMap);
    }
    void fill(u32 u,WidgetKind k) {
        const u32 resource=0x400000+static_cast<u32>(k)*0x1000;
        memory[u]=kind_info(k)->vtable; memory[u+0xC]=0xAAFD4321;
        memory[u+0xF0]=resource; memory[u+0xF4]=u+0x1000; memory[u+0xF8]=u+0x2000;
        memory[resource+0x68]=resource+0x100; memory[resource+0x144]=3;
        memory[u+0x106C]=u;
        for (u32 i=0;i<3;++i) { memory[u+0x2000+4*i]=u+0x3000+0x100*i; memory[u+0x306C+0x100*i]=u; }
    }
    static bool read(void* p,u32 a,u32* v) noexcept {
        auto& m=*static_cast<Mock*>(p); const auto it=m.memory.find(a);
        if (it==m.memory.end()) return false;
        *v=it->second; return true;
    }
    static bool write(void* p,u32 a,u32 v) noexcept {
        auto& m=*static_cast<Mock*>(p); auto it=m.memory.find(a);
        if (++m.writes==m.fail_write || it==m.memory.end()) return false;
        it->second=v; return true;
    }
    static bool safepoint(void* p) noexcept { return static_cast<Mock*>(p)->safe; }
    static u32 construct(void* p,WidgetKind k) noexcept {
        auto& m=*static_cast<Mock*>(p); const int i=static_cast<int>(k);
        if (i==m.fail_construct) return 0;
        if (i==m.alias_construct) return m.originals[0];
        const u32 a=m.next; m.next+=0x10000;
        m.allocated.push_back(a); m.memory[a]=i==m.wrong_class ? 0 : kind_info(k)->vtable;
        m.memory[a+0xF0]=m.memory[a+0xF4]=m.memory[a+0xF8]=0;
        return a;
    }
    static void initialize(void* p,u32 a,WidgetKind k) noexcept {
        auto& m=*static_cast<Mock*>(p);
        m.initialized.push_back(a);
        if (static_cast<int>(k)!=m.fail_initialize) m.fill(a,k);
        else {m.memory[a+0xF0]=0;m.memory[a+0xF4]=0;m.memory[a+0xF8]=0;}
        if(m.stop_init) m.life->stop();
    }
    static void destroy(void* p,u32 a,WidgetKind k) noexcept {
        auto& m=*static_cast<Mock*>(p);
        CHECK(m.safe);
        for (auto b:m.originals) CHECK(a!=b);
        for (auto b:m.destroyed) CHECK(a!=b);
        // A previously bound P2 entry must not resolve to its actor in teardown.
        CHECK(m.registry.resolve(a,k).mode!=Mode::Local);
        CHECK(!m.life->collect()); // reentrant collection never frees twice
        m.destroyed.push_back(a);m.memory.erase(a);
    }
    static void phase(void* p,u32 a,WidgetKind k,u32 phase,u32 context) noexcept {
        auto& m=*static_cast<Mock*>(p);
        CHECK(phase==8 || phase==9 || phase==11);CHECK(context==0x1234);
        CHECK(m.registry.resolve(a,k).actor==m.session.sub0.address);
        CHECK(draw_view(m.memory[a+0xC])==2);
        m.memory[a+0xC]^=0x80; // legitimate unrelated engine flag update survives
        ++m.phase_calls;m.invoked.push_back(a);
        CHECK(!m.life->collect());
        if(m.stop_phase) {m.life->stop();CHECK(!m.life->collect());}
    }
    Backend backend() {return {{this,read},safepoint,construct,initialize,destroy,phase,write};}
};
struct Fixture { Mock mock; Lifecycle life; Fixture():life(mock.registry,mock.backend()){mock.life=&life;} };
static u32 ready(Fixture& f) {
    auto& m=f.mock; auto& l=f.life;
    u32 t=l.prepare(m.session,m.parents,m.originals);
    CHECK(t);CHECK(l.state()==LifeState::Prepared);CHECK(m.registry.widget_count()==3);
    CHECK(!l.dispatch(t,m.session,1,8,0,0x1234));CHECK(l.publish(t,m.session));
    CHECK(l.state()==LifeState::Live);return t;
}
int main() {
    unsigned scenarios=0;
    { Fixture f; auto& m=f.mock;GuiTree a{},b{};
      CHECK(capture_tree(m.backend().memory,m.originals[0],WidgetKind::Reticle,&a));
      m.fill(0x500000,WidgetKind::Reticle);
      CHECK(capture_tree(m.backend().memory,0x500000,WidgetKind::Reticle,&b));
      CHECK(a.resource==b.resource);CHECK(disjoint_trees(a,b));
      b.nodes[1]=a.nodes[1];CHECK(!disjoint_trees(a,b));
      GuiTree sentinel{};sentinel.unit=55;m.memory[0x500000+0x306C]=123;
      CHECK(!capture_tree(m.backend().memory,0x500000,WidgetKind::Reticle,&sentinel));CHECK(sentinel.unit==55);
      ++scenarios; }
    for(int limit: {0,257,1000000}) {
      Fixture f;auto& m=f.mock;GuiTree out{};m.memory[0x400144]=static_cast<u32>(limit);
      CHECK(!capture_tree(m.backend().memory,m.originals[0],WidgetKind::Reticle,&out));++scenarios;
    }
    { Fixture f;auto& m=f.mock;const auto original=m.memory;u32 t=ready(f);
      CHECK(!f.life.publish(t,m.session));CHECK(!f.life.dispatch(t,m.session,1,11,0,0x1234));
      for(u32 phase:{8u,9u,11u}) {CHECK(f.life.dispatch(t,m.session,1,phase,1,0x1234));CHECK(!f.life.dispatch(t,m.session,1,phase,1,0x1234));}
      CHECK(m.phase_calls==9);CHECK(!f.life.dispatch(t,m.session,0,8,1,0x1234));
      for(u32 i=0;i<3;++i) CHECK(draw_view(m.memory[f.life.unit(i)+0xC])==draw_view(0xAAFD4321));
      for(auto& kv:original) CHECK(m.memory[kv.first]==kv.second);
      f.life.stop();m.safe=false;CHECK(!f.life.collect());CHECK(m.destroyed.empty());m.safe=true;
      CHECK(f.life.collect());CHECK(m.destroyed.size()==3);CHECK(m.registry.widget_count()==0);
      CHECK(m.destroyed[0]==m.allocated[2]);CHECK(m.destroyed[2]==m.allocated[0]);
      CHECK(!f.life.collect());u32 t2=ready(f);CHECK(t2!=t);CHECK(!f.life.dispatch(t,m.session,2,8,1,0x1234));
      f.life.stop();CHECK(f.life.collect());++scenarios; }
    for(int fail=0;fail<3;++fail) {
      Fixture f;auto& m=f.mock;m.fail_construct=fail;
      CHECK(!f.life.prepare(m.session,m.parents,m.originals));CHECK(f.life.state()==LifeState::Empty);
      CHECK(m.allocated.size()==static_cast<unsigned>(fail));CHECK(m.destroyed.size()==m.allocated.size());
      CHECK(m.registry.widget_count()==0);++scenarios;
    }
    for(int fail=0;fail<3;++fail) {
      Fixture f;auto& m=f.mock;m.fail_initialize=fail;
      CHECK(!f.life.prepare(m.session,m.parents,m.originals));CHECK(f.life.state()==LifeState::Empty);
      CHECK(m.destroyed.size()==m.allocated.size());CHECK(m.registry.widget_count()==0);++scenarios;
    }
    { Fixture f;auto& m=f.mock;m.safe=false;
      CHECK(!f.life.prepare(m.session,m.parents,m.originals));CHECK(m.allocated.empty());++scenarios; }
    { Fixture f;auto& m=f.mock;m.alias_construct=1;
      CHECK(!f.life.prepare(m.session,m.parents,m.originals));CHECK(m.destroyed.size()==1);
      CHECK(m.memory.count(m.originals[0]));++scenarios; }
    { Fixture f;auto& m=f.mock;m.stop_init=true;
      CHECK(!f.life.prepare(m.session,m.parents,m.originals));CHECK(m.allocated.size()==1);
      CHECK(m.destroyed.size()==1);++scenarios; }
    { Fixture f;auto& m=f.mock;u32 t=ready(f);m.stop_phase=true;
      CHECK(!f.life.dispatch(t,m.session,1,8,1,0x1234));CHECK(m.phase_calls==1);CHECK(m.destroyed.empty());
      CHECK(f.life.state()==LifeState::Draining);CHECK(f.life.collect());++scenarios; }
    for(int mutation=0;mutation<5;++mutation) {
      Fixture f;auto& m=f.mock;u32 t=ready(f);Session next=m.session;
      switch(mutation){case 0:next.active=false;break;case 1:++next.epoch;break;case 2:++next.sub0.lifetime;break;
        case 3:++next.sub0.serial;break;case 4:next.sub0.think_mode=3;break;}
      f.life.observe(next);CHECK(f.life.state()==LifeState::Draining);
      f.life.observe(m.session);CHECK(!f.life.publish(t,m.session));CHECK(!f.life.dispatch(t,m.session,1,8,1,0x1234));
      CHECK(f.life.collect());++scenarios;
    }
    { Fixture f;auto& m=f.mock;u32 t=ready(f);
      f.life.parent_destroying(m.parents[1]);CHECK(!f.life.dispatch(t,m.session,1,8,1,0x1234));CHECK(f.life.collect());++scenarios; }
    { Fixture f;auto& m=f.mock;u32 t=f.life.prepare(m.session,m.parents,m.originals);CHECK(t);
      // Distinct clone object but aliases P1's mutable table. Do not destroy it.
      m.memory[f.life.unit(0)+0xF8]=m.memory[m.originals[0]+0xF8];
      CHECK(!f.life.publish(t,m.session));CHECK(f.life.state()==LifeState::Draining);
      CHECK(!f.life.collect());CHECK(f.life.state()==LifeState::Quarantined);
      CHECK(m.destroyed.size()==2);CHECK(m.memory.count(m.originals[0]));
      CHECK(!f.life.collect());CHECK(m.destroyed.size()==2);++scenarios;
    }
    { Fixture f;auto& m=f.mock;u32 t=ready(f);f.life.stop();CHECK(f.life.collect());
      m.fail_initialize=0;CHECK(!f.life.prepare(m.session,m.parents,m.originals));
      CHECK(f.life.state()==LifeState::Empty);CHECK(m.destroyed.size()==4);
      CHECK(!f.life.publish(t,m.session));++scenarios; }
    for (int bad=0;bad<4;++bad) {
      Fixture f;auto& m=f.mock;Session s=m.session;
      switch(bad){case 0:s.sub0.think_mode=3;break;case 1:s.sub0.address=s.self.address;break;
      case 2:s.sub0.serial=-1;break;case 3:s.sub0.lifetime=0;break;}
      CHECK(!f.life.prepare(s,m.parents,m.originals));CHECK(m.allocated.empty());++scenarios;
    }
    { Fixture f;auto& m=f.mock;m.memory[m.parents[1]]=0;
      CHECK(!f.life.prepare(m.session,m.parents,m.originals));CHECK(m.allocated.empty());
      CHECK(f.life.state()==LifeState::Empty);++scenarios; }
    { Fixture f;auto& m=f.mock;u32 t=ready(f);Session s=m.session;s.active=false;
      CHECK(!f.life.dispatch(t,s,1,8,0,0x1234));CHECK(m.phase_calls==0);
      CHECK(f.life.collect());++scenarios; }
    { Fixture f;auto& m=f.mock;u32 old=ready(f);f.life.stop();CHECK(f.life.collect());
      u32 fresh=ready(f);Session expired=m.session;expired.active=false;
      CHECK(!f.life.dispatch(old,expired,2,8,1,0x1234));CHECK(f.life.state()==LifeState::Live);
      CHECK(f.life.dispatch(fresh,m.session,2,8,1,0x1234));f.life.stop();CHECK(f.life.collect());++scenarios; }
    { Fixture f;auto& m=f.mock;m.wrong_class=1;
      CHECK(!f.life.prepare(m.session,m.parents,m.originals));CHECK(f.life.state()==LifeState::Quarantined);
      CHECK(m.initialized.size()==1);CHECK(m.destroyed.size()==1);
      CHECK(!f.life.collect());CHECK(m.destroyed.size()==1);++scenarios; }
    for(u32 fail:{1u,2u}) { Fixture f;auto& m=f.mock;u32 t=ready(f);m.fail_write=fail;
      CHECK(!f.life.dispatch(t,m.session,1,8,1,0x1234));CHECK(f.life.state()==LifeState::Draining);
      CHECK(m.phase_calls==fail-1);CHECK(m.destroyed.empty());CHECK(f.life.collect());++scenarios; }
    { Fixture f;auto& m=f.mock;u32 t=ready(f);
      const u32 before=m.memory[f.life.unit(0)+0xC];
      CHECK(f.life.dispatch(t,m.session,1,8,1,0x1234));
      CHECK(m.memory[f.life.unit(0)+0xC]==(before^0x80));
      CHECK(!f.life.dispatch(t,m.session,1,7,1,0x1234));
      f.life.stop();CHECK(f.life.collect());++scenarios; }
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false,\"engine_calls_mocked\":true}\n",scenarios,checks);
}
