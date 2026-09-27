#include "../../patches/genesis/equipment.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
using namespace rev_genesis;
int checks=0;
#define C(v) do {++checks;if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::exit(1);}}while(0)
struct Fixture {
    static constexpr u32 widget=0x10000,primary=0x20000,secondary=0x30000,weapon=0xA0000;
    Owner owner{{true,9,{0x40000,2,0,1},{0x50000,3,1,1}},0x50000};
    std::map<u32,u32> m;
    Mode mode=Mode::Local;u32 reads=0,resolves=0,fail=0,change_address=0,change_value=0;
    bool change_owner=false;
    Fixture(){
        m[owner.session.self.address+0x1524]=primary;m[owner.actor+0x1524]=secondary;
        m[0x04D2D440]=0x01BFECCD;
        for(u32 p:{primary,secondary}){
            m[p]=0x04D2D42C;m[p+0xD4]=2;
            for(u32 i=0;i<15;++i)m[p+4+4*i]=0;
        }
        m[secondary+12]=weapon;m[primary+12]=weapon+0x10000;
        // No mapping for either weapon: identity lookup must not dereference it.
    }
    ProgressHost host(){return {this,[](void* p,u32 w,Owner* o) noexcept {
        auto& f=*static_cast<Fixture*>(p);C(w==widget);++f.resolves;*o=f.owner;
        if(f.resolves==2){if(f.change_owner)++o->session.sub0.lifetime;
            if(f.change_address)f.m[f.change_address]=f.change_value;}
        return f.mode;
    }};}
    rev_hud::Reader reader(){return {this,[](void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fixture*>(p);++f.reads;
        if(f.reads==f.fail)return false;
        auto it=f.m.find(a);if(it==f.m.end())return false;*v=it->second;return true;
    }};}
    Mode get(u32* out){return equipment(host(),reader(),widget,out);}
};
int main(){
    {Fixture f;u32 out=99;const auto before=f.m;C(f.get(&out)==Mode::Local);C(out==f.weapon);C(f.m==before);C(f.reads==76);}
    for(u32 fail=1;fail<=76;++fail){Fixture f;f.fail=fail;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    for(const auto& e:Fixture{}.m){Fixture f;f.change_address=e.first;f.change_value=e.second+4;
        u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    {Fixture f;f.change_owner=true;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    for(u32 index:{0u,1u,14u,15u,0xFFFFFFFFu}){Fixture f;f.m[f.secondary+0xD4]=index;
        u32 out=99;C(f.get(&out)==Mode::Local);C(out==0);}
    for(u32 i=0;i<15;++i){Fixture f;f.m[f.primary+4+i*4]=f.weapon;
        u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    for(u32 p:{0u,Fixture::primary,Fixture::primary+4,0xFFFFFF30u}){
        Fixture f;f.m[f.owner.actor+0x1524]=p;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    {Fixture f;f.m[f.secondary]=0x04D2D428;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    {Fixture f;f.mode=Mode::Stock;u32 out=99;C(f.get(&out)==Mode::Stock);C(out==99);C(f.reads==0);}
    {Fixture f;f.mode=Mode::Hidden;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);C(f.reads==0);}
    {Fixture f;C(f.get(nullptr)==Mode::Hidden);C(f.reads==0);}
    {Fixture f;u32 out=99;C(equipment({},f.reader(),f.widget,&out)==Mode::Stock);C(out==99);C(f.reads==0);}
    {Fixture f;f.owner.session.sub0.think_mode=3;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);C(f.reads==0);}
    {Fixture f;f.owner.actor=f.owner.session.self.address;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);C(f.reads==0);}
    {Fixture f;f.owner.actor=f.owner.session.sub0.address=0xFFFFEADC;u32 out=99;C(f.get(&out)==Mode::Hidden);C(out==0);}
    std::printf("Genesis equipment ownership: %d checks PASS\n",checks);
}
