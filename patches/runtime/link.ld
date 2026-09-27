OUTPUT_FORMAT(elf32-i386)
ENTRY(rev_runtime_entry)
rev_original_entry = 0x01C897E7;
rev_action_draw_continue = 0x01EB73A9;
rev_action_stock_mask = 0x01BE2D20;
rev_priority_begin_continue = 0x01EA61AE;
rev_priority_stock_command_priority = 0x01BEEF99;
rev_priority_decide_continue = 0x01EA623C;
rev_priority_loop_next = 0x01EA61BB;
rev_priority_loop_end = 0x01EA6694;
rev_priority_commit_continue = 0x01EA6681;
rev_priority_finish_continue = 0x01EA66BF;
rev_priority_stock_draw_priority = 0x01C6FF63;
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
