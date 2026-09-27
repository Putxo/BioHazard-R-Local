#include "../../patches/genesis/filter_resource.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_genesis;
unsigned checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
constexpr u32 Source=0x100000,Copy=0x200000;
struct Fake {
    std::map<u32,u32> m;
    u32 create_result=Copy,fail=0,mutate=0,reads=0;
    unsigned created=0,copied=0,released=0;
    int fault=0;
    static void graph(std::map<u32,u32>& m,u32 p,u32 n) {
        m[p]=0x04DB7A34;m[p+0x48]=1;m[p+0x54]=m[p+0x58]=m[p+0x5c]=0;
        m[p+0x68]=0x04CC7278;m[p+0x6c]=m[p+0x70]=n;m[p+0x74]=1;m[p+0x78]=n?p+0x1000:0;
        for(u32 i=0;i<n;++i) {m[p+0x1000+i*4]=p+0x2000+i*0x100;m[p+0x2000+i*0x100]=0x04DB8000+i*4;}
    }
    Fake(){graph(m,Source,3);}
    Reader reader(){return {this,[](void* p,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fake*>(p);auto it=f.m.find(a);
        if(a==f.fail || it==f.m.end())return false;
        *out=it->second;
        if(a==f.mutate && ++f.reads==1)++it->second;
        return true;
    }};}
    FilterCopyHost host(){return {reader(),this,
        [](void* p) noexcept ->u32 {auto& f=*static_cast<Fake*>(p);++f.created;
            if(f.create_result==Copy)graph(f.m,Copy,0);
            if(f.fault==1)f.m[Copy+0x58]=1;
            return f.create_result;},
        [](void* p,u32 source,u32 destination) noexcept ->bool {
            auto& f=*static_cast<Fake*>(p);C(source==Source);C(destination==Copy);++f.copied;
            graph(f.m,Copy,3);
            if(f.fault==2 || f.fault==8)f.m[Copy+0x1000]=f.m[Source+0x1000];
            if(f.fault==3)f.m[Copy+0x78]=f.m[Source+0x78];
            if(f.fault==4)f.m[Source+0x1000]+=0x10;
            if(f.fault==5)f.m[Copy+0x2000]+=4;
            if(f.fault==6)f.m[Copy+0x48]=2;
            if(f.fault==9)f.m[Copy+0x6c]=65;
            return f.fault!=7 && f.fault!=8;},
        [](void* p,u32 resource) noexcept {auto& f=*static_cast<Fake*>(p);C(resource==Copy);++f.released;}};}
};
int main(){
    Fake f;FilterResource a{},b{},saved{};
    C(capture_filter_resource(f.reader(),Source,&a));saved=a;
    const auto initial=f.m;
    C(!capture_filter_resource(f.reader(),0,&b));C(!capture_filter_resource(f.reader(),Source,nullptr));
    C(!disjoint_filter_resources(a,a));C(!disjoint_filter_resources(a,{}));
    C(copy_filter_resource(f.host(),Source)==Copy);C(f.created==1 && f.copied==1 && !f.released);
    C(capture_filter_resource(f.reader(),Copy,&b));C(disjoint_filter_resources(a,b));
    C(disjoint_filter_resources(b,a));C(!same_filter_resource(a,b));
    for(const auto& kv:initial)C(f.m.at(kv.first)==kv.second);
    for(int fault=1;fault<=9;++fault) {
        Fake g;g.fault=fault;C(!copy_filter_resource(g.host(),Source));
        C(g.created==1);C(g.copied==(fault==1?0u:1u));
        C(g.released==((fault==5 || fault==7)?1u:0u));
    }
    // Any address within the source allocation, pointer array or unit base
    // must be rejected before serialization/destruction.
    for(u32 borrowed:{Source,Source+4,Source+0x7c,Source+0x1000,Source+0x1004,Source+0x2000,Source+0x2020,0u,~0u}) {
        Fake g;g.create_result=borrowed;C(!copy_filter_resource(g.host(),Source));C(!g.copied && !g.released);
    }
    for(u32 off:{0u,0x68u,0x6cu,0x70u,0x74u,0x78u}) {
        Fake g;g.fail=Source+off;saved=a;C(!capture_filter_resource(g.reader(),Source,&saved));C(same_filter_resource(a,saved));
        C(!copy_filter_resource(g.host(),Source));C(!g.created);
    }
    for(u32 bad:{65u,~0u}) {Fake g;g.m[Source+0x6c]=bad;C(!capture_filter_resource(g.reader(),Source,&b));}
    {Fake g;g.m[Source+0x70]=2;C(!capture_filter_resource(g.reader(),Source,&b));}
    {Fake g;g.m[Source+0x1004]=g.m[Source+0x1000]+4;g.m[Source+0x2004]=0x04DB8004;C(!capture_filter_resource(g.reader(),Source,&b));}
    {Fake g;g.m[Source+0x1004]=0;C(capture_filter_resource(g.reader(),Source,&b));C(!b.units[1] && !b.vtables[1]);}
    {Fake g;g.mutate=Source+0x6c;C(!capture_filter_resource(g.reader(),Source,&b));}
    {Fake g;g.mutate=Source+0x2000;C(!capture_filter_resource(g.reader(),Source,&b));}
    {Fake g;auto h=g.host();h.copy=nullptr;C(!copy_filter_resource(h,Source));C(!g.created);}
    {Fake g;Fake::graph(g.m,Source,64);C(capture_filter_resource(g.reader(),Source,&b));C(b.count==64);}
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"gameplay_executed\":false}\n",checks);
}
