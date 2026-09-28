using u32=unsigned int;
extern "C" {
u32 call_bind(u32,u32,u32);u32 call_unbind(u32);
extern u32 seen_begin_hunter,seen_begin_context,seen_bind_context,seen_bind_material,seen_unbind_context,seen_end;
}
extern "C" int run_tests(){
 if(call_bind(0x11110000,0x22220000,0x33330000)!=0xB16DB16D)return 1;
 if(seen_begin_hunter!=0x11110000||seen_begin_context!=0x22220000)return 2;
 if(seen_bind_context!=0x22220000||seen_bind_material!=0x33330000)return 3;
 if(call_unbind(0x44440000)!=0xABCD1234)return 4;
 if(seen_unbind_context!=0x44440000||seen_end!=1)return 5;
 const char m[]="{\"status\":\"PASS\",\"gateways\":2,\"native_i386\":true,\"gameplay_executed\":false}\n";
 asm volatile("int $0x80"::"a"(4),"b"(1),"c"(m),"d"(sizeof(m)-1):"memory");
 return 0;
}
