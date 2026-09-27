# BioHazard-R-Local

Cooperativo local en una sola instancia para Resident Evil Revelations 1 PC.

**Estado:** implementación fuente avanzada pero incompleta; campaña completa no validada jugando.

## Continuar

Rama canónica: `research/script-member-serial-validation`.

Leer:
- [CURRENT_STATUS.md](CURRENT_STATUS.md)
- [START_HERE_NEW_CHAT.md](START_HERE_NEW_CHAT.md)
- [CONTINUE_IN_NEW_CHAT.md](CONTINUE_IN_NEW_CHAT.md)
- [docs/45-local-menu-owner-routing.md](docs/45-local-menu-owner-routing.md)
- [docs/46-pause-global-submenu-handoff.md](docs/46-pause-global-submenu-handoff.md)
- [research/current_state.json](research/current_state.json)

Último bloque integrado: **PR #19**, merge `b0900736d9e44a04a47f9804b75059f007dd12fd`, 15 workflows PASS.

MenuOwnerRouter ya distingue J1/J2 al abrir Pause/SubMenu; el SubMenu de J2 usa Pad1 y el inventario del Sub0 exacto.

**Siguiente bloque:** navegación/opciones de PauseHD para owner J2, manteniendo una sola pausa global de sIDCockpit, y después lifetime/cierre del owner.

Por petición del propietario, no generar ni entregar nuevos EXE salvo solicitud expresa. No subir binarios propietarios. CI/mocks no equivalen a gameplay validado.
