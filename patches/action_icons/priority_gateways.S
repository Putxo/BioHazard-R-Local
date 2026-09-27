.intel_syntax noprefix
.code32
.text
.macro observe name,callback
.global \name
\name:
    pushfd
    pushad
    push ebp
    call \callback
    add esp,4
    popad
    popfd
.endm
observe rev_priority_begin_gate,rev_priority_begin
    lea eax,[ebp-0x11C]
    push eax
    jmp rev_priority_begin_continue

.global rev_priority_prepare_gate
rev_priority_prepare_gate:
    push ecx
    call rev_priority_stock_command_priority
    push eax
    push dword ptr [esp+4]
    push ebp
    call rev_priority_prepare
    add esp,8
    pop eax
    add esp,4
    ret

.global rev_priority_decide_gate
rev_priority_decide_gate:
    pushfd
    pushad
    setle al
    movzx eax,al
    push eax
    push ebp
    call rev_priority_decide
    add esp,8
    cmp eax,0
    je .proceed
    cmp eax,1
    je .next
    popad
    popfd
    jmp rev_priority_loop_end
.proceed:
    popad
    popfd
    jmp rev_priority_decide_continue
.next:
    popad
    popfd
    jmp rev_priority_loop_next

.global rev_priority_commit_gate
rev_priority_commit_gate:
    pushfd
    pushad
    push ebp
    call rev_priority_commit
    add esp,4
    test eax,eax
    jz .abort
    popad
    popfd
    push 0
    lea ecx,[ebp-0x10C]
    jmp rev_priority_commit_continue
.abort:
    popad
    popfd
    jmp rev_priority_loop_end

observe rev_priority_finish_gate,rev_priority_finish
    mov eax,[ebp-8]
    mov ecx,[ebp-0x100]
    jmp rev_priority_finish_continue

.global rev_priority_draw_getter_gate
rev_priority_draw_getter_gate:
    push ecx
    call rev_priority_stock_draw_priority
    push eax
    push dword ptr [esp+4]
    call rev_action_draw_priority
    add esp,12
    ret
.section .note.GNU-stack,"",@progbits
