# Synthetic Linux i386 caller and continuations. No game code or assets.
.intel_syntax noprefix
.code32
.text
.global _start
_start:
    mov dword ptr [case_no],0
.Lnext:
    call test_one
    test eax,eax
    jne .Lfailed
    inc dword ptr [case_no]
    cmp dword ptr [case_no],12
    jb .Lnext
    mov eax,4
    mov ebx,1
    mov ecx,offset pass_msg
    mov edx,offset pass_len
    int 0x80
    xor eax,eax
    jmp .Lexit
.Lfailed:
    mov eax,[case_no]
    inc eax
.Lexit:
    mov ebx,eax
    mov eax,1
    int 0x80

test_one:
    push ebp
    push ebx
    push esi
    push edi
    mov [saved_esp],esp
    mov eax,[case_no]
    mov edx,[destinations+eax*4]
    mov [destination],edx
    mov edx,[sites+eax*4]
    mov [expected_site],edx
    mov dword ptr [callback_error],0
    mov dword ptr [callback_count],0
    mov dword ptr [fake_object+0x50],0xAABBCC00
    cmp eax,11
    jne .Lseedready
    mov byte ptr [fake_object+0x50],0xAB
.Lseedready:
    fninit
    fld1
    movdqu xmm0,[xmm_seed]
    ldmxcsr [mxcsr_seed]
    fxsave [fx_before]
    mov ebp,offset fake_frame+128
    mov dword ptr [ebp-8],offset fake_object
    mov dword ptr [ebp-0x2C],0
    mov esp,offset test_stack+8192
    mov [initial_esp],esp
    mov dword ptr [esp],0x77777777
    mov dword ptr [esp+4],0x66666666
    mov dword ptr [esp+8],0x33333333
    mov eax,0x11221122
    mov ecx,offset fake_object
    mov edx,0x33443344
    mov ebx,0x55555555
    mov esi,0x88888888
    mov edi,0x99999999
    stc
    std
    pushfd
    pop dword ptr [initial_flags]
    jmp dword ptr [destination]

# All returning original instruction windows feed the same synthetic verifier.
.macro continuation name
.global rev_life_continue_\name
rev_life_continue_\name:
    jmp .Lverify
.endm
continuation npc_born
continuation cockpit_born
continuation minimap_born
continuation npc_dying
continuation player_dying
continuation cockpit_dying
continuation minimap_dying
continuation main_dying
continuation sub_dying
continuation bind_begin
continuation bind_end
.Lverify:
    mov [actual+0],eax
    mov [actual+4],ecx
    mov [actual+8],edx
    mov [actual+12],ebx
    mov [actual+16],esi
    mov [actual+20],edi
    mov [actual+24],ebp
    mov [actual+28],esp
    pushfd
    pop dword ptr [actual+32]
    fxsave [fx_after]
    cld
    mov eax,1
    cmp dword ptr [callback_error],0
    jne .Ldone
    cmp dword ptr [callback_count],1
    jne .Ldone
    # Defined FP/SIMD state: FCW/FSW/FTW, MXCSR, 80-bit ST0 and XMM0.
    mov edx,[fx_before]
    cmp edx,[fx_after]
    jne .Ldone
    mov dl,[fx_before+4]
    cmp dl,[fx_after+4]
    jne .Ldone
    mov edx,[fx_before+24]
    cmp edx,[fx_after+24]
    jne .Ldone
    mov esi,offset fx_before+32
    mov edi,offset fx_after+32
    mov ecx,10
    repe cmpsb
    jne .Ldone
    mov esi,offset fx_before+160
    mov edi,offset fx_after+160
    mov ecx,16
    repe cmpsb
    jne .Ldone
    mov ecx,[case_no]
    cmp ecx,3
    jb .Lconstructor
    cmp ecx,9
    jb .Ldestructor
    # Binder sites preserve arithmetic flags and use caller EBP.
    mov edx,[actual+32]
    xor edx,[initial_flags]
    test edx,0xCD5
    jnz .Ldone
    cmp dword ptr [actual+24],offset fake_frame+128
    jne .Ldone
    cmp dword ptr [actual+12],0x55555555
    jne .Ldone
    cmp dword ptr [actual+16],0x88888888
    jne .Ldone
    cmp dword ptr [actual+20],0x99999999
    jne .Ldone
    cmp ecx,10
    je .Lbindend
    # Corrected begin replays EAX=PCS and MOVZX of exactly its byte +50.
    cmp dword ptr [actual],offset fake_object
    jne .Ldone
    movzx edx,byte ptr [fake_object+0x50]
    cmp edx,[actual+4]
    jne .Ldone
    cmp dword ptr [actual+8],0x33443344
    jne .Ldone
    cmp dword ptr [fake_frame+128-0x2C],0
    jne .Ldone
    mov edx,[initial_esp]
    cmp edx,[actual+28]
    jne .Ldone
    jmp .Lsuccess
.Lbindend:
    cmp dword ptr [actual],0x11221122
    jne .Ldone
    cmp dword ptr [actual+4],offset fake_frame+128
    jne .Ldone
    cmp dword ptr [actual+8],offset rev_life_binder_debug_table
    jne .Ldone
    mov edx,[initial_esp]
    sub edx,8
    cmp edx,[actual+28]
    jne .Ldone
    cmp dword ptr [edx],0x11221122
    jne .Ldone
    cmp dword ptr [edx+4],0x33443344
    jne .Ldone
    jmp .Lsuccess
.Lconstructor:
    cmp dword ptr [actual],offset fake_object
    jne .Ldone
    cmp dword ptr [actual+4],offset fake_object
    jne .Ldone
    cmp dword ptr [actual+8],0x33443344
    jne .Ldone
    cmp dword ptr [actual+12],0x33333333
    jne .Ldone
    cmp dword ptr [actual+16],0x66666666
    jne .Ldone
    cmp dword ptr [actual+20],0x77777777
    jne .Ldone
    cmp dword ptr [actual+24],offset fake_frame+128
    jne .Ldone
    mov edx,[initial_esp]
    add edx,12
    cmp edx,[actual+28]
    jne .Ldone
    mov edx,[actual+32]
    xor edx,[initial_flags]
    test edx,0xCD5
    jnz .Ldone
    jmp .Lsuccess
.Ldestructor:
    cmp dword ptr [actual],0x11221122
    jne .Ldone
    cmp dword ptr [actual+4],offset fake_object
    jne .Ldone
    cmp dword ptr [actual+8],0x33443344
    jne .Ldone
    cmp dword ptr [actual+12],0x55555555
    jne .Ldone
    cmp dword ptr [actual+16],0x88888888
    jne .Ldone
    cmp dword ptr [actual+20],0x99999999
    jne .Ldone
    mov edx,[initial_esp]
    sub edx,4
    cmp edx,[actual+24]
    jne .Ldone
    cmp dword ptr [edx],offset fake_frame+128
    jne .Ldone
    sub edx,[sizes+ecx*4]
    pushfd
    pop esi
    cmp edx,[actual+28]
    jne .Ldone
    # SUB produces new arithmetic flags; compare to replay, preserve DF.
    xor esi,[actual+32]
    test esi,0x8D5
    jnz .Ldone
    test dword ptr [actual+32],0x400
    jz .Ldone
.Lsuccess:
    xor eax,eax
.Ldone:
    mov esp,[saved_esp]
    cld
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

.global rev_hud_lifetime_event
rev_hud_lifetime_event:
    pushfd
    pop eax
    test eax,0x400
    jnz .Lbadcallback
    mov eax,esp
    and eax,15
    cmp eax,12
    jne .Lbadcallback
    mov eax,[esp+4]
    cmp eax,[expected_site]
    jne .Lbadcallback
    cmp dword ptr [esp+8],offset fake_object
    jne .Lbadcallback
    jmp .Lcallback
.Lbadcallback:
    mov dword ptr [callback_error],1
.Lcallback:
    inc dword ptr [callback_count]
    fninit
    pxor xmm0,xmm0
    ldmxcsr [mxcsr_other]
    mov eax,0xAAAAAAAA
    mov ecx,0xBBBBBBBB
    mov edx,0xCCCCCCCC
    clc
    ret
.section .rodata
.align 16
xmm_seed: .long 0x12345678,0x98ABCDEF,0x76543210,0xFFEEDDCC
mxcsr_seed: .long 0x1F80
mxcsr_other: .long 0x3F80
sites: .long 0x0277D4FB,0x02B47907,0x02B67F51,0x0277DC70,0x027B5D00,0x02B47A30,0x02B68040,0x02DF5970,0x02DF5F90,0x02DF4F65,0x02DF504B,0x02DF4F65
sizes: .long 0,0,0,0xD8,0xCC,0x2C8,0xF4,0xCC,0xCC,0,0,0
destinations: .long rev_life_gate_npc_born,rev_life_gate_cockpit_born,rev_life_gate_minimap_born,rev_life_gate_npc_dying,rev_life_gate_player_dying,rev_life_gate_cockpit_dying,rev_life_gate_minimap_dying,rev_life_gate_main_dying,rev_life_gate_sub_dying,rev_life_gate_bind_begin,rev_life_gate_bind_end,rev_life_gate_bind_begin
pass_msg: .ascii "{\"status\":\"PASS\",\"gateways\":11,\"invocations\":12,\"begin_byte_cases\":2,\"native_i386\":true,\"receiver_mocked\":true,\"gameplay_executed\":false}\n"
.set pass_len,.-pass_msg
.bss
.align 16
fx_before: .skip 512
fx_after: .skip 512
actual: .skip 36
case_no: .skip 4
initial_flags: .skip 4
initial_esp: .skip 4
saved_esp: .skip 4
expected_site: .skip 4
destination: .skip 4
callback_error: .skip 4
callback_count: .skip 4
.global rev_life_binder_debug_table
rev_life_binder_debug_table: .skip 16
fake_object: .skip 96
fake_frame: .skip 256
.align 16
test_stack: .skip 16384
.section .note.GNU-stack,"",@progbits
