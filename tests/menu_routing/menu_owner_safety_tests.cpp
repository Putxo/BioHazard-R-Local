#include "menu_owner.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_menu;
static unsigned assertions=0,scenarios=0;
#define CHECK(x) do { ++assertions; if(!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
struct Fixture {
    std::map<u32,u32> memory;
    u32 active=1,sub=0x200000,reads=0;
    static constexpr u32 pad=0x100000;
    MenuOwnerRouter router;
    Fixture():router(Access{this,read,local,partner}) {
        memory[pad+0x970]=0;
        memory[pad+0x198]=0x101; memory[pad+0x1A0]=0; memory[pad+0x1AC]=0x102;
        memory[pad+0x2F8+0x198]=0x201; memory[pad+0x2F8+0x1A0]=0; memory[pad+0x2F8+0x1AC]=0x202;
        memory[sub+0xE3C]=1; memory[sub+0xE40]=1;
    }
    static bool read(void* ctx,u32 address,u32* out) noexcept {
        auto& f=*static_cast<Fixture*>(ctx); ++f.reads;
        auto it=f.memory.find(address);
        if(it==f.memory.end()) return false;
        *out=it->second; return true;
    }
    static u32 local(void* ctx) noexcept { return static_cast<Fixture*>(ctx)->active; }
    static u32 partner(void* ctx) noexcept { return static_cast<Fixture*>(ctx)->sub; }
    void open(bool pause) {
        memory[pad+0x2F8+0x1A0]=pause?8:1;
        if(pause) router.pause_open_word(pad); else router.submenu_open_word(pad);
        CHECK(router.owner()==1);
    }
};
int main() {
    // Previously these wrapped into mapped low addresses and admitted a fake actor.
    { Fixture f; f.sub=0xFFFFF200;
      f.memory[f.sub+0xE3C]=1; f.memory[f.sub+0xE40]=1;
      f.memory[f.pad+0x2F8+0x1A0]=8;
      CHECK(f.router.pause_open_word(f.pad)==0); CHECK(f.router.owner()==0);
      CHECK(f.router.surface()==Surface::None); ++scenarios; }
    { Fixture f; f.active=0; const u32 badpad=0xFFFFF800;
      f.memory[badpad+0x970]=0; f.memory[badpad+0x1A0]=8;
      CHECK(f.router.pause_open_word(badpad)==0); CHECK(f.reads==0); ++scenarios; }
    // Observe invalidation on a foreign surface; restoring validity must not rebind.
    for(u32 cause=0;cause<5;++cause) for(bool pause: {false,true}) {
        Fixture f; f.open(pause);
        if(cause==0) f.active=0;
        if(cause==1) f.memory[f.sub+0xE40]=2;
        if(cause==2) f.memory[f.sub+0xE3C]=2;
        if(cause==3) f.memory.erase(f.sub+0xE40);
        if(cause==4) { f.sub=0x300000; f.memory[f.sub+0xE40]=1; f.memory[f.sub+0xE3C]=1; }
        if(pause) CHECK(f.router.submenu_actor(0x400000)==0x400000);
        else CHECK(f.router.pause_word_1ac(f.pad)==0x102);
        CHECK(f.router.owner()==0); CHECK(f.router.surface()==Surface::None);
        f.active=1; f.sub=0x200000; f.memory[f.sub+0xE40]=1; f.memory[f.sub+0xE3C]=1;
        if(pause) CHECK(f.router.pause_word_1ac(f.pad)==0x102);
        else CHECK(f.router.submenu_actor(0x400000)==0x400000);
        ++scenarios;
    }
    for(bool pause: {false,true}) {
        Fixture f; f.open(pause);
        u32 offset=pause?0x1AC:0x198;
        f.memory.erase(f.pad+0x2F8+offset);
        if(pause) CHECK(f.router.pause_word_1ac(f.pad)==0x102);
        else CHECK(f.router.submenu_word_198(f.pad)==0x101);
        CHECK(f.router.owner()==0); CHECK(f.router.surface()==Surface::None);
        f.memory[f.pad+0x2F8+offset]=0x999;
        if(pause) CHECK(f.router.pause_word_1ac(f.pad)==0x102);
        else CHECK(f.router.submenu_word_198(f.pad)==0x101);
        ++scenarios;
    }
    // Preserve upstream nested state semantics, including a real 8->6->8 return.
    for(bool pause: {false,true}) for(u32 nested: {6u,7u}) {
        Fixture f; f.open(pause);
        f.router.state_transition(nested); CHECK(f.router.owner()==1);
        f.router.state_transition(pause?5:8); CHECK(f.router.owner()==1);
        CHECK(f.router.surface()==(pause?Surface::Pause:Surface::SubMenu));
        f.router.state_transition(1); CHECK(f.router.owner()==0);
        f.router.state_transition(pause?5:8); CHECK(f.router.owner()==0);
        ++scenarios;
    }
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"assertions\":%u,\"gameplay_executed\":false}\n",scenarios,assertions);
}
