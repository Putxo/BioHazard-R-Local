#include "../../patches/genesis/target_views.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
using namespace rev_genesis;
int checks=0;
#define C(v) do {++checks;if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::exit(1);}}while(0)
constexpr u32 Born=0x0281AE26,Dying=0x02823A53,Widget=0x9000;
struct Fixture {
    std::map<u32,u32> memory;
    u32 thread=7,resolves=0;bool reenter=false,change=false,die=false;
    Mode mode=Mode::Local;
    ViewFrame frame{{{true,8,{0x50000,9,1,1},{0x60000,10,2,1}},0x60000},7};
    TargetKey key{};ViewSample sample{{0x3F800000,0xC0000000,0},3};
    TargetLifetime life{{{this,[](void* p,u32 a,u32* out) noexcept {
        auto& f=*static_cast<Fixture*>(p);auto it=f.memory.find(a);
        if(it==f.memory.end())return false;*out=it->second;return true;
    }},[](void* p) noexcept {return static_cast<Fixture*>(p)->thread;}}};
    TargetViews views{life,{this,[](void* p) noexcept {return static_cast<Fixture*>(p)->thread==7;},
        [](void* p,u32 w,ViewFrame* out) noexcept {
        auto& f=*static_cast<Fixture*>(p);C(w==Widget);++f.resolves;
        if(f.reenter){f.reenter=false;ViewSample nested{};C(f.views.read(w,f.key,&nested)==Mode::Hidden);}
        *out=f.frame;
        if(f.resolves==2){if(f.change)++out->owner.session.sub0.lifetime;
            if(f.die)C(f.life.event(Dying,f.key.address));}
        return f.mode;
    }}};
    Fixture(){C(life.start(7));key=born(0x1000);}
    TargetKey born(u32 a){memory[a]=0x04DA8570;C(life.event(Born,a));return life.capture(a);}
    Mode put(){resolves=0;return views.publish(Widget,key,sample);}
    Mode get(ViewSample& out){resolves=0;return views.read(Widget,key,&out);}
};
bool equal(ViewSample a,ViewSample b){for(int i=0;i<3;++i)if(a.position[i]!=b.position[i])return false;return a.focus==b.focus;}
int main(){
    {Fixture f;ViewSample out=f.sample;C(f.get(out)==Mode::Hidden);C(equal(out,{}));
     const auto before=f.memory;C(f.put()==Mode::Local);C(f.get(out)==Mode::Local);C(equal(out,f.sample));C(f.memory==before);
     ++f.frame.number;C(f.get(out)==Mode::Local);++f.frame.number;C(f.get(out)==Mode::Hidden);C(equal(out,{}));
     C(f.put()==Mode::Local);C(f.get(out)==Mode::Local);--f.frame.number;C(f.get(out)==Mode::Hidden);}
    {Fixture f;f.mode=Mode::Stock;ViewSample out=f.sample;C(f.put()==Mode::Stock);C(f.get(out)==Mode::Stock);C(equal(out,f.sample));}
    {Fixture f;C(f.put()==Mode::Local);f.thread=8;ViewSample out=f.sample;C(f.get(out)==Mode::Hidden);C(f.resolves==0);C(equal(out,{}));
     f.thread=7;C(f.get(out)==Mode::Local);}
    {Fixture f;f.reenter=true;C(f.put()==Mode::Local);ViewSample out{};f.reenter=true;C(f.get(out)==Mode::Local);C(equal(out,f.sample));}
    {Fixture f;f.change=true;C(f.put()==Mode::Hidden);f.change=false;ViewSample out{};C(f.get(out)==Mode::Hidden);}
    {Fixture f;C(f.put()==Mode::Local);f.die=true;ViewSample out=f.sample;C(f.get(out)==Mode::Hidden);C(equal(out,{}));}
    {Fixture f;C(f.put()==Mode::Local);auto old=f.key;C(f.life.event(Dying,old.address));f.key=f.born(old.address);
     C(f.key.generation!=old.generation);ViewSample out=f.sample;C(f.get(out)==Mode::Hidden);C(equal(out,{}));
     C(f.put()==Mode::Local);C(f.get(out)==Mode::Local);C(f.views.read(Widget,old,&out)==Mode::Hidden);}
    {Fixture f;C(f.put()==Mode::Local);++f.frame.owner.session.epoch;ViewSample out=f.sample;C(f.get(out)==Mode::Hidden);C(equal(out,{}));
     C(f.put()==Mode::Local);++f.frame.owner.session.self.lifetime;C(f.get(out)==Mode::Hidden);}
    for(u32 bad:{0x7F800000u,0xFF800000u,0x7FC00001u,0xFFFFFFFFu}){
     Fixture f;f.sample.position[1]=bad;C(f.put()==Mode::Hidden);}
    for(u32 bad:{4u,rev_hud::Invalid}){Fixture f;f.sample.focus=bad;C(f.put()==Mode::Hidden);}
    {Fixture f;f.frame.number=0;C(f.put()==Mode::Hidden);f.frame.number=1;f.frame.owner.session.sub0.think_mode=3;C(f.put()==Mode::Hidden);}
    {Fixture f;C(f.put()==Mode::Local);f.mode=Mode::Hidden;ViewSample out=f.sample;C(f.get(out)==Mode::Hidden);C(equal(out,{}));}
    {Fixture f;C(f.put()==Mode::Local);C(f.views.read(Widget,f.key,nullptr)==Mode::Hidden);}
    {Fixture f;const auto first=f.key;C(f.put()==Mode::Local);
     for(u32 i=1;i<TargetLifetime::Capacity;++i){f.key=f.born(0x1000+i*0x40);f.sample.focus=i%4;C(f.put()==Mode::Local);}
     f.key=first;ViewSample out{};C(f.get(out)==Mode::Local);C(out.focus==3);
     C(f.life.event(Dying,first.address));f.key=f.born(0x100000);C(f.get(out)==Mode::Hidden);C(f.put()==Mode::Local);}
    std::printf("Genesis view-derived target state: %d checks PASS\n",checks);
}
