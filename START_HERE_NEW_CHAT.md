# EMPEZAR AQUÍ — cooperativo local actual

Rama técnica canónica:

```text
research/script-member-serial-validation
```

No usar `main` como estado técnico de implementación.

Leer en este orden:

1. [CURRENT_STATUS.md](CURRENT_STATUS.md)
2. [docs/45-local-menu-owner-routing.md](docs/45-local-menu-owner-routing.md)
3. [docs/46-pause-global-submenu-handoff.md](docs/46-pause-global-submenu-handoff.md)
4. [research/current_state.json](research/current_state.json)
5. [AGENTS.md](AGENTS.md)

Checkpoint integrado antes de este handoff:

```text
PR #19
merge b0900736d9e44a04a47f9804b75059f007dd12fd
15 workflows PASS
```

## Continuación exacta

`MenuOwnerRouter` ya distingue J1/J2 al abrir Pause/SubMenu. SubMenu owner=J2 ya usa Pad1 y los datos de Sub0.

**Ahora continuar por Pause owner=1:** localizar la navegación real de PauseHD/opciones y enrutar confirmar/cancelar/navegación a Pad1 solo cuando owner=J2. Mantener una sola pausa global de sIDCockpit.

Después cerrar owner/surface al salir de states 5/8 y auditar states 6/7/callers alternativos.

No confundir los índices de sGameFlags de PauseHD con pads. No volver a pickup, HUD base, puertas generales, v12/v13/v14 ni 0x049A8023.

No generar EXE salvo petición expresa.
