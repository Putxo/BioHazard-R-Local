#include "ownership.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
#include <initializer_list>
namespace {
unsigned assertions = 0, cases = 0;
#define CHECK(e) do { ++assertions; if (!(e)) { std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#e); std::exit(1); } } while (0)
const Actor P1{0x10000000, 1, 0, 1}, P2{0x20000000, 1, 1, 1};
const Session Live{true, 1, P1, P2};
struct Fixture {
    Registry r;
    Token cockpit, map;
    Fixture() {
        r.set_session(Live);
        cockpit = r.open_manager(0x30000000, 0x04DE5C14, ManagerKind::Cockpit);
        map = r.open_manager(0x30010000, 0x04DE86FC, ManagerKind::MiniMap);
        CHECK(cockpit.valid() && map.valid());
    }
    Token bind(u32 address, WidgetKind kind, u32 member) {
        const auto* info = kind_info(kind);
        return r.bind_widget(info->manager == ManagerKind::Cockpit ? cockpit : map,
                             address, info->vtable, kind, member, member ? P2 : P1, member);
    }
};
void test_classes_and_phases() {
    ++cases;
    CHECK(kind_info(WidgetKind::Reticle)->actor_phase == 9);
    CHECK(kind_info(WidgetKind::MainEquipment)->actor_phase == 8);
    CHECK(kind_info(WidgetKind::MapHerb)->manager == ManagerKind::MiniMap);
    CHECK(!kind_info(static_cast<WidgetKind>(3)));
    CHECK(!manager_vtable(static_cast<ManagerKind>(99)));
}
void test_independent_widgets() {
    ++cases; Fixture f;
    for (u32 k=0;k<3;++k) for (u32 member=0;member<2;++member) {
        const auto kind=static_cast<WidgetKind>(k);
        const u32 address=0x40000000+k*0x10000+member*0x1000;
        CHECK(f.bind(address,kind,member).valid());
        const auto route=f.r.resolve(address,kind);
        CHECK(route.mode==Mode::Local);
        CHECK(route.actor==(member ? P2.address : P1.address));
        CHECK(route.member==member && route.view==member);
    }
    CHECK(f.r.widget_count()==6);
    CHECK(f.r.resolve(0x7654,WidgetKind::Reticle).mode==Mode::Stock);
}
void test_registration_guards() {
    ++cases; Fixture f;
    CHECK(!f.r.open_manager(0,0x04DE5C14,ManagerKind::Cockpit).valid());
    CHECK(!f.r.open_manager(0x30000000,0x04DE5C14,ManagerKind::Cockpit).valid());
    CHECK(!f.r.open_manager(0x30000001,0x04DE86FC,ManagerKind::Cockpit).valid());
    CHECK(!f.r.bind_widget(f.cockpit,0x1234,0x04DE810C,WidgetKind::MapHerb,1,P2,1).valid());
    CHECK(!f.r.bind_widget(f.map,0x1234,0x04DE52C4,WidgetKind::Reticle,1,P2,1).valid());
    CHECK(!f.r.bind_widget(f.cockpit,0,0x04DE52C4,WidgetKind::Reticle,1,P2,1).valid());
    CHECK(!f.r.bind_widget(f.cockpit,0x1234,0x04DE810C,WidgetKind::Reticle,1,P2,1).valid());
    CHECK(!f.r.bind_widget(f.cockpit,0x1234,0x04DE52C4,WidgetKind::Reticle,2,P2,2).valid());
    CHECK(!f.r.bind_widget(f.cockpit,0x1234,0x04DE52C4,WidgetKind::Reticle,1,P2,0).valid());
    CHECK(!f.r.bind_widget(f.cockpit,0x1234,0x04DE52C4,WidgetKind::Reticle,1,P1,1).valid());
    CHECK(f.bind(0x1234,WidgetKind::Reticle,1).valid());
    CHECK(!f.bind(0x1234,WidgetKind::MapHerb,1).valid());
    CHECK(f.r.resolve(0x1234,WidgetKind::MapHerb).mode==Mode::Hidden);
}
void test_session_guards_and_latched_revocation() {
    ++cases;
    for (u32 field=0;field<12;++field) {
        Fixture f; const auto p2=f.bind(0x4000,WidgetKind::Reticle,1);
        CHECK(f.bind(0x5000,WidgetKind::MainEquipment,0).valid());
        Session changed=Live;
        switch(field) {
            case 0:changed.active=false;break;
            case 1:changed.epoch=2;break;
            case 2:changed.sub0.address=0;break;
            case 3:changed.sub0.lifetime=2;break;
            case 4:changed.sub0.serial=2;break;
            case 5:changed.sub0.think_mode=2;break;
            case 6:changed.sub0.think_mode=3;break;
            case 7:changed.sub0.serial=-1;break;
            case 8:changed.sub0.lifetime=0;break;
            case 9:changed.sub0.address=P1.address;break;
            case 10:changed.sub0.serial=P1.serial;break;
            case 11:changed.epoch=0;break;
        }
        f.r.set_session(changed);
        CHECK(f.r.resolve(0x4000,WidgetKind::Reticle).mode==Mode::Hidden);
        CHECK(f.r.flags_for(0x4000,WidgetKind::Reticle,0xFFFFFFFF)==0xFC00FFFF);
        // A rollback, a reconnect or a repeated pointer must not resurrect P2.
        f.r.set_session(Live);
        CHECK(f.r.resolve(0x4000,WidgetKind::Reticle).mode==Mode::Hidden);
        CHECK(f.r.retire_widget(p2));
        const auto rebound=f.bind(0x4000,WidgetKind::Reticle,1);
        CHECK(rebound.valid() && rebound.generation!=p2.generation);
        CHECK(!f.r.retire_widget(p2));
        CHECK(f.r.resolve(0x4000,WidgetKind::Reticle).actor==P2.address);
    }
}
void test_stock_restoration_and_live_refresh() {
    ++cases;Fixture f;
    CHECK(f.bind(0x1110,WidgetKind::Reticle,0).valid());
    CHECK(f.bind(0x1120,WidgetKind::Reticle,1).valid());
    f.r.set_session(Live);
    CHECK(f.r.resolve(0x1120,WidgetKind::Reticle).mode==Mode::Local);
    Session stopped=Live;stopped.active=false;f.r.set_session(stopped);
    CHECK(f.r.resolve(0x1110,WidgetKind::Reticle).mode==Mode::Stock);
    CHECK(f.r.flags_for(0x1110,WidgetKind::Reticle,0xA5A5A5A5)==0xA5A5A5A5);
    CHECK(f.r.resolve(0x1120,WidgetKind::Reticle).actor==0);
}
void test_manager_teardown_and_aba() {
    ++cases;Fixture f;const auto old=f.cockpit;
    const auto first=f.bind(0x6000,WidgetKind::Reticle,1);
    const auto original=f.bind(0x6010,WidgetKind::MainEquipment,0);
    CHECK(!f.r.release_manager(old));
    CHECK(f.r.invalidate_manager(old));
    CHECK(f.r.resolve(0x6000,WidgetKind::Reticle).mode==Mode::Hidden);
    CHECK(f.r.resolve(0x6010,WidgetKind::MainEquipment).mode==Mode::Hidden);
    CHECK(!f.bind(0x7000,WidgetKind::Reticle,1).valid());
    CHECK(!f.r.release_manager(old));
    CHECK(f.r.retire_widget(first) && f.r.retire_widget(original));
    CHECK(f.r.release_manager(old));
    CHECK(!f.r.release_manager(old));
    f.cockpit=f.r.open_manager(0x30000000,0x04DE5C14,ManagerKind::Cockpit);
    CHECK(f.cockpit.valid() && f.cockpit.generation!=old.generation);
    CHECK(!f.r.invalidate_manager(old));
    const auto second=f.bind(0x6000,WidgetKind::Reticle,1);
    CHECK(second.valid());
    CHECK(!f.r.retire_widget(first));
    CHECK(f.r.resolve(0x6000,WidgetKind::Reticle).mode==Mode::Local);
}
void test_capacity() {
    ++cases;Registry r;r.set_session(Live);Token owners[Registry::ManagerCapacity];
    for (u32 i=0;i<Registry::ManagerCapacity;++i) {
        owners[i]=r.open_manager(0x100+i,0x04DE5C14,ManagerKind::Cockpit);CHECK(owners[i].valid());
    }
    CHECK(!r.open_manager(0xFFFF,0x04DE5C14,ManagerKind::Cockpit).valid());
    Token widgets[Registry::WidgetCapacity];
    for (u32 i=0;i<Registry::WidgetCapacity;++i) {
        widgets[i]=r.bind_widget(owners[i%Registry::ManagerCapacity],0x10000+i,
            0x04DE52C4,WidgetKind::Reticle,1,P2,1);CHECK(widgets[i].valid());
    }
    CHECK(!r.bind_widget(owners[0],0xFFFF,0x04DE52C4,WidgetKind::Reticle,1,P2,1).valid());
    CHECK(r.widget_count()==Registry::WidgetCapacity);
    CHECK(r.retire_widget(widgets[0]));
    CHECK(r.bind_widget(owners[0],0x10000,0x04DE52C4,WidgetKind::Reticle,1,P2,1).valid());
    CHECK(!r.retire_widget(widgets[0]));
}
void test_draw_view_packing() {
    ++cases;
    for (u32 flags : {0u,0xFFFFFFFFu,0x80010001u,0x55AA55AAu}) {
        for (u32 mask=0;mask<1024;++mask) {
            u32 result=0;CHECK(replace_draw_view(flags,mask,&result));
            CHECK(draw_view(result)==mask);
            CHECK((result&~ViewBits)==(flags&~ViewBits));
            for (u32 view=0;view<12;++view)
                CHECK(view_allowed(result,view)==(view<10 && (mask&(1u<<view))!=0));
        }
    }
    u32 value=17;CHECK(!replace_draw_view(0,1024,&value) && value==17);
    CHECK(!replace_draw_view(0,1,nullptr));CHECK(!view_allowed(0xFFFFFFFF,0xFFFFFFFF));
    Fixture f;CHECK(f.bind(0x8100,WidgetKind::Reticle,1).valid());
    CHECK(f.r.flags_for(0x8100,WidgetKind::Reticle,0xFFFFFFFF)==0xFC02FFFF);
    CHECK(f.r.flags_for(0x8101,WidgetKind::Reticle,0xFFFFFFFF)==0xFFFFFFFF);
}
void test_exported_resolver() {
    ++cases;auto& r=registry();r.set_session(Live);
    const auto mt=r.open_manager(0x60000000,0x04DE5C14,ManagerKind::Cockpit);
    const auto wt=r.bind_widget(mt,0x60000100,0x04DE52C4,WidgetKind::Reticle,1,P2,1);
    CHECK(mt.valid() && wt.valid());
    u32 actor=999;
    CHECK(rev_hud_resolve_actor(0x60000100,0,&actor)==1 && actor==P2.address);
    CHECK(rev_hud_resolve_actor(0x70000000,0,&actor)==0 && actor==0);
    CHECK(rev_hud_resolve_actor(0x60000100,2,&actor)==2 && actor==0);
    CHECK(rev_hud_resolve_actor(0x60000100,0,nullptr)==0);
    CHECK(r.invalidate_manager(mt));
    CHECK(rev_hud_resolve_actor(0x60000100,0,&actor)==2 && actor==0);
    CHECK(r.retire_widget(wt) && r.release_manager(mt));
}
}
int main() {
    test_classes_and_phases();test_independent_widgets();test_registration_guards();
    test_session_guards_and_latched_revocation();test_stock_restoration_and_live_refresh();
    test_manager_teardown_and_aba();test_capacity();test_draw_view_packing();test_exported_resolver();
    std::printf("{\"status\":\"PASS\",\"test_groups\":%u,\"assertions\":%u,\"gameplay_executed\":false,\"engine_code_executed\":false}\n",cases,assertions);
}
