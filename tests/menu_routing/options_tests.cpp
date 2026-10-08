#include "menu_owner.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
using namespace rev_menu;
static unsigned checks=0,scenarios=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct F {
    static constexpr u32 Pad=0x100000,Actor=0x200000,Manager=0x300000,Global=0x400000;
    static constexpr u32 Config=Manager+0x4C,Reference=Manager+0x150;
    static constexpr u32 Offsets[5]={0x10,0xC,0x18,0x20,0x1C};
    std::map<u32,u32> m;
    u32 active=1,sub=Actor,thread=7,reads=0,applies=0,publishes=0,fail_write=0;
    u32 member=1,primary=0,last_config=0,last_save=0,last_flag=0,result=1;
    u32 read_fault=0,apply_fault=0,publish_fault=0;
    Access access;MenuOwnerRouter router;
    F(u32 owner=1,u32 main=0):member(owner),primary(main),access{this,word,local,sub0,
        {this,write,tid,read_native,apply_native,publish_native}},router(access){
        m[Pad]=0x04E11D30;m[0x057A7480]=Pad;m[Pad+0x970]=main;
        m[Actor+0xE3C]=3;m[Actor+0xE40]=1;m[Manager]=0x04DF5AE4;
        m[Pad+0x1A0]=owner==0?8:0;m[Pad+0x2F8+0x1A0]=owner==1?8:0;
        m[0x055926F4]=Global;
        for(u32 slot=0;slot<2;++slot)for(u32 i=0;i<5;++i)m[Pad+0x668+slot*0xC0+Offsets[i]]=10*slot+i+1;
        for(u32 i=0;i<65;++i)m[Global+0x3C+i*4]=300+i;
    }
    static bool word(void* p,u32 a,u32* v) noexcept {
        auto& f=*static_cast<F*>(p);auto it=f.m.find(a);if(it==f.m.end())return false;*v=it->second;return true;
    }
    static bool write(void* p,u32 a,u32 v) noexcept {
        auto& f=*static_cast<F*>(p);if(a==f.fail_write)return false;f.m[a]=v;return true;
    }
    static u32 local(void* p) noexcept{return static_cast<F*>(p)->active;}
    static u32 sub0(void* p) noexcept{return static_cast<F*>(p)->sub;}
    static u32 tid(void* p) noexcept{return static_cast<F*>(p)->thread;}
    static void read_native(void* p,u32 a) noexcept {
        auto& f=*static_cast<F*>(p);++f.reads;
        for(u32 i=0;i<65;++i)f.m[a+i*4]=100+i;
        if(f.read_fault==1)f.m[Actor+0xE3C]++;
        if(f.read_fault==2)f.router.clear();
        if(f.read_fault==3){f.router.clear();f.router.pause_open_word(Pad);}
        if(f.read_fault==4)f.router.options_read(a,0); // Suppressed reentry.
    }
    static u32 apply_native(void* p,u32 a,u32 save,u32 flag) noexcept {
        auto& f=*static_cast<F*>(p);++f.applies;f.last_config=a;f.last_save=save;f.last_flag=flag;
        for(u32 i=0;i<5;++i){
            if(f.apply_fault==1 && i==1)f.router.clear();
            if(f.apply_fault==2 && i==1){f.router.clear();f.router.pause_open_word(Pad);}
            const u32 slot=f.router.options_index(Pad,f.primary);
            C(slot==f.member); // Even after revocation, never switch remaining setters to J1.
            f.m[Pad+0x668+slot*0xC0+Offsets[i]]=f.m[a+4+i*4];
            C(f.router.options_index(Pad+4,9)==9);
        }
        if(f.apply_fault==3){C(f.router.options_apply(a,save,flag)==0);f.router.options_read(a,0);}
        if(f.apply_fault==4){++f.thread;C(f.router.options_index(Pad,9)==9);--f.thread;}
        return f.result;
    }
    static void publish_native(void* p,u32 global,const u32* copy) noexcept {
        auto& f=*static_cast<F*>(p);++f.publishes;C(global==Global);C(f.applies==0);
        C(f.router.options_index(Pad,9)==9); // Scope begins only at native application.
        for(u32 i=0;i<65;++i)f.m[global+0x3C+i*4]=copy[i];
        if(f.publish_fault)f.router.clear();
    }
    void open(){C(router.pause_open_word(Pad)==8);C(router.owner()==member);}
    void load(){open();router.options_read(Config,0);router.options_read(Reference,1);}
    void edit(){for(u32 i=0;i<5;++i)m[Config+4+i*4]=50+i;m[Config+0x20]=999;}
};
int main(){
    for(u32 member:{0u,1u})for(u32 primary:{0u,1u})for(u32 keyboard:{0u,1u}){
        F f(member,primary);
        if(keyboard){f.m[F::Pad+0x668+member*0xC0+0x10]=6;f.m[F::Pad+0x668+member*0xC0+0x14]=4;}
        f.load();C(f.reads==2);
        for(u32 i=0;i<5;++i){const u32 expected=keyboard && i==0?4:10*member+i+1;
            C(f.m[F::Config+4+i*4]==expected);C(f.m[F::Reference+4+i*4]==expected);}
        const auto before=f.m;f.edit();C(f.router.options_apply(F::Config,1,7)==1);
        C(f.applies==1 && f.publishes==1 && f.last_save==0 && f.last_flag==7);
        for(u32 i=0;i<5;++i){C(f.m[F::Pad+0x668+member*0xC0+F::Offsets[i]]==50+i);
            const u32 other=F::Pad+0x668+(1-member)*0xC0+F::Offsets[i];C(f.m[other]==before.at(other));
            C(f.m[F::Global+0x40+i*4]==(member?301+i:50+i));}
        C(f.m[F::Global+0x3C+0x20]==999);C(f.m[F::Pad+0x970]==primary);
        C(f.router.options_index(F::Pad,9)==9);++scenarios;
    }
    for(u32 save:{0u,2u,0x100u,0x101u})for(u32 global:{0u,F::Global}){
        F f;f.load();f.edit();f.m[0x055926F4]=global;f.result=0;
        C(f.router.options_apply(F::Config,save,0)==0);C(f.applies==1);
        C(f.publishes==(((save&255)==1 && global)?1u:0u));++scenarios;
    }
    for(u32 fault=0;fault<12;++fault){
        F f;f.load();f.edit();const auto before=f.m;
        switch(fault){
        case 0:f.m[F::Actor+0xE3C]++;break;
        case 1:f.m[F::Actor+0xE40]=3;break;
        case 2:f.sub+=0x10000;break;
        case 3:f.active=0;break;
        case 4:f.m[F::Manager]=0;break;
        case 5:f.m[0x057A7480]+=0x10000;break;
        case 6:f.m[F::Pad]=0;break;
        case 7:++f.thread;break;
        case 8:f.router.clear();break;
        case 9:f.router.pause_open_word(F::Pad);break;
        case 10:f.router.state_transition(8);break;
        case 11:f.m.erase(F::Actor+0xE3C);break;
        }
        C(f.router.options_apply(F::Config,1,0)==0);C(f.applies==0 && f.publishes==0);
        f.m=before;f.thread=7;f.sub=F::Actor;f.active=1;
        C(f.router.options_apply(F::Config,1,0)==0);C(f.applies==0);++scenarios;
    }
    for(u32 field=0;field<65;++field){F f;f.load();f.m.erase(F::Config+4*field);
        C(f.router.options_apply(F::Config,1,0)==0);C(!f.applies && !f.publishes);++scenarios;}
    for(u32 field=1;field<=5;++field){F f;f.load();f.m.erase(F::Global+0x3C+field*4);
        C(f.router.options_apply(F::Config,1,0)==0);C(!f.applies && !f.publishes);++scenarios;}
    for(u32 ref=0;ref<2;++ref)for(u32 field=0;field<5;++field){F f;f.open();
        if(ref)f.router.options_read(F::Config,0);
        f.fail_write=(ref?F::Reference:F::Config)+4+field*4;
        f.router.options_read(ref?F::Reference:F::Config,ref);
        f.fail_write=0;f.router.options_read(F::Reference,1);
        C(f.router.options_apply(F::Config,1,0)==0);C(!f.applies && !f.publishes);++scenarios;}
    for(u32 fault=1;fault<=4;++fault){F f;f.read_fault=fault;f.load();
        C(f.router.options_apply(F::Config,1,0)==(fault==4?1u:0u));C(f.applies==(fault==4?1u:0u));++scenarios;}
    for(u32 fault=1;fault<=4;++fault){F f;f.load();f.edit();f.apply_fault=fault;
        C(f.router.options_apply(F::Config,0,0)==1);C(f.applies==1);C(f.router.options_index(F::Pad,9)==9);
        if(fault<3){C(f.router.options_apply(F::Config,0,0)==0);C(f.applies==1);}++scenarios;}
    {F f;f.open();f.router.options_read(F::Config,0);C(f.router.options_apply(F::Config,1,0)==0);C(!f.applies);++scenarios;}
    {F f;f.load();f.publish_fault=1;C(f.router.options_apply(F::Config,1,0)==0);C(!f.applies && f.publishes==1);++scenarios;}
    {F f;f.load();f.router.options_read(F::Reference+4,1);C(f.reads==2);
        C(f.router.options_apply(F::Config,1,0)==0);C(!f.applies);++scenarios;}
    {F f;f.load();++f.thread;f.router.options_read(F::Reference,1);C(f.reads==2);--f.thread;
        C(f.router.options_apply(F::Config,1,0)==0);C(!f.applies);++scenarios;}
    {F f;f.open();f.m[F::Actor+0xE40]=3;f.router.options_read(F::Config,0);C(!f.reads);
        f.m[F::Actor+0xE40]=1;C(f.router.options_apply(F::Config,1,0)==0);C(!f.applies);++scenarios;}
    {F f(0,0);f.active=0;f.router.options_read(F::Config,0);f.edit();
        C(f.router.options_apply(F::Config,2,9)==1);C(f.last_save==2 && f.last_flag==9 && !f.publishes);++scenarios;}
    std::printf("{\"status\":\"PASS\",\"scenarios\":%u,\"checks\":%u,\"engine_mocked\":true,\"game_executed\":false}\n",scenarios,checks);
}
