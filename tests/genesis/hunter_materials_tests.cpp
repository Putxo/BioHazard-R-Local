#include "hunter_materials.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace rev_genesis;
static unsigned checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
static u32 W(float f){u32 u;std::memcpy(&u,&f,4);return u;}
struct F {
    std::map<u32,u32> m;u32 fail=0;unsigned writes=0;
    MaterialAccess access{this,read,write};
    static bool read(void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<F*>(p);auto i=f.m.find(a);if(i==f.m.end())return false;*v=i->second;return true;
    }
    static bool write(void* p,u32 a,u32 v) noexcept {
        auto& f=*static_cast<F*>(p);++f.writes;if(a==f.fail)return false;
        auto i=f.m.find(a);if(i==f.m.end())return false;i->second=v;return true;
    }
    void setup(unsigned parts=2,bool share=false){
        const u32 h=0x100000,arr=0x110000;m[h]=0x04D09FBC;m[h+0xFC]=parts;m[h+0xF8]=arr;
        m[h+0x1A74]=W(1.0f);m[0x0556DD74]=0x500000;
        for(unsigned i=0;i<parts;++i){
            const u32 p=0x120000+i*0x1000,idx=0x180000+i*0x1000,base=0x1A0000+i*0x1000;
            const u32 cbuf=0x200000+(share?0:i*0x1000),abuf=0x240000+(share?0:i*0x1000);
            m[arr+i*4]=p;m[p+0x8C]=2;m[p+0x7C]=idx;m[p+0x30]=base;
            m[idx]=0;m[idx+4]=1;m[base]=(0x1f4u<<20);m[base+4]=cbuf|3;
            m[base+12]=(0x22bu<<20);m[base+16]=abuf|7;
            for(unsigned j=0;j<64;++j){m.emplace(cbuf+j*4,0xA0000000u+j);m.emplace(abuf+j*4,0xB0000000u+j);}
        }
    }
};
int main(){
    {F f;f.setup();auto before=f.m;HunterMaterials h;
     C(h.begin(f.access,0x100000,true)==MaterialResult::Applied);C(h.active());C(h.hunter()==0x100000);
     C(h.color_count()==2);C(h.alpha_count()==2);
     const float alpha=0.99f,inv=1.0f-alpha;
     for(unsigned i=0;i<2;++i){const u32 c=0x200000+i*0x1000,a=0x240000+i*0x1000;
        for(unsigned j=0;j<3;++j)C(f.m[c+(44+j)*4]==W(1.0f-inv*0.75f));
        for(unsigned j=0;j<3;++j)C(f.m[c+j*4]==W(inv));
        for(unsigned j=0;j<3;++j)C(f.m[c+(40+j)*4]==W(inv*0.8f));
        C(f.m[a+4]==W(alpha));
     }
     C(h.begin(f.access,0x100000,true)==MaterialResult::Refused);
     C(h.end(f.access)==MaterialResult::Applied);C(!h.active());C(f.m==before);C(h.end(f.access)==MaterialResult::Refused);}
    {F f;f.setup(1);f.m[0x0556DD74]=0;HunterMaterials h;
     C(h.begin(f.access,0x100000,true)==MaterialResult::Applied);C(f.m[0x240004]==W(1.0f));C(h.end(f.access)==MaterialResult::Applied);}
    {F f;f.setup(1);HunterMaterials h;
     C(h.begin(f.access,0x100000,false)==MaterialResult::Applied);C(f.m[0x240004]==W(1.0f));C(h.end(f.access)==MaterialResult::Applied);}
    {F f;f.setup(2,true);HunterMaterials h;
     C(h.begin(f.access,0x100000,true)==MaterialResult::Applied);C(h.color_count()==1);C(h.alpha_count()==1);C(h.end(f.access)==MaterialResult::Applied);}
    {F f;f.setup(1);HunterMaterials h;f.fail=0x200000+44*4;auto before=f.m;
     C(h.begin(f.access,0x100000,true)==MaterialResult::Fault);C(!h.active());C(f.m==before);}
    {F f;f.setup(1);HunterMaterials h;f.m[0x100000]=0;
     C(h.begin(f.access,0x100000,true)==MaterialResult::Fault);C(!h.active());}
    {F f;f.setup(1);HunterMaterials h;f.m.erase(0x120000+0x8C);
     C(h.begin(f.access,0x100000,true)==MaterialResult::Fault);}
    {F f;f.setup(1);HunterMaterials h;auto a=f.access;a.write=nullptr;
     C(h.begin(a,0x100000,true)==MaterialResult::Refused);}
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"gameplay_executed\":false}\n",checks);
}
