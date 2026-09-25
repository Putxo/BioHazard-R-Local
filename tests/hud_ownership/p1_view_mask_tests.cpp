#include "p1_view_mask.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks=0,scenarios=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Fixture {
    std::map<u32,u32> m; P1ViewMask mask; u32 writes=0,fail_write=0;
    u32 cockpit=0x100000,minimap=0x110000,equip=0x200000,reticle=0x210000,herb=0x220000,ctx=0x300000;
    Fixture():mask({{this,read},write}) {
        m[cockpit]=manager_vtable(ManagerKind::Cockpit);m[minimap]=manager_vtable(ManagerKind::MiniMap);
        m[cockpit+0x5C]=equip;m[cockpit+0x90]=reticle;
        m[minimap+0x30]=herb;m[minimap+0x40]=herb;
        m[equip]=kind_info(WidgetKind::MainEquipment)->vtable;
        m[reticle]=kind_info(WidgetKind::Reticle)->vtable;
        m[herb]=kind_info(WidgetKind::MapHerb)->vtable;
        m[equip+0xC]=0xA5FF1234;m[reticle+0xC]=0x5A071234;m[herb+0xC]=0xCC031234;
        m[ctx+0x158]=1;
    }
    static bool read(void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fixture*>(p);auto it=f.m.find(a);if(it==f.m.end())return false;*v=it->second;return true;
    }
    static bool write(void* p,u32 a,u32 v) noexcept {
        auto& f=*static_cast<Fixture*>(p);++f.writes;if(f.fail_write&&f.writes==f.fail_write)return false;
        auto it=f.m.find(a);if(it==f.m.end())return false;it->second=v;return true;
    }
};
int main(){
    {Fixture f;const u32 e=f.m[f.equip+0xC],r=f.m[f.reticle+0xC];
     CHECK(f.mask.begin(0x02B49DE3,f.cockpit,f.ctx));CHECK(f.mask.active());
     CHECK(draw_view(f.m[f.equip+0xC])==(draw_view(e)&~2u));CHECK(draw_view(f.m[f.reticle+0xC])==(draw_view(r)&~2u));
     f.m[f.equip+0xC]^=0x80;f.m[f.reticle+0xC]^=0x40;
     CHECK(f.mask.end(0x02B49E5C,f.cockpit));CHECK(!f.mask.active());
     CHECK(draw_view(f.m[f.equip+0xC])==draw_view(e));CHECK(draw_view(f.m[f.reticle+0xC])==draw_view(r));
     CHECK((f.m[f.equip+0xC]&0xFFFF)==((e^0x80)&0xFFFF));CHECK((f.m[f.reticle+0xC]&0xFFFF)==((r^0x40)&0xFFFF));++scenarios;}
    {Fixture f;const u32 h=f.m[f.herb+0xC];CHECK(f.mask.begin(0x02B68724,f.minimap,f.ctx));
     CHECK(draw_view(f.m[f.herb+0xC])==(draw_view(h)&~2u));CHECK(f.mask.end(0x02B687F0,f.minimap));
     CHECK(draw_view(f.m[f.herb+0xC])==draw_view(h));++scenarios;}
    {Fixture f;f.m[f.ctx+0x158]=0;const auto before=f.m;CHECK(!f.mask.begin(0x02B49DE3,f.cockpit,f.ctx));
     CHECK(f.mask.fault()==P1MaskFault::Layout);CHECK(f.m==before);++scenarios;}
    {Fixture f;f.m[f.minimap+0x40]=0xDEAD;CHECK(!f.mask.begin(0x02B68724,f.minimap,f.ctx));
     CHECK(f.mask.fault()==P1MaskFault::Layout);++scenarios;}
    {Fixture f;f.m[f.equip]=0;CHECK(!f.mask.begin(0x02B49DE3,f.cockpit,f.ctx));CHECK(f.writes==0);++scenarios;}
    {Fixture f;f.fail_write=2;const u32 e=f.m[f.equip+0xC],r=f.m[f.reticle+0xC];
     CHECK(!f.mask.begin(0x02B49DE3,f.cockpit,f.ctx));CHECK(f.mask.fault()==P1MaskFault::Memory);
     CHECK(draw_view(f.m[f.equip+0xC])==draw_view(e));CHECK(f.m[f.reticle+0xC]==r);++scenarios;}
    {Fixture f;CHECK(f.mask.begin(0x02B49DE3,f.cockpit,f.ctx));CHECK(!f.mask.begin(0x02B49DE3,f.cockpit,f.ctx));
     CHECK(f.mask.fault()==P1MaskFault::Protocol);++scenarios;}
    {Fixture f;CHECK(f.mask.begin(0x02B49DE3,f.cockpit,f.ctx));f.m[f.cockpit+0x5C]=0xABC;
     CHECK(!f.mask.end(0x02B49E5C,f.cockpit));CHECK(f.mask.fault()==P1MaskFault::Restore);++scenarios;}
    {Fixture f;CHECK(f.mask.end(0x02B687F0,f.minimap));CHECK(f.mask.fault()==P1MaskFault::None);++scenarios;}
    {Fixture f;CHECK(f.mask.begin(0x02B68724,f.minimap,f.ctx));CHECK(!f.mask.end(0x02B49E5C,f.minimap));
     CHECK(f.mask.fault()==P1MaskFault::Protocol);++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false}\n",scenarios,checks);
}
