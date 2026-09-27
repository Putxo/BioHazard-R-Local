.intel_syntax noprefix
.code32
.text
# A CALL plus one NOP replaces a six-byte ECX load. The return address points
# to that NOP. On refusal skip the native use, retaining the caller's cleanup.
.macro USE name, sound, skip, args
.global rev_genesis_weapon_\name\()_gate
rev_genesis_weapon_\name\()_gate:
    pushfd
    pushad
    push \sound
    push eax
    call rev_genesis_weapon_use
    add esp,8
    test eax,eax
    jz 1f
    popad
    popfd
    mov ecx,dword ptr [eax+0x2FC]
    ret
1:
    add dword ptr [esp+36],\skip
    popad
    popfd
    ret \args
.endm
# getter(no args), mov ECX,EAX, command(ret8). Drop the two pending args.
USE command,1,13,8
# Wrapper has not pushed its two activate arguments yet. Skip to its epilogue.
USE reactivate,0,14,0
.section .note.GNU-stack,"",@progbits
