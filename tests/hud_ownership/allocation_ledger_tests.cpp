#include "allocation_ledger.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks=0,scenarios=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Fake {
    bool safe=true,image=true,upstream_safe=true,upstream_fresh=true;
    u32 next=0x100000,alloc_calls=0,construct_calls=0,destroy_calls=0,last_target=0,last_size=0,last_align=0;
    JanuaryCalls native() {
        JanuaryCalls c{};c.context=this;
        c.allocate=[](void* p,u32 t,u32 n,u32 a) noexcept -> u32 {
            auto& f=*static_cast<Fake*>(p);++f.alloc_calls;f.last_target=t;f.last_size=n;f.last_align=a;return f.next;
        };
        c.construct=[](void* p,u32,u32 s) noexcept -> u32 {++static_cast<Fake*>(p)->construct_calls;return s;};
        c.method0=[](void*,u32,u32) noexcept {};
        c.method1=[](void* p,u32,u32,u32) noexcept {++static_cast<Fake*>(p)->destroy_calls;};
        c.singleton=[](void*,u32) noexcept -> u32 {return 0x500000;};
        c.contains=[](void*,u32,u32,u32) noexcept -> u32 {return 0;};
        return c;
    }
    AllocationGate gate(){return {this,[](void* p) noexcept{return static_cast<Fake*>(p)->safe;}};}
    AdmissionServices services(){
        AdmissionServices s{};s.memory={this,[](void*,u32,u32* o) noexcept{if(o)*o=0;return o!=nullptr;}};
        s.thread_id=[](void*) noexcept -> u32{return 7;};
        s.image_ok=[](void* p) noexcept{return static_cast<Fake*>(p)->image;};
        s.structural_safe=[](void* p) noexcept{return static_cast<Fake*>(p)->upstream_safe;};
        s.write_word=[](void*,u32,u32) noexcept{return true;};
        s.fresh_allocation=[](void* p,u32,u32) noexcept{return static_cast<Fake*>(p)->upstream_fresh;};
        return s;
    }
};
int main(){
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     const u32 p=c.allocate(c.context,0x01C11706,0x2E0,0x10);
     CHECK(p==0x100000);CHECK(l.retained()==1);CHECK(!l.owns(p));
     CHECK(l.certify(p,0x2E0));CHECK(l.owns(p,0x2E0));CHECK(!l.certify(p,0x2E0));
     CHECK(l.fault()==AllocationFault::Protocol);++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     const u32 p=c.allocate(c.context,0x01C15775,0x370,0x10);CHECK(l.certify(p,0x370));
     CHECK(c.construct(c.context,0x01B7D1F0,p)==p);CHECK(f.construct_calls==1);++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     CHECK(!c.allocate(c.context,0x01C11706,0x370,0x10));CHECK(f.alloc_calls==0);
     CHECK(l.fault()==AllocationFault::Target);++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     CHECK(!c.allocate(c.context,0x01C11706,0x2E0,8));CHECK(f.alloc_calls==0);++scenarios;}
    {Fake f;f.safe=false;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     CHECK(!c.allocate(c.context,0x01C11706,0x2E0,0x10));CHECK(f.alloc_calls==0);
     CHECK(l.fault()==AllocationFault::Unsafe);++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     const u32 p=c.allocate(c.context,0x01C7855A,0x2C0,0x10);
     CHECK(!c.allocate(c.context,0x01C15775,0x370,0x10));CHECK(f.alloc_calls==1);
     CHECK(l.fault()==AllocationFault::Protocol);(void)p;++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     const u32 p=c.allocate(c.context,0x01C7855A,0x2C0,0x10);CHECK(l.certify(p,0x2C0));
     c.method1(c.context,0x01BAB4F6,p,1);CHECK(f.destroy_calls==1);CHECK(l.retained()==0);++scenarios;}
    {Fake f;f.next=0x100003;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     CHECK(c.allocate(c.context,0x01C11706,0x2E0,0x10)==0x100003);
     CHECK(l.fault()==AllocationFault::Alignment);CHECK(!l.certify(0x100003,0x2E0));++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());auto c=l.callbacks();
     const u32 p=c.allocate(c.context,0x01C11706,0x2E0,0x10);CHECK(l.certify(p,0x2E0));
     f.next=p+0x100;c.method1(c.context,0x01C40C04,p,1);
     const u32 q=c.allocate(c.context,0x01C15775,0x370,0x10);CHECK(q==p+0x100);++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());
     AllocationAdmissionBridge b(f.gate(),l,f.services());auto s=b.callbacks();
     CHECK(s.structural_safe(s.memory.context));
     auto calls=l.callbacks();const u32 p=calls.allocate(calls.context,0x01C11706,0x2E0,0x10);
     CHECK(s.fresh_allocation(s.memory.context,p,0x2E0));CHECK(l.owns(p,0x2E0));++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());
     AllocationAdmissionBridge b(f.gate(),l,f.services());auto s=b.callbacks();
     f.upstream_safe=false;CHECK(!s.structural_safe(s.memory.context));++scenarios;}
    {Fake f;JanuaryAllocationLedger l(f.gate(),f.native());
     AllocationAdmissionBridge b(f.gate(),l,f.services());auto s=b.callbacks();auto calls=l.callbacks();
     const u32 p=calls.allocate(calls.context,0x01C11706,0x2E0,0x10);f.upstream_fresh=false;
     CHECK(!s.fresh_allocation(s.memory.context,p,0x2E0));CHECK(!l.owns(p));++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false}\n",scenarios,checks);
}
