.intel_syntax noprefix
.code32
.text
.global _start
_start:
    sub esp,0x300
    lea ebp,[esp+0x200]
    mov [before],esp
    mov ebx,0x12345678
    mov esi,0x23456789
    mov edi,0x3456789A
    mov dword ptr [ebp-8],0xABCD1000
    call rev_priority_begin_gate
    cmp dword ptr [seen_begin],1
    jne fail
    mov ecx,0xABCD2000
    call rev_priority_prepare_gate
    cmp eax,0x601
    jne fail
    cmp dword ptr [seen_prepare],1
    jne fail
    mov dword ptr [decision],0
    mov eax,3
    cmp eax,7
    call rev_priority_decide_gate
    cmp dword ptr [accepted],1
    jne fail
    cmp dword ptr [destination],0
    jne fail
    mov dword ptr [decision],1
    mov eax,7
    cmp eax,3
    call rev_priority_decide_gate
    cmp dword ptr [accepted],0
    jne fail
    cmp dword ptr [destination],1
    jne fail
    mov dword ptr [decision],2
    mov eax,0x80000000
    cmp eax,0x7FFFFFFF
    call rev_priority_decide_gate
    cmp dword ptr [accepted],1
    jne fail
    cmp dword ptr [destination],2
    jne fail
    mov dword ptr [commit_ok],1
    call rev_priority_commit_gate
    cmp dword ptr [destination],3
    jne fail
    mov dword ptr [commit_ok],0
    call rev_priority_commit_gate
    cmp dword ptr [destination],2
    jne fail
    call rev_priority_finish_gate
    cmp dword ptr [destination],4
    jne fail
    mov ecx,0xABCD3000
    call rev_priority_draw_getter_gate
    cmp eax,222
    jne fail
    cmp esp,[before]
    jne fail
    cmp ebx,0x12345678
    jne fail
    cmp esi,0x23456789
    jne fail
    cmp edi,0x3456789A
    jne fail
    lea eax,[esp+0x200]
    cmp ebp,eax
    jne fail
    xor ebx,ebx
    mov eax,1
    int 0x80
fail:
    mov ebx,1
    mov eax,1
    int 0x80

# All callbacks and continuations below are synthetic; no game code is loaded.
.global rev_priority_begin
rev_priority_begin:
    cmp [esp+4],ebp
    jne fail
    mov dword ptr [seen_begin],1
    ret
.global rev_priority_begin_continue
rev_priority_begin_continue:
    lea edx,[ebp-0x11C]
    pop eax
    cmp eax,edx
    jne fail
    ret
.global rev_priority_stock_command_priority
rev_priority_stock_command_priority:
    cmp ecx,0xABCD2000
    jne fail
    xor ecx,ecx
    mov eax,0x601
    ret
.global rev_priority_prepare
rev_priority_prepare:
    cmp [esp+4],ebp
    jne fail
    cmp dword ptr [esp+8],0xABCD2000
    jne fail
    mov dword ptr [seen_prepare],1
    mov eax,0xDEADBEEF
    ret
.global rev_priority_decide
rev_priority_decide:
    cmp [esp+4],ebp
    jne fail
    mov eax,[esp+8]
    mov [accepted],eax
    mov eax,[decision]
    ret
.global rev_priority_decide_continue
rev_priority_decide_continue:
    mov dword ptr [destination],0
    ret
.global rev_priority_loop_next
rev_priority_loop_next:
    mov dword ptr [destination],1
    ret
.global rev_priority_loop_end
rev_priority_loop_end:
    mov dword ptr [destination],2
    ret
.global rev_priority_commit
rev_priority_commit:
    cmp [esp+4],ebp
    jne fail
    mov eax,[commit_ok]
    ret
.global rev_priority_commit_continue
rev_priority_commit_continue:
    pop eax
    test eax,eax
    jne fail
    lea eax,[ebp-0x10C]
    cmp eax,ecx
    jne fail
    mov dword ptr [destination],3
    ret
.global rev_priority_finish
rev_priority_finish:
    cmp [esp+4],ebp
    jne fail
    mov dword ptr [ebp-0x100],0x456
    ret
.global rev_priority_finish_continue
rev_priority_finish_continue:
    cmp eax,0xABCD1000
    jne fail
    cmp ecx,0x456
    jne fail
    mov dword ptr [destination],4
    ret
.global rev_priority_stock_draw_priority
rev_priority_stock_draw_priority:
    cmp ecx,0xABCD3000
    jne fail
    xor ecx,ecx
    mov eax,99
    ret
.global rev_action_draw_priority
rev_action_draw_priority:
    cmp dword ptr [esp+4],0xABCD3000
    jne fail
    cmp dword ptr [esp+8],99
    jne fail
    mov eax,222
    ret
.bss
.align 4
before: .skip 4
seen_begin: .skip 4
seen_prepare: .skip 4
accepted: .skip 4
decision: .skip 4
destination: .skip 4
commit_ok: .skip 4
.section .note.GNU-stack,"",@progbits
