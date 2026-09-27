# Prompt para continuar

Continúa exclusivamente `Putxo/BioHazard-R-Local` desde la rama `research/script-member-serial-validation`. Lee `CURRENT_STATUS.md`, `docs/45-local-menu-owner-routing.md`, `docs/46-pause-global-submenu-handoff.md`, `research/current_state.json` y `AGENTS.md` antes de escribir.

Último handoff: PR #20, merge `6f19b0c9ccb4fcdabd8d35ada6af69d0bebdc284`. PR #19 ya integró MenuOwnerRouter: owner J1/J2 al abrir Pause/SubMenu, Pad1 para SubMenu owner J2 y Sub0 para sus dos rutas de inventario, sin cambiar Self ni mStartPadNo global.

Continúa exactamente por Pause owner=1: encuentra la navegación real de PauseHD/opciones y enruta confirmar/cancelar/navegación a Pad1 solo cuando owner=J2. Mantén una sola pausa global. Después limpia owner/surface al cerrar state 5/8 y audita states 6/7/callers alternativos.

Después sigue Genesis/QTE/scripts, muerte/checkpoints, cutscenes/cámaras forzadas, escenas sin partner e instalación/validación completa.

No rehagas pickup, HUD base, puertas generales ni v12/v13/v14. No generes EXE salvo petición expresa.
