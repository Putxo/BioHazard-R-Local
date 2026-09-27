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
        m[pad+0x198]=0x11110000; m[pad+0x1A0]=0;
        m[pad+0x2F8+0x198]=0x22220000; m[pad+0x2F8+0x1A0]=0;
        m[actor+0xE3C]=1; m[actor+0xE40]=1;
    }
    static bool read(void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<F*>(p); auto i=f.m.find(a); if(i==f.m.end())return false;*v=i->second;return true;
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
     CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);CHECK(f.router.submenu_word_198(f.pad)==0x11110000);++scenarios;}
    {F f;f.local();f.m[f.pad+0x1A0]=8;f.m[f.pad+0x2F8+0x1A0]=8;
     CHECK(f.router.pause_open_word(f.pad)&8);CHECK(f.router.owner()==0);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=1;CHECK(f.router.submenu_open_word(f.pad)&1);
     f.m[f.actor+0xE40]=2;CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);
     CHECK(f.router.submenu_word_198(f.pad)==0x11110000);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=1;CHECK(f.router.submenu_open_word(f.pad)&1);
     f.active=0;CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);++scenarios;}
    {F f;f.local();f.m[f.actor+0xE3C]=128;f.m[f.pad+0x2F8+0x1A0]=1;
     CHECK((f.router.submenu_open_word(f.pad)&1)==0);CHECK(f.router.owner()==0);++scenarios;}
    {F f;f.local();f.m[f.pad+0x2F8+0x1A0]=1;CHECK(f.router.submenu_open_word(f.pad)&1);
     f.router.clear();CHECK(f.router.owner()==0);CHECK(f.router.surface()==Surface::None);CHECK(f.router.submenu_actor(f.stock_actor)==f.stock_actor);++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false}\n",scenarios,assertions);
}
