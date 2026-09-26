#include "structural_coordinator.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>

using namespace rev_hud;

static unsigned checks = 0, scenarios = 0;
#define CHECK(x) do { ++checks; if (!(x)) {     std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::abort(); } } while(0)

struct Fake {
    std::map<u32,u32> memory;
    LifeSnapshot snap{};
    bool has_snapshot = true;
    bool healthy_source = true;
    bool allocation_clean = true;
    bool configure_ok = true;
    bool publish_ok = true;
    bool collect_ok = true;
    LifeState state = LifeState::Empty;
    u32 next_ticket = 7;
    u32 capture_calls = 0, configure_calls = 0, prepare_calls = 0;
    u32 publish_calls = 0, stop_calls = 0, collect_calls = 0;

    Fake() {
        snap.session = {true,1,{0x300000,1,0,1},{0x310000,1,1,1}};
        snap.parents[0] = 0x200000;
        snap.parents[1] = 0x210000;
        snap.parent_lifetimes[0] = 11;
        snap.parent_lifetimes[1] = 12;

        memory[0x200000] = manager_vtable(ManagerKind::Cockpit);
        memory[0x210000] = manager_vtable(ManagerKind::MiniMap);

        memory[0x200000 + 0x90] = 0x100000;
        memory[0x200000 + 0x5C] = 0x110000;
        memory[0x210000 + 0x30] = 0x120000;
        memory[0x210000 + 0x40] = 0x120000;

        memory[0x100000] = kind_info(WidgetKind::Reticle)->vtable;
        memory[0x110000] = kind_info(WidgetKind::MainEquipment)->vtable;
        memory[0x120000] = kind_info(WidgetKind::MapHerb)->vtable;
    }

    static bool read(void* p,u32 address,u32* out) noexcept {
        auto& f=*static_cast<Fake*>(p);
        const auto it=f.memory.find(address);
        if(!out || it==f.memory.end()) return false;
        *out=it->second; return true;
    }

    CoordinatorOps ops() {
        CoordinatorOps out{};
        out.context=this;
        out.capture=[](void* p,LifeSnapshot* snap) noexcept {
            auto& f=*static_cast<Fake*>(p);
            ++f.capture_calls;
            if(!f.has_snapshot || !snap) return false;
            *snap=f.snap; return true;
        };
        out.healthy=[](void* p) noexcept {
            return static_cast<Fake*>(p)->healthy_source;
        };
        out.state=[](void* p) noexcept {
            return static_cast<Fake*>(p)->state;
        };
        out.allocation_clean=[](void* p) noexcept {
            return static_cast<Fake*>(p)->allocation_clean;
        };
        out.configure=[](void* p,const u32*,const u32*) noexcept {
            auto& f=*static_cast<Fake*>(p);
            ++f.configure_calls; return f.configure_ok;
        };
        out.prepare=[](void* p,const Session&,const u32*,const u32*) noexcept -> u32 {
            auto& f=*static_cast<Fake*>(p);
            ++f.prepare_calls;
            if(!f.next_ticket) return 0;
            f.state=LifeState::Prepared;
            return f.next_ticket;
        };
        out.publish=[](void* p,u32,const Session&) noexcept {
            auto& f=*static_cast<Fake*>(p);
            ++f.publish_calls;
            if(!f.publish_ok) return false;
            f.state=LifeState::Live;
            return true;
        };
        out.stop=[](void* p) noexcept {
            auto& f=*static_cast<Fake*>(p);
            ++f.stop_calls;
            if(f.state!=LifeState::Empty) f.state=LifeState::Draining;
        };
        out.collect=[](void* p) noexcept {
            auto& f=*static_cast<Fake*>(p);
            ++f.collect_calls;
            if(!f.collect_ok) return false;
            f.state=LifeState::Empty;
            f.allocation_clean=true;
            return true;
        };
        return out;
    }

    StructuralCoordinator coordinator() {
        return StructuralCoordinator({this,read},ops());
    }
};

int main() {
    {
        Fake f; auto c=f.coordinator();
        CHECK(c.run(1,0x10,0x20));
        CHECK(c.live()); CHECK(c.ticket()==7);
        CHECK(f.configure_calls==1 && f.prepare_calls==1 && f.publish_calls==1);
        ++scenarios;
    }
    {
        Fake f; auto c=f.coordinator();
        CHECK(c.run(1,0x10,0x20));
        CHECK(c.run(2,0x10,0x20));
        CHECK(f.configure_calls==1 && f.stop_calls==0 && f.collect_calls==0);
        ++scenarios;
    }
    {
        Fake f; auto c=f.coordinator();
        CHECK(c.run(1,0x10,0x20));
        ++f.snap.session.epoch;
        CHECK(c.run(2,0x10,0x20));
        CHECK(!c.live());
        CHECK(f.stop_calls==1 && f.collect_calls==1);
        CHECK(f.configure_calls==1);
        CHECK(c.run(3,0x10,0x20));
        CHECK(c.live()); CHECK(f.configure_calls==2);
        ++scenarios;
    }
    {
        Fake f; auto c=f.coordinator();
        CHECK(c.run(1,0x10,0x20));
        f.has_snapshot=false;
        CHECK(c.run(2,0x10,0x20));
        CHECK(!c.live()); CHECK(f.collect_calls==1);
        ++scenarios;
    }
    {
        Fake f; f.has_snapshot=false; auto c=f.coordinator();
        CHECK(c.run(1,0x10,0x20));
        CHECK(!c.live()); CHECK(f.configure_calls==0);
        ++scenarios;
    }
    {
        Fake f; f.has_snapshot=false; f.healthy_source=false; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Source);
        ++scenarios;
    }
    {
        Fake f; f.allocation_clean=false; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Allocation);
        ++scenarios;
    }
    {
        Fake f; f.memory[0x210000+0x40]=0xDEAD; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Layout);
        CHECK(f.configure_calls==0);
        ++scenarios;
    }
    {
        Fake f; f.memory[0x110000]=0; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Layout);
        ++scenarios;
    }
    {
        Fake f; f.configure_ok=false; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Backend);
        ++scenarios;
    }
    {
        Fake f; f.next_ticket=0; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Prepare);
        ++scenarios;
    }
    {
        Fake f; f.publish_ok=false; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(f.stop_calls==1 && f.collect_calls==1);
        CHECK(c.fault()==CoordinatorFault::Publish);
        ++scenarios;
    }
    {
        Fake f; f.state=LifeState::Draining; f.has_snapshot=false;
        f.collect_ok=false; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Collect);
        ++scenarios;
    }
    {
        Fake f; f.state=LifeState::Quarantined; auto c=f.coordinator();
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Quarantined);
        ++scenarios;
    }
    {
        Fake f; auto c=f.coordinator();
        CHECK(c.run(1,0x10,0x20));
        CHECK(!c.run(1,0x10,0x20));
        CHECK(c.fault()==CoordinatorFault::Config);
        ++scenarios;
    }
    {
        Fake f; auto c=f.coordinator();
        CHECK(!c.run(1,0,0x20));
        CHECK(c.fault()==CoordinatorFault::Config);
        ++scenarios;
    }
    {
        Fake f; auto c=f.coordinator();
        const StructuralTask task=c.task();
        CHECK(task.run(task.context,1,0x10,0x20));
        CHECK(c.ticket()==7);
        ++scenarios;
    }

    std::printf(
        "{\"status\":\"PASS\",\"scenarios\":%u,"
        "\"assertions\":%u,\"engine_calls_mocked\":true,"
        "\"gameplay_executed\":false}\n",
        scenarios,checks);
}
