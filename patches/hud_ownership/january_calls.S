// cdecl entry bridges to the January MSVC i386 ABI. No engine addresses here.
// Tested with synthetic callees; fixed native targets live in JanuaryType.
#ifdef _WIN32
#define SYM(x) _##x
#else
#define SYM(x) x
#endif
.intel_syntax noprefix
.code32
.text
.global SYM(rev_january_cdecl0)
SYM(rev_january_cdecl0):
 push ebp
 mov ebp,esp
 and esp,-16
 call dword ptr [ebp+8]
 mov esp,ebp
 pop ebp
 ret
.global SYM(rev_january_cdecl2)
SYM(rev_january_cdecl2):
 push ebp
 mov ebp,esp
 and esp,-16
 sub esp,8
 push dword ptr [ebp+16]
 push dword ptr [ebp+12]
 call dword ptr [ebp+8]
 mov esp,ebp
 pop ebp
 ret
.global SYM(rev_january_this0)
SYM(rev_january_this0):
 push ebp
 mov ebp,esp
 and esp,-16
 mov ecx,dword ptr [ebp+12]
 call dword ptr [ebp+8]
 mov esp,ebp
 pop ebp
 ret
.global SYM(rev_january_this1)
SYM(rev_january_this1):
 push ebp
 mov ebp,esp
 and esp,-16
 sub esp,12
 push dword ptr [ebp+16]
 mov ecx,dword ptr [ebp+12]
 call dword ptr [ebp+8]
 mov esp,ebp
 pop ebp
 ret
#ifndef _WIN32
.section .note.GNU-stack,"",@progbits
#endif
