#include "resource_fixture.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_genesis;
unsigned checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Fake {
    std::map<u32,u32> memory;
    u32 fail=0;
    Reader reader(){return {this,[](void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fake*>(p);auto it=f.memory.find(a);
        if(a==f.fail || it==f.memory.end())return false;
        *v=it->second;return true;
    }};}
};
int main(){
    Fake f;constexpr u32 first=0x100000,second=0x200000,resource=0x500000;
    scanner_fixture::populate(f.memory,first,resource);
    scanner_fixture::populate(f.memory,second,resource);
    Resources a{},b{},again{};
    const auto original=f.memory;
    C(capture_resources(f.reader(),first,&a));C(capture_resources(f.reader(),second,&b));
    C(disjoint(a,b));C(!disjoint(a,a));C(capture_resources(f.reader(),first,&again));
    C(same_resources(a,again));C(!same_resources(a,b));C(original==f.memory);
    // Every cached element, animation and nested text must name its own node.
    for(u32 off=0x358;off<=0x3f4;off+=4) {
        auto it=f.memory.find(first+off);if(it==f.memory.end())continue;
        const u32 previous=it->second;it->second=f.memory[second+off];
        again=a;C(!capture_resources(f.reader(),first,&again));C(same_resources(a,again));
        it->second=previous;
    }
    // Missing ownership, oversize counts, interior aliases and array cookie drift.
    const u32 addresses[]={first+0x106c,first+0x224,first+0x230,first+0x2b8,
        first+0x30c,first+0xc000,first+0xd000,first+0xf8,first+0x306c};
    for(u32 address:addresses){
        const u32 previous=f.memory[address];
        for(u32 value:{0u,~0u,first+4}){f.memory[address]=value;C(!capture_resources(f.reader(),first,&again));}
        f.memory[address]=previous;
    }
    // A whole valid pool copied from another instance is still borrowed.
    f.memory[first+0x2b4]=f.memory[second+0x2b4];
    C(capture_resources(f.reader(),first,&again));C(!disjoint(again,b));
    C(!same_resources(a,again));f.memory[first+0x2b4]=original.at(first+0x2b4);
    // Nested child node corruption must be detected, including its owner field.
    const u32 nested=f.memory[first+0x3cc];f.memory[nested+0x6c]=second;
    C(!capture_resources(f.reader(),first,&again));f.memory[nested+0x6c]=original.at(nested+0x6c);
    for(u32 address:{first,first+0xf0,first+0x230,first+0x3f4,first+0xc000}){
        f.fail=address;again=a;C(!capture_resources(f.reader(),first,&again));C(same_resources(a,again));
    }
    f.fail=0;C(capture_resources(f.reader(),first,&again));C(same_resources(a,again));
    C(!capture_resources(f.reader(),0,&again));C(!capture_resources(f.reader(),first,nullptr));
    C(!disjoint(a,Resources{}));
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"regions\":%u,\"gameplay_executed\":false}\n",checks,a.count);
}
