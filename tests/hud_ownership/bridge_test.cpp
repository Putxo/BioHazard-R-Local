#include "ownership.hpp"
using namespace rev_hud;
extern "C" {
void rev_hud_reticle_self();void rev_hud_equipment_self();void rev_hud_herb_self();
void invoke_bridge(u32 widget, void (*bridge)(), u32* result);
u32 stock_calls=0, mock_errors=0;
}
namespace {
u32 failed=0, scenarios=0;
void check(bool yes) { if (!yes) ++failed; }
void test(u32 widget, void (*bridge)(), u32 expected, bool stock) {
    u32 result[10]{};const auto before=stock_calls;
    invoke_bridge(widget,bridge,result);
    check(result[0]==expected);check(result[1]==0x41414141);
    check(result[2]==0x11223344 && result[3]==0x55667788 && result[4]==0x7abc1234);
    check(result[5]==result[6]); // caller frame preserved
    check(result[7]==result[8]); // original argument popped exactly once
    check(result[9]==0x1234abcd);check(stock_calls==before+u32(stock));
    check(mock_errors==0);++scenarios;
}
void output(const char* text,u32 length) {
    asm volatile("int $0x80" : : "a"(4),"b"(1),"c"(text),"d"(length) : "memory");
}
}
extern "C" int test_main() {
    auto& r=registry();
    const Actor self{0x10000000,1,0,1},sub{0x20000000,1,1,1};
    Session session{true,1,self,sub};r.set_session(session);
    const auto cockpit=r.open_manager(0x30000000,0x04DE5C14,ManagerKind::Cockpit);
    const auto mini=r.open_manager(0x30010000,0x04DE86FC,ManagerKind::MiniMap);
    check(cockpit.valid() && mini.valid());
    void (*bridges[3])()={rev_hud_reticle_self,rev_hud_equipment_self,rev_hud_herb_self};
    Token members[3];
    for (u32 i=0;i<3;++i) {
        const auto kind=static_cast<WidgetKind>(i);const auto* info=kind_info(kind);
        members[i]=r.bind_widget(i==2 ? mini : cockpit,0x40000000+i*0x1000,
                                info->vtable,kind,1,sub,1);
        check(members[i].valid());
        test(0x41000000+i,bridges[i],0x11110000,true);
        test(0x40000000+i*0x1000,bridges[i],sub.address,false);
    }
    const auto original=r.bind_widget(cockpit,0x50000000,0x04DE52C4,
                                     WidgetKind::Reticle,0,self,0);
    check(original.valid());
    test(0x50000000,bridges[0],self.address,false);
    test(0x40000000,bridges[2],0,false); // wrong class must not read another layout
    session.active=false;r.set_session(session);
    for (u32 i=0;i<3;++i) test(0x40000000+i*0x1000,bridges[i],0,false);
    test(0x50000000,bridges[0],0x11110000,true);
    session.active=true;r.set_session(session);
    for (u32 i=0;i<3;++i) test(0x40000000+i*0x1000,bridges[i],0,false); // latched
    check(r.invalidate_manager(cockpit));
    test(0x50000000,bridges[0],0,false); // never call stock on invalidated owner
    check(scenarios==16);
    if (failed) {
        constexpr char message[]="{\"status\":\"FAIL\",\"native_i386\":true,\"engine_code_executed\":false}\n";
        output(message,sizeof(message)-1);return 1;
    }
    constexpr char message[]="{\"status\":\"PASS\",\"bridge_scenarios\":16,\"native_i386\":true,\"actual_registry_executed\":true,\"stock_finder_mocked\":true,\"gameplay_executed\":false,\"engine_code_executed\":false}\n";
    output(message,sizeof(message)-1);return 0;
}
