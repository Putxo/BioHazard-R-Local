using u32=unsigned int;
extern "C" {
void rev_hud_gate_cockpit8(); void rev_hud_gate_cockpit9(); void rev_hud_gate_cockpit11();
void rev_hud_gate_minimap8(); void rev_hud_gate_minimap9(); void rev_hud_gate_minimap11();
void run_gateway_once(void(*)(),u32,u32,u32);
extern u32 gate_result[9],gate_args[5],gate_expected_flags,gate_expected_sp;
extern unsigned char fake_frame[64],gate_before_fx[512],gate_after_fx[512];
}
static u32 owner[32];
static bool same(u32 begin,u32 size) {
    for(u32 i=begin;i<begin+size;++i) if(gate_before_fx[i]!=gate_after_fx[i])return false;
    return true;
}
extern "C" int run_manager_gateway_tests(){
    void (*functions[])()={rev_hud_gate_cockpit8,rev_hud_gate_cockpit9,rev_hud_gate_cockpit11,
                          rev_hud_gate_minimap8,rev_hud_gate_minimap9,rev_hud_gate_minimap11};
    const u32 sites[]={0x02B497CA,0x02B49C5B,0x02B49DE3,0x02B6847B,0x02B685B3,0x02B68724};
    const u32 p=reinterpret_cast<u32>(owner);
    owner[15]=0xABCDEF01;
    for(u32 i=0;i<6;++i){
        run_gateway_once(functions[i],p,0x87654321,i>=3);
        if(gate_args[0]!=sites[i] || gate_args[1]!=p ||
           gate_args[2]!=(i%3==2?0x87654321u:0u))return 10+i;
        if(gate_args[3]!=12 || (gate_args[4]&0x400))return 20+i;
        if(gate_result[0]!=(i>=3?p+0x30:p) ||
           gate_result[1]!=(i>=3?0x22334455u:0xABCDEF01u) || gate_result[2]!=0x33445566)return 30+i;
        if(gate_result[3]!=0x44556677 || gate_result[4]!=0x55667788 || gate_result[5]!=0x66778899 ||
           gate_result[6]!=reinterpret_cast<u32>(fake_frame+16))return 40+i;
        if(gate_result[7]!=gate_expected_sp || ((gate_result[8]^gate_expected_flags)&0xCD5))return 50+i;
        // Defined FXSAVE fields only; reserved bytes are intentionally excluded.
        if(!same(0,5) || !same(24,4) || !same(160,128))return 60+i;
        for(u32 r=0;r<8;++r) if(!same(32+r*16,10))return 70+i;
    }
    const char msg[]="{\"status\":\"PASS\",\"gateways\":6,\"gpr_flags_stack_fx_verified\":true,\"native_i386\":true,\"gameplay_executed\":false}\n";
    asm volatile("int $0x80"::"a"(4),"b"(1),"c"(msg),"d"(sizeof(msg)-1):"memory");
    return 0;
}
