# Trabajo y entrega

Actualización canónica: 27 de septiembre de 2026.

- Rama técnica: `research/script-member-serial-validation`.
- Último bloque integrado: PR #19, merge `b0900736d9e44a04a47f9804b75059f007dd12fd`, 15 workflows PASS.
- Entrega: fuentes/tests/evidencia/PRs. No generar ni entregar EXE salvo petición expresa.
- Nunca subir binarios propietarios, PDB, assets ni .obj.

Leer antes de escribir:
1. CURRENT_STATUS.md
2. docs/45-local-menu-owner-routing.md
3. docs/46-pause-global-submenu-handoff.md
4. research/current_state.json
5. START_HERE_NEW_CHAT.md

Punto exacto: terminar **Pause owner=1**. MenuOwnerRouter ya registra owner y SubMenu J2 ya usa Pad1/Sub0. Falta navegación/opciones/confirmar/cancelar de PauseHD en Pad1 cuando owner=J2 y limpiar owner al cerrar state 5/8.

No duplicar PauseHD. No cambiar Self, mStartPadNo, GameMode, serial o Network globalmente. Los índices 0/1/2 de PauseHD::slot8 son sGameFlags, no pads.

Después: Genesis -> QTE/scripts -> muerte/checkpoints -> cutscenes -> escenas sin partner -> instalación y campaña.

Preflight remoto, preservar trabajo paralelo y distinguir CI/mocks de gameplay real.
