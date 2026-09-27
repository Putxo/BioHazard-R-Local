.intel_syntax noprefix
.code32
.text
.global rev_runtime_draw_schedule_gate
.type rev_runtime_draw_schedule_gate,@function
rev_runtime_draw_schedule_gate:
    call rev_runtime_stock_immediate
    push eax
    push dword ptr [ebp+8]
    call rev_runtime_draw_schedule
    add esp,8
    ret
.size rev_runtime_draw_schedule_gate,.-rev_runtime_draw_schedule_gate
.section .note.GNU-stack,"",@progbits
