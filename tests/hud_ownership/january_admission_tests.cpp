#include "january_admission.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks=0,scenarios=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Gate {
    bool event=false,view=false,exact=false,unit=false;
    static bool p(void* c,WidgetKind,u32,u32,u32) noexcept{return static_cast<Gate*>(c)->exact;}
    static bool u(void* c,WidgetKind,u32) noexcept{return static_cast<Gate*>(c)->unit;}
    static bool e(void* c) noexcept{return static_cast<Gate*>(c)->event;}
    static bool v(void* c) noexcept{return static_cast<Gate*>(c)->view;}
    AdmissionGate cb(){return {this,p,u,e,v};}
};
struct Fixture {
    std::map<u32,u32> mem{{0x1000,0xAA}};
    u32 thread=7,writes=0;bool image=true,safe=true,fresh=true;
    Gate gate;JanuaryAdmission admission;
    Fixture():admission(gate.cb(),{{this,read},tid,img,structural,write,alloc}){CHECK(admission.start(7));}
    static bool read(void* c,u32 a,u32* o) noexcept{auto& f=*static_cast<Fixture*>(c);auto i=f.mem.find(a);if(i==f.mem.end())return false;*o=i->second;return true;}
    static u32 tid(void* c) noexcept{return static_cast<Fixture*>(c)->thread;}
    static bool img(void* c) noexcept{return static_cast<Fixture*>(c)->image;}
    static bool structural(void* c) noexcept{return static_cast<Fixture*>(c)->safe;}
    static bool write(void* c,u32 a,u32 v) noexcept{auto& f=*static_cast<Fixture*>(c);auto i=f.mem.find(a);if(i==f.mem.end())return false;i->second=v;++f.writes;return true;}
    static bool alloc(void* c,u32,u32) noexcept{return static_cast<Fixture*>(c)->fresh;}
};
int main(){
    {Fixture f;auto h=f.admission.callbacks();
     CHECK(h.permit(h.memory.context,NativeOp::Structural,WidgetKind::Reticle,0,0,0));
     CHECK(h.permit(h.memory.context,NativeOp::Initialize,WidgetKind::Reticle,1,0,0));
     CHECK(h.permit(h.memory.context,NativeOp::Destroy,WidgetKind::Reticle,1,0,0));
     CHECK(!h.permit(h.memory.context,NativeOp::Phase,WidgetKind::Reticle,1,8,0));++scenarios;}
    {Fixture f;auto h=f.admission.callbacks();f.gate.event=f.gate.view=f.gate.exact=f.gate.unit=true;f.safe=false;
     CHECK(h.permit(h.memory.context,NativeOp::Phase,WidgetKind::Reticle,0x2000,11,0x3000));
     CHECK(h.permit(h.memory.context,NativeOp::Write,WidgetKind::Reticle,0x2000,0,0));
     CHECK(!h.permit(h.memory.context,NativeOp::Structural,WidgetKind::Reticle,0,0,0));
     u32 v=0;CHECK(h.memory.word(h.memory.context,0x1000,&v));CHECK(v==0xAA);
     CHECK(h.write(h.memory.context,0x1000,0xBB));CHECK(f.mem[0x1000]==0xBB);++scenarios;}
    {Fixture f;auto h=f.admission.callbacks();f.safe=false;u32 v=0;
     CHECK(!h.memory.word(h.memory.context,0x1000,&v));CHECK(f.admission.last_fault()==AdmissionFault::Unsafe);++scenarios;}
    {Fixture f;auto h=f.admission.callbacks();f.image=false;
     CHECK(!h.permit(h.memory.context,NativeOp::Structural,WidgetKind::Reticle,0,0,0));
     CHECK(f.admission.last_fault()==AdmissionFault::Image);++scenarios;}
    {Fixture f;auto h=f.admission.callbacks();f.thread=9;
     CHECK(!h.permit(h.memory.context,NativeOp::Structural,WidgetKind::Reticle,0,0,0));
     CHECK(f.admission.last_fault()==AdmissionFault::Thread);++scenarios;}
    {Fixture f;auto h=f.admission.callbacks();CHECK(h.fresh_allocation(h.memory.context,0x2000,0x100));
     f.fresh=false;CHECK(!h.fresh_allocation(h.memory.context,0x2000,0x100));++scenarios;}
    {Fixture f;auto h=f.admission.callbacks();f.gate.event=f.gate.view=true;f.gate.exact=false;f.gate.unit=true;f.safe=false;
     CHECK(!h.permit(h.memory.context,NativeOp::Phase,WidgetKind::MainEquipment,0x2000,8,0));
     CHECK(h.permit(h.memory.context,NativeOp::Write,WidgetKind::MainEquipment,0x2000,0,0));++scenarios;}
    {Fixture f;auto h=f.admission.callbacks();f.gate.event=f.gate.view=true;f.gate.exact=true;f.gate.unit=false;f.safe=false;
     CHECK(h.permit(h.memory.context,NativeOp::Phase,WidgetKind::MapHerb,0x2000,11,0x3000));
     CHECK(!h.permit(h.memory.context,NativeOp::Write,WidgetKind::MapHerb,0x2000,0,0));++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false}\n",scenarios,checks);
}
