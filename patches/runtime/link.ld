OUTPUT_FORMAT(elf32-i386)
ENTRY(rev_runtime_entry)
rev_original_entry = 0x01C897E7;
rev_action_draw_continue = 0x01EB73A9;
rev_hud_stock_self = 0x01C2C7F4;
rev_menu_stock_self = 0x01C2C7F4;
rev_menu_stock_state_transition = 0x02CDD180;
INCLUDE patches/hud_ownership/lifetime_continuations.ld
INCLUDE patches/hud_ownership/pipeline_continuations.ld
INCLUDE patches/hud_ownership/manager_continuations.ld
SECTIONS {
 . = 0x05800000;
 .revtext : { *(.text*) *(.rodata*) }
 . = ALIGN(4096);
 .revdata : {
   *(.data*)
   . = ALIGN(4);
   rev_init_begin = .; KEEP(*(.init_array*)) rev_init_end = .;
   *(.bss*) *(COMMON)
 }
 /DISCARD/ : { *(.eh_frame*) *(.note*) *(.comment*) *(.llvm_addrsig) }
}
