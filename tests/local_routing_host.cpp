// Host-only behavioral tests with mocked engine services; not Win32/gameplay.
#include <cstdio>
#include <initializer_list>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <cstdint>
#include "../patches/local_routing/core.cpp"
static unsigned checks=0;
#define CHECK(expr) do { ++checks; if(!(expr)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#expr);std::exit(1);} } while(0)
struct alignas(16) Object { U8 bytes[0x2000]{}; };
static Object p1,sub,npc,door,item,matrix,control;
static void* self_value=&p1; static void* find_value=nullptr;
static bool allowed_p1=true,allowed_sub=true; static int find_calls=0,set_calls=0;
static bool door_kind=true, matrix_ok=true;
static ActionGroup copied{};
extern "C" {
volatile U32 lc_active=0; void* volatile lc_sub0=nullptr;
U8 api_door_dti=0,api_charge_zero=0,api_quint_zero=0,api_ac_charge_zero=0,api_foot_zero=0;
void* api_self(){return self_value;}
void* TC api_find(void*,I32){++find_calls;return find_value;}
I32 TC api_local_serial(void*){return 7;}
bool TC api_allowed(void* p,I32 kind){CHECK(kind>=0 && kind<=3);return (p==&p1 && allowed_p1)||(p==&sub && allowed_sub);}
bool TC api_kind_of(void*,void* d){CHECK(d==&api_door_dti);return door_kind;}
U32 api_static_invoke(void* const*,U32){return 0;}
bool api_static_manage(void** a,void** b,U32 op){if(op==0)*b=*a;else if(op==1)*a=nullptr;else if(op==2)return *a==*b;return true;}
void* TC api_set_group(void* cmd,const ActionGroup* g){++set_calls;copied=*g;return cmd;}
}
static void* TC get_dti(void*){return &api_door_dti;}
static void* TC get_matrix(void*,I32 index){CHECK(index==-1);return matrix_ok?&matrix:nullptr;}
static void* vtable[25]{};
static void put(Object& o,unsigned off,I32 value){std::memcpy(o.bytes+off,&value,4);}
static void ptr(Object& o,unsigned off,void* value){std::memcpy(o.bytes+off,&value,sizeof(value));}
static void position(Object& o,float x,float y=0,float z=0){float a[3]={x,y,z};std::memcpy(o.bytes+0x40,a,12);}
static void reset(){
 lc_forget_all();lc_active=1;lc_sub0=&sub;self_value=&p1;find_value=nullptr;
 allowed_p1=allowed_sub=door_kind=matrix_ok=true;find_calls=set_calls=0;
 put(p1,0xE3C,0);put(p1,0xE40,1);put(sub,0xE3C,1);put(sub,0xE40,1);
 put(npc,0xE3C,2);put(npc,0xE40,2);position(p1,3);position(sub,1);
 vtable[4]=reinterpret_cast<void*>(get_dti);vtable[24]=reinterpret_cast<void*>(get_matrix);
 ptr(door,0,vtable);ptr(item,0xF48,nullptr);ptr(control,0x0C,&p1);
}
static void membership(){
 reset();CHECK(lc_actor_pad(&p1)==0);CHECK(lc_actor_pad(&sub)==1);CHECK(lc_actor_pad(&npc)==0);CHECK(lc_actor_pad(nullptr)==0);
 for(U32 flag: {0u,1u,2u}) for(int mode: {0,1,2,3}){
  lc_active=flag;put(sub,0xE40,mode);ptr(item,0xF48,&sub);
  const U32 expected=(flag!=0 && mode==1);CHECK(lc_actor_pad(&sub)==expected);CHECK(lc_item_member(&item,99)==expected);
  CHECK(lc_item_member(nullptr,0)==0);ptr(item,0xF48,&p1);CHECK(lc_item_member(&item,0)==0);
 }
 reset();lc_sub0=nullptr;ptr(item,0xF48,nullptr);CHECK(lc_item_member(&item,0)==0);CHECK(lc_actor_pad(nullptr)==0);
}
static void candidates(){
 reset();CHECK(lc_begin(&door)==&p1);CHECK(lc_is_managed(&door));CHECK(!lc_is_managed(nullptr));
 CHECK(!lc_consider(&door,&npc));CHECK(!lc_consider(&door,nullptr));
 CHECK(lc_consider(&door,&p1));CHECK(lc_consider(&door,&sub));CHECK(lc_wait_actor(&door)==&sub);
 CHECK(lc_model_member(&door,0)==1);CHECK(lc_wait_serial(&door,&sub)==1);CHECK(lc_wait_allowed(&door,&sub,0));CHECK(!lc_wait_allowed(&door,&p1,0));
 CHECK(lc_begin(&door)==&p1);CHECK(lc_model_member(&door,0)==0);CHECK(!lc_wait_allowed(&door,&p1,0));
 position(p1,1);CHECK(lc_consider(&door,&sub));CHECK(lc_consider(&door,&p1));CHECK(lc_wait_actor(&door)==&p1);
 CHECK(lc_model_member(&door,0)==0);CHECK(lc_wait_serial(&door,&p1)==0);
 lc_begin(&door);allowed_p1=false;CHECK(lc_availability(&door,&p1,0));CHECK(!lc_consider(&door,&p1));CHECK(lc_consider(&door,&sub));
 allowed_sub=false;CHECK(!lc_wait_allowed(&door,&sub,0));CHECK(!lc_availability(&door,&p1,0));
 reset();lc_begin(&door);put(sub,0xE3C,0);CHECK(!lc_consider(&door,&sub));
 for(int serial:{-1,128,255}){put(sub,0xE3C,serial);CHECK(!lc_consider(&door,&sub));}
 put(sub,0xE3C,127);CHECK(lc_consider(&door,&sub));CHECK(lc_wait_serial(&door,&sub)==127);
 lc_begin(&door);position(sub,std::numeric_limits<float>::quiet_NaN());CHECK(!lc_consider(&door,&sub));
 position(sub,std::numeric_limits<float>::infinity());CHECK(!lc_consider(&door,&sub));
 position(sub,3.4e38F);CHECK(!lc_consider(&door,&sub));position(sub,1);matrix_ok=false;CHECK(!lc_consider(&door,&sub));
 matrix_ok=true;CHECK(lc_consider(&door,&sub));door_kind=false;CHECK(lc_model_member(&door,0)==0);door_kind=true;
 // Stale pointers are compared with current live actor identities before reads.
 binding(&door,false)->actor=reinterpret_cast<void*>(std::uintptr_t(1));CHECK(lc_model_member(&door,0)==0);CHECK(lc_wait_actor(&door)==&p1);
 reset();lc_begin(&door);CHECK(lc_consider(&door,&sub));lc_reset_owner(&door);CHECK(lc_model_member(&door,0)==0);
 lc_active=0;CHECK(lc_begin(&door)==&p1);CHECK(!lc_consider(&door,&sub));CHECK(lc_wait_actor(&door)==&p1);CHECK(lc_wait_serial(&door,&sub)==7);
 CHECK(lc_wait_allowed(&door,&p1,0));CHECK(!lc_is_managed(&door));CHECK(lc_model_member(&door,0)==0);
 reset();self_value=nullptr;CHECK(lc_begin(&door)==&sub);CHECK(lc_consider(&door,&sub));CHECK(lc_wait_actor(&door)==&sub);
 lc_forget_all();CHECK(!lc_is_managed(&door));
 // Capacity exhaustion must never write outside the table or crash.
 for(unsigned i=0;i<512;++i)lc_reset_owner(reinterpret_cast<void*>(std::uintptr_t(0x100000+i*0x100)));
 CHECK(!binding(&door,true));CHECK(lc_wait_actor(&door)==nullptr);lc_forget_all();
}
static void finders(){
 reset();CHECK(lc_find_actor(&door,1)==&sub);CHECK(find_calls==1);CHECK(lc_find_actor(&door,0)==nullptr);CHECK(lc_find_actor(nullptr,1)==nullptr);
 find_value=&npc;CHECK(lc_find_actor(&door,1)==&npc);find_value=nullptr;lc_active=0;CHECK(!lc_find_actor(&door,1));
 lc_active=1;put(sub,0xE40,3);CHECK(!lc_find_actor(&door,1));put(sub,0xE40,1);
 for(int serial:{-1,128,255})CHECK(!lc_find_actor(&door,serial));
}
static void rescue(){
 reset();CHECK(lc_rescue_other(&p1,&npc)==&sub);CHECK(lc_rescue_other(&sub,&npc)==&p1);CHECK(lc_rescue_other(&npc,&npc)==&npc);
 CHECK(lc_rescue_other(nullptr,&npc)==&npc);CHECK(lc_rescue_control(&control,&npc)==&sub);
 ptr(control,0x0C,&sub);CHECK(lc_rescue_control(&control,&npc)==&p1);CHECK(lc_rescue_control(nullptr,&npc)==&npc);
 lc_active=0;CHECK(lc_rescue_other(&p1,&npc)==&npc);CHECK(lc_rescue_control(&control,&npc)==&npc);
 lc_active=1;put(sub,0xE40,3);CHECK(lc_rescue_other(&p1,&npc)==&npc);put(sub,0xE40,1);
 put(p1,0xE40,3);CHECK(lc_rescue_other(&sub,&npc)==&npc);put(p1,0xE40,1);
 put(sub,0xE3C,0);CHECK(lc_rescue_other(&p1,&npc)==&npc);put(sub,0xE3C,1);
 self_value=nullptr;CHECK(lc_rescue_other(&sub,&npc)==&npc);
}
static void groups(){
 reset();ActionGroup g{};g.callbacks[0]={&api_charge_zero,api_static_invoke,api_static_manage};
 for(int i=1;i<5;++i)g.callbacks[i]={&npc,api_static_invoke,api_static_manage};
 ActionGroup original=g;CHECK(lc_install_actor_group(&door,&g,&sub)==&door);CHECK(set_calls==1);
 CHECK(std::memcmp(&g,&original,sizeof g)==0);CHECK(copied.callbacks[0].context==&sub);CHECK(copied.callbacks[0].invoke==lc_delegate_pad);
 CHECK(copied.callbacks[0].manage==api_static_manage);CHECK(std::memcmp(copied.callbacks+1,g.callbacks+1,4*sizeof(ActionDelegate))==0);
 CHECK(copied.callbacks[0].invoke(&copied.callbacks[0].context,0)==1);lc_active=0;CHECK(copied.callbacks[0].invoke(&copied.callbacks[0].context,0)==0);
 CHECK(lc_delegate_pad(nullptr,0)==0);lc_active=1;put(sub,0xE40,3);CHECK(copied.callbacks[0].invoke(&copied.callbacks[0].context,0)==0);put(sub,0xE40,1);
 lc_install_actor_group(&door,&g,&p1);CHECK(std::memcmp(&copied,&g,sizeof g)==0);
 lc_active=0;lc_install_actor_group(&door,&g,&sub);CHECK(std::memcmp(&copied,&g,sizeof g)==0);lc_active=1;
 g.callbacks[0].context=&api_quint_zero;lc_install_actor_group(&door,&g,&sub);CHECK(copied.callbacks[0].invoke==lc_delegate_pad);
 g.callbacks[0].context=&npc;lc_install_actor_group(&door,&g,&sub);CHECK(std::memcmp(&copied,&g,sizeof g)==0);
 g.callbacks[0].context=&api_charge_zero;g.callbacks[0].manage=nullptr;lc_install_actor_group(&door,&g,&sub);CHECK(std::memcmp(&copied,&g,sizeof g)==0);
 CHECK(lc_install_actor_group(nullptr,&g,&sub)==nullptr);CHECK(lc_install_actor_group(&door,nullptr,&sub)==nullptr);
 for(void* name:{static_cast<void*>(&api_ac_charge_zero),static_cast<void*>(&api_foot_zero)}){
  g.callbacks[0]={name,api_static_invoke,api_static_manage};lc_active=0;put(sub,0xE40,2);
  CHECK(lc_install_component_group(&door,&g,&sub)==&door);CHECK(copied.callbacks[0].invoke==lc_delegate_pad);
  CHECK(copied.callbacks[0].invoke(&copied.callbacks[0].context,0)==0);
  lc_active=1;put(sub,0xE40,1);CHECK(copied.callbacks[0].invoke(&copied.callbacks[0].context,0)==1);
  put(sub,0xE40,3);CHECK(copied.callbacks[0].invoke(&copied.callbacks[0].context,0)==0);
 }
 CHECK(lc_install_component_group(nullptr,&g,&sub)==nullptr);CHECK(lc_install_component_group(&door,nullptr,&sub)==nullptr);
 lc_install_component_group(&door,&g,nullptr);CHECK(std::memcmp(&copied,&g,sizeof g)==0);
}
int main(){membership();candidates();finders();rescue();groups();std::printf("{\"status\":\"PASS\",\"host_assertions\":%u,\"gameplay_executed\":false,\"win32_abi_executed\":false}\n",checks);}
