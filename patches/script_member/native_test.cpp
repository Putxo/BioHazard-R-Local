// Freestanding Linux i386 test of the real assembled helper, not the game.
// No game EXE, library, asset or engine API is linked into this executable.
using U32=unsigned int;
extern "C" U32 fixture_active;
extern "C" U32 fixture_sub;
extern "C" U32 script_call_checked(void*,U32);
static_assert(sizeof(void*)==4,"run this test as i386");
alignas(16) U32 owner[0x1080/4];
alignas(16) U32 actor[0xE44/4];
static U32 failures;
static void check(bool b) { if(!b) ++failures; }
static U32 query() { return script_call_checked(owner,0xDEADC0DE); }
extern "C" int run_script_tests() {
    const U32 sub_vt=0x04DCF41C;
    const U32 ptr=reinterpret_cast<U32>(actor);
    const U32 flags[]={0,1,0xffffffff};
    const U32 types[]={sub_vt,0x04DCBFAC,0x04DBD47C,0x04E15EF4};
    const U32 contexts[]={0,ptr,1};
    const U32 serials[]={0xffffffff,1,127};
    U32 combinations=0;
    for(U32 flag:flags) for(U32 type:types) for(U32 tracked=0;tracked!=2;++tracked)
    for(U32 mode=0;mode!=4;++mode) for(U32 context:contexts)
    for(U32 serial:serials) for(U32 stored:serials) {
        fixture_active=flag;fixture_sub=tracked?ptr:0;
        owner[0]=type;owner[0x1078/4]=context;owner[0x1074/4]=stored;
        actor[0xE40/4]=mode;actor[0xE3C/4]=serial;
        U32 expected=(flag && type==sub_vt && tracked && mode==1 && context==ptr &&
                      !(serial&0x80000000) && stored==serial)?1U:0U;
        check(query()==expected);++combinations;
    }
    check(combinations==2592);
    // Invalid owner/context pointers must not be read on the guarded paths.
    check(script_call_checked(nullptr,0x12345678)==0);
    fixture_active=0;fixture_sub=1;
    check(script_call_checked(reinterpret_cast<void*>(1),0x12345678)==0);
    fixture_active=1;owner[0]=0x04E15EF4;
    check(query()==0); // unknown owner class must not dereference tracker=1
    owner[0]=sub_vt;fixture_sub=0;check(query()==0);
    fixture_sub=1;owner[0x1078/4]=0;check(query()==0); // context mismatch: tracker=1 must not be read
    fixture_sub=ptr;actor[0xE40/4]=1;actor[0xE3C/4]=1;
    owner[0x1078/4]=1;owner[0x1074/4]=1;check(query()==0);
    owner[0x1078/4]=ptr;
    const U32 large_serials[]={0U,17U,127U,128U,0x7fffffffU};
    for(U32 serial : large_serials) {
        actor[0xE3C/4]=owner[0x1074/4]=serial;check(query()==1);
    }
    actor[0xE40/4]=3;check(query()==0);
    actor[0xE40/4]=1;owner[0x1074/4]=7;check(query()==0);
    return failures ? 1 : 0;
}
