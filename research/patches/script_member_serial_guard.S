# Exact owner, live local actor and matching script serial. No engine calls.
# This is a thiscall member callback: ECX=owner, one ignored stack argument.
# EAX/EDX are volatile; ECX and all nonvolatile registers are preserved.
.intel_syntax noprefix
.code32
.text
.global script_member_serial_guard
script_member_serial_guard:
 xor eax,eax
 test ecx,ecx
 je .done
 cmp dword ptr [0x057D9188],0
 je .done
 cmp dword ptr [ecx],0x04DCF41C
 jne .done
 mov edx,dword ptr [0x057D9184]
 test edx,edx
 je .done
 cmp dword ptr [ecx+0x1078],edx
 jne .done
 cmp dword ptr [edx+0xE40],1
 jne .done
 mov edx,dword ptr [edx+0xE3C]
 test edx,edx
 js .done
 cmp dword ptr [ecx+0x1074],edx
 sete al
.done:
 ret 4
.section .note.GNU-stack,"",@progbits
