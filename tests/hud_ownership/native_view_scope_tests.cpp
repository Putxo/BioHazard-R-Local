#include "native_view_scope.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks=0,scenarios=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Fixture {
    std::map<u32,u32> mem; u32 thread=7;
    const u32 draw=0x100000,camera=0x200000,sub=0x300000;
    NativeViewScope scope;
    ManagerSite site{0x02B49DE3,ManagerKind::Cockpit,11};
    ManagerFrame frame{};
    Fixture():scope({{this,read},tid}) {
        mem[draw]=0x04F79014;mem[draw+0x158]=1;
        mem[0x057D9188]=1;mem[0x057D9184]=sub;mem[0x05799D3C]=camera;
        mem[camera]=0x04EC9AA8;
        mem[camera+0x1C0+0x10]=0x03000101;
        mem[camera+0x1C0+0x14]=0;
        const u32 r[4]={0,360,1280,720};
        for(u32 i=0;i<4;++i)mem[camera+0x1C0+0x18+i*4]=mem[draw+0xBC+i*4]=r[i];
        frame.frame=9;frame.session.active=true;frame.session.epoch=3;
        frame.session.self={0x310000,4,0,1};frame.session.sub0={sub,5,1,1};
        frame.parents[0]=0x400000;frame.parents[1]=0x410000;
        frame.parent_lifetimes[0]=8;frame.parent_lifetimes[1]=9;
    }
    static bool read(void* c,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fixture*>(c);auto it=f.mem.find(a);
        if(it==f.mem.end()) return false;
        *out=it->second;
        return true;
    }
    static u32 tid(void* c) noexcept {return static_cast<Fixture*>(c)->thread;}
};
int main(){
    {Fixture f;CHECK(f.scope.enter(f.site,f.frame,f.draw));CHECK(f.scope.active());
     CHECK(f.scope.leave());CHECK(!f.scope.active());++scenarios;}
    {Fixture f;ManagerSite s{0x02B497CA,ManagerKind::Cockpit,8};
     CHECK(f.scope.enter(s,f.frame,0));CHECK(f.scope.leave());++scenarios;}
    {Fixture f;f.mem[f.draw+0x158]=0;CHECK(!f.scope.enter(f.site,f.frame,f.draw));++scenarios;}
    {Fixture f;f.mem[f.camera+0x1C0+0x10]=0x02000101;CHECK(!f.scope.enter(f.site,f.frame,f.draw));++scenarios;}
    {Fixture f;f.mem[f.camera+0x1C0+0x10]=0x03000100;CHECK(!f.scope.enter(f.site,f.frame,f.draw));++scenarios;}
    {Fixture f;f.mem[f.draw+0xBC+4]=359;CHECK(!f.scope.enter(f.site,f.frame,f.draw));++scenarios;}
    {Fixture f;f.mem[f.draw+0xBC+8]=0;f.mem[f.camera+0x1C0+0x18+8]=0;
     CHECK(!f.scope.enter(f.site,f.frame,f.draw));++scenarios;}
    {Fixture f;f.mem[0x057D9184]=0xDEAD;CHECK(!f.scope.enter(f.site,f.frame,f.draw));++scenarios;}
    {Fixture f;CHECK(f.scope.enter(f.site,f.frame,f.draw));f.mem[f.draw+0xBC+4]=361;
     CHECK(!f.scope.leave());CHECK(f.scope.fault()==ViewScopeFault::Changed);++scenarios;}
    {Fixture f;CHECK(f.scope.enter(f.site,f.frame,f.draw));f.thread=8;
     CHECK(!f.scope.leave());CHECK(f.scope.fault()==ViewScopeFault::Changed);++scenarios;}
    {Fixture f;auto s=f.scope.services();u32 v=0;
     CHECK(s.memory.word(s.memory.context,f.draw,&v));CHECK(v==0x04F79014);CHECK(s.thread_id(s.memory.context)==7);
     CHECK(s.enter_scope(s.memory.context,f.site,f.frame,f.draw));CHECK(s.leave_scope(s.memory.context));++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false}\n",scenarios,checks);
}
