using u32=unsigned int;
extern "C" {
void rev_menu_gate_pause_open();void rev_menu_gate_pause_1a0();void rev_menu_gate_pause_1ac();
void rev_menu_gate_submenu_open();void rev_menu_gate_submenu_198();void rev_menu_gate_submenu_1a0();
u32 call_pad_gate(u32,u32,void(*)());u32 call_actor_gate(u32);u32 call_state_gate(u32,u32,u32);
extern u32 seen_pad,seen_actor,seen_state_this,seen_state,seen_flag,seen_observed_state;
extern u32 stack_before,stack_after,seen_incoming_eax,seen_incoming_edx;
}
extern "C" int run_tests(){
 void(*g[])()={rev_menu_gate_pause_open,rev_menu_gate_pause_1a0,rev_menu_gate_pause_1ac,rev_menu_gate_submenu_open,rev_menu_gate_submenu_198,rev_menu_gate_submenu_1a0};
 u32 want[]={0x8008,0xA1A0,0xA1AC,0x9001,0xB198,0xB1A0};
 for(u32 i=0;i<6;++i){seen_pad=0;if(call_pad_gate(0x12340000,0,g[i])!=want[i]||seen_pad!=0x12340000||stack_before!=stack_after)return 10+i;}
 seen_actor=0;if(call_actor_gate(0x77770000)!=0x33330010||seen_actor!=0x33330000)return 20;
 seen_state_this=seen_state=seen_flag=seen_observed_state=0;
 if(call_state_gate(0x44440000,6,1)!=0xC0DEC0DE)return 30;
 if(seen_state_this!=0x44440000||seen_state!=6||seen_flag!=1||seen_observed_state!=6)return 31;
 if(stack_before!=stack_after||seen_incoming_eax!=0x11223344||seen_incoming_edx!=0x55667788)return 32;
 const char msg[]="{\"status\":\"PASS\",\"gateways\":8,\"native_i386\":true,\"engine_mocked\":true,\"gameplay_executed\":false}\n";
 asm volatile("int $0x80"::"a"(4),"b"(1),"c"(msg),"d"(sizeof(msg)-1):"memory");
 return 0;
}
