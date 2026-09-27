using u32=unsigned int;
extern "C" {
void rev_menu_gate_pause_open();void rev_menu_gate_submenu_open();void rev_menu_gate_submenu_198();void rev_menu_gate_submenu_1a0();
u32 call_pad_gate(u32,u32,void(*)());u32 call_actor_gate(u32);
extern u32 seen_pad,seen_actor;
}
extern "C" int run_tests(){
 void(*g[])()={rev_menu_gate_pause_open,rev_menu_gate_submenu_open,rev_menu_gate_submenu_198,rev_menu_gate_submenu_1a0};
 u32 want[]={0x8008,0x9001,0xA198,0xA1A0};
 for(u32 i=0;i<4;++i){seen_pad=0;if(call_pad_gate(0x12340000,0,g[i])!=want[i]||seen_pad!=0x12340000)return 10+i;}
 seen_actor=0;if(call_actor_gate(0x77770000)!=0x33330010||seen_actor!=0x33330000)return 20;
 const char msg[]="{\"status\":\"PASS\",\"gateways\":5,\"native_i386\":true,\"engine_mocked\":true,\"gameplay_executed\":false}\n";
 asm volatile("int $0x80"::"a"(4),"b"(1),"c"(msg),"d"(sizeof(msg)-1):"memory");
 return 0;
}
