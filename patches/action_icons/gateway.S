.intel_syntax noprefix
.code32
.text
.extern rev_action_draw
.extern rev_action_draw_continue
# Original thiscall entry: ECX=icon, one render-context argument, ret 4.
.global rev_action_draw_gate
.type rev_action_draw_gate,@function
rev_action_draw_gate:
    push dword ptr [esp+4]
    push ecx
    call rev_action_draw
    add esp,8
    ret 4
.size rev_action_draw_gate,.-rev_action_draw_gate

# Replays exactly the nine overwritten bytes of 01EB73A0, then resumes stock.
# Used only by the native draw callback, never as the public hook entry.
.global rev_action_stock_draw
.type rev_action_stock_draw,@function
rev_action_stock_draw:
    push ebp
    mov ebp,esp
    sub esp,0xFC
    jmp rev_action_draw_continue
.size rev_action_stock_draw,.-rev_action_stock_draw
.section .note.GNU-stack,"",@progbits
