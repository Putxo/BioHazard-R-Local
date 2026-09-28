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
rev_runtime_stock_immediate = 0x01BCD42A;
rev_damage_actor_state = 0x01BDA229;
rev_damage_state_value = 0x01C09353;
rev_damage_health_ratio = 0x01B7DFCE;
rev_damage_set_percent = 0x01C90F33;
rev_damage_set_visible = 0x01C7799D;
rev_genesis_stock_read = 0x01B82362;
rev_genesis_stock_set = 0x01BFA407;
rev_genesis_stock_add = 0x01BB2404;
rev_genesis_stock_selected_actor = 0x01C07341;
rev_genesis_stock_camera = 0x01C5741D;
rev_genesis_target_born_resume = 0x0281AE2B;
rev_genesis_target_dying_resume = 0x02823A5C;
rev_genesis_effect_filter_born_resume = 0x02933454;
rev_genesis_effect_filter_dying_resume = 0x0293357C;
rev_genesis_effect_noise_born_resume = 0x029355A4;
rev_genesis_effect_noise_dying_resume = 0x0293581C;
rev_genesis_effect_outline_born_resume = 0x037DD30F;
rev_genesis_effect_outline_dying_resume = 0x037DD4A8;
rev_hud_stock_heal = 0x01C88B6C;
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

rev_genesis_target_focus_resume = 0x0281F569;
rev_genesis_target_position_resume = 0x02823229;
rev_genesis_detect_stock = 0x01BBECD1;
rev_genesis_detector_camera_stock = 0x01C5741D;
rev_genesis_detector_actor_stock = 0x01C2C7F4;
rev_genesis_detector_widget_stock = 0x01B86273;
rev_genesis_candidate_stock = 0x01C0582A;
rev_genesis_set_focus_resume = 0x0281F519;
rev_genesis_set_position_resume = 0x0281DDE9;
rev_genesis_target_cleanup_stock = 0x01C919B5;
rev_genesis_notify_stock = 0x01C315EC;
rev_aim_visibility_resume = 0x0279D11A;
rev_scope_actor_stock = 0x01C2C7F4;
rev_scope_phase8_end = 0x02B41EF0;
rev_genesis_scan_end = 0x02B26E7E;
rev_genesis_scan_stock = 0x01C41E92;
rev_genesis_state_end = 0x02B2F9C2;
rev_genesis_state_stock = 0x01C0CE54;
rev_genesis_phase9_end = 0x02B201BF;
rev_genesis_filter_draw_resume = 0x02933B29;
rev_genesis_noise_draw_resume = 0x02935669;
rev_genesis_outline_draw_resume = 0x037DE566;
rev_genesis_weapon_born_resume = 0x02487324;
rev_genesis_weapon_dying_resume = 0x0248742C;
rev_genesis_weapon_retain_resume = 0x02B20D9F;
rev_genesis_weapon_activate_end = 0x02B21097;
rev_genesis_effect_toggle_resume = 0x02B23129;
rev_genesis_color_draw_enabled_stock = 0x01C42635;
rev_genesis_hunter_cache_stock = 0x01BE0142;
rev_genesis_hunter_material_bind_stock = 0x01C3B12D;
rev_genesis_hunter_material_unbind_stock = 0x01B9E855;
