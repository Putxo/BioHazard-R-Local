#include "../../patches/hud_ownership/scope_resources.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
using namespace rev_hud;
unsigned checks=0;
#define C(v) do {++checks;if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::exit(1);}}while(0)
struct Fixture {
    std::map<u32,u32> m;
    u32 reads=0,fail=0,change=0;
    static constexpr u32 unit=0x10000,other=0x20000,resource=0x50000;
    Fixture(){for(u32 u:{unit,other}){
        m[u]=0x04DE5594;m[u+0xF0]=resource;m[u+0xF4]=u+0x1000;m[u+0xF8]=u+0x2000;
        m[resource+0x68]=resource+0x100;m[resource+0x144]=3;m[u+0x106C]=u;
        for(u32 i=0;i<3;++i){m[u+0x2000+i*4]=m[u+0x2A0+i*4]=u+0x3000+i*0x100;m[u+0x306C+i*0x100]=u;}
        m[u+0x224]=1;m[u+0x230]=u+0x6000;m[u+0x6000]=m[u+0x2AC]=u+0x6100;m[u+0x6100]=0x70000;
    }}
    Reader reader(){return {this,[](void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fixture*>(p);++f.reads;
        if(f.reads==f.fail)return false;
        if(f.reads==28 && f.change)f.m[f.change]+=4;
        auto it=f.m.find(a);if(it==f.m.end())return false;*v=it->second;return true;
    }};}
};
int main(){
    {Fixture f;ScopeResources a{},b{};const auto before=f.m;
     C(capture_scope(f.reader(),f.unit,&a));C(f.reads==54);C(capture_scope(f.reader(),f.other,&b));
     C(disjoint_scopes(a,b));C(!disjoint_scopes(a,a));C(same_scope(a,a));C(!same_scope(a,b));C(f.m==before);}
    for(u32 i=1;i<=54;++i){Fixture f;ScopeResources out{};out.tree.unit=123;f.fail=i;
     C(!capture_scope(f.reader(),f.unit,&out));C(out.tree.unit==123);}
    for(u32 off:{0u,0xF0u,0xF4u,0xF8u,0x224u,0x230u,0x2A0u,0x2A4u,0x2A8u,0x2ACu}){
     Fixture f;ScopeResources out{};f.change=f.unit+off;C(!capture_scope(f.reader(),f.unit,&out));}
    for(u32 address:{Fixture::unit+0x6000,Fixture::unit+0x6100}){
     Fixture f;ScopeResources out{};f.change=address;C(!capture_scope(f.reader(),f.unit,&out));}
    for(u32 count:{0u,33u,0xFFFFFFFFu}){Fixture f;ScopeResources out{};f.m[f.unit+0x224]=count;
     C(!capture_scope(f.reader(),f.unit,&out));}
    {Fixture f;ScopeResources out{};f.m[f.unit+0x224]=2;f.m[f.unit+0x6004]=f.m[f.unit+0x6000];C(!capture_scope(f.reader(),f.unit,&out));}
    {Fixture f;ScopeResources out{};f.m[f.unit+0x6000]=f.m[f.unit+0x2AC]=f.unit+0x3000;C(!capture_scope(f.reader(),f.unit,&out));}
    {Fixture f;ScopeResources a{},b{};C(capture_scope(f.reader(),f.unit,&a));
     f.m[f.other+0x6000]=f.m[f.other+0x2AC]=f.unit+0x6100;C(capture_scope(f.reader(),f.other,&b));C(!disjoint_scopes(a,b));}
    {Fixture f;ScopeResources a{},b{};C(capture_scope(f.reader(),f.unit,&a));
     f.m[f.unit+0x6100]+=4;C(capture_scope(f.reader(),f.unit,&b));C(!same_scope(a,b));}
    {Fixture f;ScopeResources out{};C(!capture_scope(f.reader(),0xFFFFFD51,&out));C(f.reads==0);C(!capture_scope(f.reader(),f.unit,nullptr));}
    std::printf("Scope resources: %u checks PASS\n",checks);
}
