# Prompt de continuación

Continúa exclusivamente `Putxo/BioHazard-R-Local` desde el remoto actual. Verifica ramas/PRs antes de escribir y lee `CURRENT_STATUS.md`, `docs/45-local-menu-owner-routing.md`, `docs/46-pause-global-submenu-handoff.md`, `research/current_state.json` y `AGENTS.md`. La rama canónica es `research/script-member-serial-validation`, no `main`.

Último bloque integrado: PR #19, merge `b0900736d9e44a04a47f9804b75059f007dd12fd`, 15 workflows PASS. `MenuOwnerRouter` ya registra owner J1/J2 al abrir Pause/SubMenu, da prioridad a J1, usa Pad1 para la navegación de uGUI_SubMenu cuando owner=J2 y sustituye únicamente sus dos Self finders por el Sub0 exacto para leer su propio cBioItemPack. No modifica Self ni mStartPadNo global.

Continúa exactamente por **Pause owner=1**: localiza la ruta física de navegación/confirmar/cancelar/opciones mientras `sIDCockpit state=5`; owner0 debe seguir en Pad0 y owner1 usar Pad1 solo dentro de ese scope. Mantén una sola pausa global. `uGUI_PauseHD::slot8` consulta sGameFlags; no interpretes sus índices 0/1/2 como pads. Después implementa limpieza/lifetime de owner/surface al salir de states 5/8, audita states 6/7 y otros callers de la state machine.

Después: Genesis/acciones restantes -> scheduler/QTE/scripts -> muerte/reanimación/checkpoints -> cutscenes/cámaras forzadas -> escenas sin partner -> instalación conjunta y validación de campaña.

Preserva PadData[1]/ThinkMode Pad, split/cámaras, HUD P2/lifecycle/NativeViewScope/máscara P1, pickup Sub0, ammo relief, puertas/interacciones, guards PcsSub owner+serial y MenuOwnerRouter. No rehagas esos bloques.

Entrega solo fuentes/tests/evidencia/commits en GitHub. No generes EXE salvo petición expresa. No llames gameplay validado a CI/mocks.
