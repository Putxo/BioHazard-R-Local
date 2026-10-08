.intel_syntax noprefix
.code32
.text
# Original uPcsInput member callback returns0 and ret4; its argument is unused.
.global rev_script_input_gate
.type rev_script_input_gate,@function
rev_script_input_gate:
    push ecx
    call rev_script_input_member
    add esp,4
    ret 4
.size rev_script_input_gate,.-rev_script_input_gate
.section .note.GNU-stack,"",@progbits
