.intel_syntax noprefix
.code32
.text
.global _start
_start:
    and esp,-16
    call run_manager_gateway_tests
    mov ebx,eax
    mov eax,1
    int 0x80

# cdecl test(gateway, owner, context, family). Entire test is sequential.
.global run_gateway_once
run_gateway_once:
    push ebp
    mov ebp,esp
    push ebx
    push esi
    push edi
    mov eax,[ebp+8]
    mov [target],eax
    mov eax,[ebp+12]
    mov [fake_frame+8],eax
    mov edx,[ebp+16]
    mov [fake_frame+24],edx
    mov edx,[ebp+20]
    mov [family],edx
    mov [saved_sp],esp
    fninit
    fld1
    fldz
    ldmxcsr [seed_mxcsr]
    movdqa xmm0,[pattern]
    movdqa xmm1,[pattern]
    movdqa xmm2,[pattern]
    movdqa xmm3,[pattern]
    movdqa xmm4,[pattern]
    movdqa xmm5,[pattern]
    movdqa xmm6,[pattern]
    movdqa xmm7,[pattern]
    fxsave [gate_before_fx]
    std
    cmp eax,eax
    pushfd
    pop dword ptr [seed_flags]
    cmp dword ptr [family],0
    je .cockpit_flags
    mov eax,[fake_frame+8]
    add eax,0x30
    pushfd
    pop dword ptr [gate_expected_flags]
    jmp .seeds
.cockpit_flags:
    mov eax,[seed_flags]
    mov [gate_expected_flags],eax
.seeds:
    push dword ptr [seed_flags]
    popfd
    mov eax,0x11223344
    mov ecx,0x22334455
    mov edx,0x33445566
    mov ebx,0x44556677
    mov esi,0x55667788
    mov edi,0x66778899
    lea ebp,[fake_frame+16]
    call dword ptr [target]
    mov [gate_result],eax
    mov [gate_result+4],ecx
    mov [gate_result+8],edx
    mov [gate_result+12],ebx
    mov [gate_result+16],esi
    mov [gate_result+20],edi
    mov [gate_result+24],ebp
    mov [gate_result+28],esp
    pushfd
    pop dword ptr [gate_result+32]
    fxsave [gate_after_fx]
    cld
    fninit
    ldmxcsr [default_mxcsr]
    mov eax,[saved_sp]
    mov [gate_expected_sp],eax
    mov esp,[saved_sp]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

# Fake engine continuations instead of fixed game addresses; not game code.
.macro continuation name
.global rev_hud_continue_\name
rev_hud_continue_\name:
    ret
.endm
continuation cockpit8
continuation cockpit9
continuation cockpit11
continuation minimap8
continuation minimap9
continuation minimap11

# Synthetic cdecl receiver deliberately changes caller-saved machine state.
# No dependency on a Windows game, symbols or any game resource.
.global rev_hud_manager_event
rev_hud_manager_event:
    mov eax,[esp+4]
    mov [gate_args],eax
    mov eax,[esp+8]
    mov [gate_args+4],eax
    mov eax,[esp+12]
    mov [gate_args+8],eax
    mov eax,esp
    and eax,15
    mov [gate_args+12],eax
    pushfd
    pop eax
    mov [gate_args+16],eax
    pxor xmm0,xmm0
    pxor xmm1,xmm1
    pxor xmm2,xmm2
    pxor xmm3,xmm3
    pxor xmm4,xmm4
    pxor xmm5,xmm5
    pxor xmm6,xmm6
    pxor xmm7,xmm7
    fninit
    ldmxcsr [default_mxcsr]
    mov eax,0xDEAD1234
    mov ecx,0xDEAD2345
    mov edx,0xDEAD3456
    std
    ret

.data
.balign 16
pattern: .long 0xA1B2C3D4,0x55AA55AA,0x01020304,0x88776655
seed_mxcsr: .long 0x5F80
default_mxcsr: .long 0x1F80
.bss
.balign 16
.global fake_frame
fake_frame: .skip 64
saved_sp: .skip 4
target: .skip 4
family: .skip 4
seed_flags: .skip 4
.global gate_expected_flags,gate_expected_sp,gate_result,gate_args
gate_expected_flags: .skip 4
gate_expected_sp: .skip 4
gate_result: .skip 36
gate_args: .skip 20
.balign 16
.global gate_before_fx,gate_after_fx
gate_before_fx: .skip 512
gate_after_fx: .skip 512
.section .note.GNU-stack,"",@progbits
