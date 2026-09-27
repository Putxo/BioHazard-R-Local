#include "../../patches/genesis/camera.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
using namespace rev_genesis;
int checks=0;
#define C(v) do {++checks;if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::exit(1);}}while(0)
struct Fixture {
    static constexpr u32 manager=0x10000,primary=0x20000,secondary=0x30000,widget=0x40000;
    std::map<u32,u32> m{{0x05799D3C,manager},{manager+0xCE0,primary},
        {manager+0xCE4,secondary},{secondary,0x04CF2B1C},{secondary+0x74,0x60000}};
    Owner owner{{true,8,{0x50000,9,1,1},{0x60000,10,2,1}},0x60000};
    Mode mode=Mode::Local;u32 reads=0,resolves=0,fail_read=0;
    bool change_owner=false;u32 change_address=0,change_value=0;
    ProgressHost host(){return {this,[](void* p,u32 w,Owner* o) noexcept {
        auto& f=*static_cast<Fixture*>(p);C(w==widget);++f.resolves;
        *o=f.owner;if(f.resolves==2){if(f.change_owner)++o->session.sub0.lifetime;
            if(f.change_address)f.m[f.change_address]=f.change_value;}
        return f.mode;
    }};}
    rev_hud::Reader reader(){return {this,[](void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fixture*>(p);++f.reads;
        if(f.reads==f.fail_read)return false;
        auto it=f.m.find(a);if(it==f.m.end())return false;*v=it->second;return true;
    }};}
    Mode get(u32* out,u32 mgr=manager){return camera(host(),reader(),widget,mgr,out);}
};
int main(){
    {Fixture f;u32 out=99;const auto before=f.m;C(f.get(&out)==Mode::Local);C(out==f.secondary);C(f.m==before);C(f.reads==10);}
    {Fixture f;f.mode=Mode::Stock;u32 out=99;C(f.get(&out)==Mode::Stock);C(out==99);C(f.reads==0);}
    {Fixture f;f.mode=Mode::Hidden;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);C(f.reads==0);}
    {Fixture f;C(f.get(nullptr)==Mode::Hidden);C(f.reads==0);}
    {Fixture f;u32 out=99;C(camera({},f.reader(),f.widget,f.manager,&out)==Mode::Stock);C(out==99);C(f.reads==0);}
    for(u32 fail=1;fail<=10;++fail){Fixture f;f.fail_read=fail;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    for(u32 mgr:{0u,0xFFFFF319u,0x123400u}){Fixture f;u32 out=99;C(f.get(&out,mgr)==Mode::Hidden);C(out==0);}
    for(const auto& entry:Fixture{}.m){Fixture f;f.m[entry.first]=0;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    for(const auto& entry:Fixture{}.m){Fixture f;f.change_address=entry.first;f.change_value=entry.second+4;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    {Fixture f;f.change_owner=true;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    {Fixture f;f.m[f.manager+0xCE4]=f.primary;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    {Fixture f;f.m[f.secondary+0x74]=f.owner.session.self.address;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    {Fixture f;f.owner.session.sub0.think_mode=3;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);C(f.reads==0);}
    {Fixture f;f.owner.actor=f.owner.session.self.address;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);C(f.reads==0);}
    std::printf("Genesis owner camera: %d checks PASS\n",checks);
}
