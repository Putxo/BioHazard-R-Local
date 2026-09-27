#include "menu_owner.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_menu;
static unsigned assertions=0,scenarios=0;
#define CHECK(x) do{++assertions;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct F {
    std::map<u32,u32> m; u32 active=0,sub=0;
    Access access; MenuOwnerRouter router;
    u32 pad=0x100000,actor=0x200000,stock_actor=0x300000;
    F():access{this,read,active_cb,sub_cb},router(access) {
        m[pad+0x970]=0;
        m[pad+0x198]=0x11110000; m[pad+0x1A0]=0; m[pad+0x1AC]=0x1111AC00;
        m[pad+0x2F8+0x198]=0x22220000; m[pad+0x2F8+0x1A0]=0; m[pad+0x2F8+0x1AC]=0x2222AC00;
        m[actor+0xE3C]=1; m[actor+0xE40]=1;
    }
    static bool read(void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<F*>(p); auto i=f.m.find(a);
        if(i==f.m.end()) return false;
        *v=i->second;
        return true;
    }
    static u32 active_cb(void* p) noexcept {return static_cast<F*>(p)->active;}
    static u32 sub_cb(void* p) noexcept {return static_cast<F*>(p)->sub;}
    void local(){active=1;sub=actor;}
};
int main(){
    {F f;f.m[f.pad+0x970]=1;f.m[f.pad+0x2F8+0x1A0]=0x1234;
     CHECK(f.router.submenu_open_word(f.pad)==0x1234);CHECK(f.router.owner()==0);CHECK(f.router.surface()==Surface::None);++scenarios;}
    {F f;f.local();f.m[f.pad+0x1A0]=1;f.m[f.pad+0x2F8+0x1A0]=1;
     CHECK(f.router.submenu_open_word(f.pad)&1);CHECK(f.router.owner()==0);CHECK(f.router.surface()==Surface::SubMenu);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=1;
     CHECK(f.router.submenu_open_word(f.pad)&1);CHECK(f.router.owner()==1);CHECK(f.router.surface()==Surface::SubMenu);
     CHECK(f.router.submenu_actor(f.stock_actor)==f.actor);
     CHECK(f.router.submenu_word_198(f.pad)==0x22220000);CHECK(f.router.submenu_word_1a0(f.pad)==1);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=8;
     CHECK(f.router.pause_open_word(f.pad)&8);CHECK(f.router.owner()==1);CHECK(f.router.surface()==Surface::Pause);
     CHECK(f.router.pause_word_1a0(f.pad)==8);CHECK(f.router.pause_word_1ac(f.pad)==0x2222AC00);
     CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);CHECK(f.router.submenu_word_198(f.pad)==0x11110000);++scenarios;}
    {F f;f.local();f.m[f.pad+0x1A0]=8;f.m[f.pad+0x2F8+0x1A0]=8;
     CHECK(f.router.pause_open_word(f.pad)&8);CHECK(f.router.owner()==0);
     CHECK(f.router.pause_word_1ac(f.pad)==0x1111AC00);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=8;CHECK(f.router.pause_open_word(f.pad)&8);
     f.m[f.actor+0xE40]=2;CHECK(f.router.pause_word_1a0(f.pad)==0);CHECK(f.router.pause_word_1ac(f.pad)==0x1111AC00);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=1;CHECK(f.router.submenu_open_word(f.pad)&1);
     f.m[f.actor+0xE40]=2;CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);
     CHECK(f.router.submenu_word_198(f.pad)==0x11110000);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=1;CHECK(f.router.submenu_open_word(f.pad)&1);
     f.active=0;CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);++scenarios;}
    {F f;f.local();f.m[f.actor+0xE3C]=128;f.m[f.pad+0x2F8+0x1A0]=1;
     CHECK((f.router.submenu_open_word(f.pad)&1)==0);CHECK(f.router.owner()==0);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=1;CHECK(f.router.submenu_open_word(f.pad)&1);
     f.router.clear();CHECK(f.router.owner()==0);CHECK(f.router.surface()==Surface::None);CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);++scenarios;}
    // A replaced actor or recycled serial must never inherit ownership.
    for(u32 change=0;change<5;++change) {
        F f; f.local(); f.m[f.pad+0x2F8+0x1A0]=1;
        CHECK(f.router.submenu_open_word(f.pad)==1);
        if(change==0) { f.sub+=0x10000; f.m[f.sub+0xE3C]=1; f.m[f.sub+0xE40]=1; }
        if(change==1) f.m[f.actor+0xE3C]=2;
        if(change==2) f.m.erase(f.actor+0xE40);
        if(change==3) f.active=0;
        if(change==4) f.m[f.actor+0xE40]=2;
        CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);
        CHECK(f.router.owner()==0); CHECK(f.router.surface()==Surface::None);
        f.local(); f.m[f.actor+0xE3C]=1; f.m[f.actor+0xE40]=1;
        CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);
        CHECK(f.router.submenu_word_198(f.pad)==0x11110000); ++scenarios;
    }
    // Every departure (including unaudited nested 6/7) revokes the owner.
    for(u32 origin: {5u,8u}) for(u32 next=0;next<=9;++next) {
        F f; f.local(); f.router.observe_state(0x400000,2);
        f.m[f.pad+0x2F8+0x1A0]=origin==5?8:1;
        if(origin==5) f.router.pause_open_word(f.pad); else f.router.submenu_open_word(f.pad);
        f.router.observe_state(0x400000,origin);
        CHECK(f.router.owner()==1);
        f.router.observe_state(0x400000,next);
        CHECK(f.router.owner()==(next==origin?1u:0u));
        CHECK(f.router.surface()==(next==origin?static_cast<Surface>(origin):Surface::None));
        if(next!=origin) {
            f.router.observe_state(0x400000,origin);
            CHECK(f.router.owner()==0); // An alternate caller cannot resurrect it.
        }
        ++scenarios;
    }
    for(u32 reset=0;reset<3;++reset) {
        F f; f.local(); f.router.observe_state(0x400000,2);
        f.m[f.pad+0x2F8+0x1A0]=8; f.router.pause_open_word(f.pad);
        f.router.observe_state(0x400000,5);
        if(reset==0) f.router.observe_state(0x500000,5);
        if(reset==1) f.router.observe_state(0,5);
        if(reset==2) f.router.reset_session();
        CHECK(f.router.owner()==0); CHECK(f.router.surface()==Surface::None);
        CHECK(f.router.pause_word_1ac(f.pad)==0x1111AC00); ++scenarios;
    }
    {F f; f.local(); f.m[f.pad+0x2F8+0x1A0]=8; f.router.pause_open_word(f.pad);
     f.m.erase(f.pad+0x2F8+0x1AC);
     CHECK(f.router.pause_word_1ac(f.pad)==0x1111AC00); CHECK(f.router.owner()==0);
     f.m[f.pad+0x2F8+0x1AC]=0xABC;
     CHECK(f.router.pause_word_1ac(f.pad)==0x1111AC00); ++scenarios;}
    {F f; f.local(); f.m[f.pad+0x970]=1; f.m[f.pad+0x1A0]=8;
     f.router.pause_open_word(f.pad);
     CHECK(f.router.pause_word_1ac(f.pad)==0x1111AC00);
     CHECK(f.m[f.pad+0x970]==1); ++scenarios;}
    {F f; f.local(); f.sub=0xFFFFF200;
     f.m[f.sub+0xE3C]=1; f.m[f.sub+0xE40]=1;
     f.m[f.pad+0x2F8+0x1A0]=8;
     CHECK(f.router.pause_open_word(f.pad)==0); CHECK(f.router.owner()==0); ++scenarios;}
    {F f; f.m[0xFFFFFF00u+0x970u]=0; f.m[0xFFFFFF00u+0x1A0u]=8;
     CHECK(f.router.pause_open_word(0xFFFFFF00u)==0); ++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false}\n",scenarios,assertions);
}
