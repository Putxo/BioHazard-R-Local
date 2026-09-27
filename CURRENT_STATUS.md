# Estado actual — cooperativo local, menú J2 y handoff canónico

> Avance del 27/09/2026 en `research/pause-owner-lifetime-audit`, basado en
> `research/pause-owner-pad1` hasta `23109d5`: cuatro entradas PauseHD Pad1
> preservadas; añadido cierre por estado, identidad actor+serial y reset de
> sesión en fuente. Win32: 42 escenarios/161 aserciones PASS; auditor exacto:
> 19 comprobaciones PASS. Detalle: [docs/48-pause-owner-lifetime.md](docs/48-pause-owner-lifetime.md).
> No está instalado en el juego. Pendientes: nested 6/7, opciones, conexión
> del reset al teardown real y campaña. El checkpoint histórico siguiente
> describe la rama canónica antes de integrar este avance.

27 de septiembre de 2026. Rama técnica canónica: `research/script-member-serial-validation`.

Último bloque integrado: **PR #19**, merge `b0900736d9e44a04a47f9804b75059f007dd12fd`, con **15 workflows PASS**. Entrega vigente: **fuentes, tests, evidencia y documentación en GitHub**. No generar ni entregar EXE salvo petición expresa del propietario.

El cooperativo local completo todavía **no está validado jugando una campaña** y la instalación runtime conjunta sigue pendiente.

## Estado por función solicitada

| Función | Estado |
|---|---|
| Mando físico J2 independiente | Routing Pad 1 / PadData[1] implementado en fuente |
| J2 sin IA | Sub0 Cpu -> Pad implementado; Network preservado |
| Movimiento J2 | Cubierto por routing general; validación campaña pendiente |
| Apuntar/disparar/recargar/combate | Routing general + rutas específicas; auditoría final pendiente |
| Cambiar arma/equipamiento | Rutas Sub0/Pad1 confirmadas |
| Genesis/acciones específicas | Pendiente pasada completa |
| Pantalla partida | VIEW_0/VIEW_1 + TOP/BOTTOM implementado en fuente |
| Cámaras J1/J2 | Infraestructura independiente implementada; forzadas/cutscenes pendientes |
| HUD propio J2 | Clones, ownership, lifecycle, NativeViewScope y máscara P1 implementados |
| Herb/equipo/iconos J2 | Routing por actor implementado en HUD trabajado |
| Inventario gameplay J2 | cBioItemPack propio confirmado |
| Menú inventario J2 | **PR #19 integrado: owner + Pad1 + Sub0** |
| Pickup J2 | Entrega al pack propio trabajada |
| Munición Coop | Regla local trabajada sin cambiar GameMode global |
| Puertas/interacciones 2P | Bloqueos conocidos auditados/ruteados |
| PcsSub/QTE/scripts | Guards owner+serial + ActionCommands parciales |
| Pausa iniciada por J2 | Owner se registra en PR #19 |
| Navegación/opciones de pausa J2 | **Pendiente actual** |
| Muerte/reanimación | Rescue parcial; cobertura completa pendiente |
| Checkpoints | Pendiente |
| Cutscenes/cámaras forzadas | Pendiente |
| Escenas sin partner | Pendiente creación/retirada P2 |
| Campaña completa | No validada |

## PR #19 — MenuOwnerRouter

Código: `patches/menu_routing/`.

Sitios canónicos:

```text
0x01F3EF43  Pause open bit 0x8
0x01F3EF92  SubMenu open bit 0x1
0x02B6A491  SubMenu input +0x198
0x02B6A49E  SubMenu input +0x1A0
0x02B6DE91  SubMenu Self finder #1
0x02B6EFDE  SubMenu Self finder #2
```

Reglas:
- P1 tiene prioridad si ambos abren a la vez.
- J2 solo vale con Sub0 exacto vivo, serial 0..127 y ThinkMode::Pad.
- SubMenu owner=1 usa Pad 1.
- Sus dos resoluciones Self se sustituyen solo por el Sub0 exacto.
- No se modifica Self global ni mStartPadNo global.

Detalle: `docs/45-local-menu-owner-routing.md`.

## Arquitectura global de pausa/submenu

`sIDCockpit` es único y posee:

```text
+0x28 uCockpitManagerMain
+0x2C uMiniMapManager
+0x30 uGUI_GameOverHD
+0x34 uGUI_PauseHD
+0x38 uGUI_PauseBlurHD
+0x3C uSubMenuManager
```

State machine `0x02CDD180`:
- state 2: Cockpit/MiniMap
- state 3: GameOver
- state 5: PauseHD
- state 8: uSubMenuManager

Las reservas globales START/END de pausa viven en `+0x40/+0x41`. **Debe existir una sola pausa global.**

`uGUI_PauseHD::update 0x02C309C0` resuelve Self, pero el virtual 8 auditado consulta `sGameFlags`; sus índices 0/1/2 **no son PadData**.

`uSubMenuManager` contiene:
```text
+0x70 uGUI_SubMenuBase
+0x74 uGUI_SubMenu
+0x78 uGUI_WorldMapIcon
+0x7C uWorldMapModel
```

El stock uGUI_SubMenu no tiene owner/member reflejado y usa Self->cBioItemPack en las dos rutas que PR #19 ya intercepta.

Más detalle: `docs/46-pause-global-submenu-handoff.md`.

## HUD ya conectado en fuente

No rehacer HUD desde cero. La línea actual ya conecta:

```text
LifetimeSource
 -> StructuralWindow
 -> AllocationLedger / JanuaryAdmission
 -> StructuralCoordinator
 -> Lifecycle prepare/publish
 -> siguiente Pipeline BEGIN
 -> BeginActivator
 -> ManagerDriver
 -> NativeViewScope
 -> clones P2
```

NativeViewScope valida VIEW_1/BOTTOM y la ruta nativa de viewport/cDraw. La máscara P1 evita duplicar los tres originales clonados en view1.

## PUNTO EXACTO DEL SIGUIENTE CHAT

1. **Terminar Pause owner=1**
   - localizar la ruta física de navegación/confirmar/cancelar/opciones mientras `sIDCockpit state=5`;
   - owner 0 -> Pad0, owner 1 -> Pad1;
   - no cambiar `mStartPadNo` global;
   - no parchear los índices de `sGameFlags` de PauseHD como si fueran pads.

2. **Cerrar lifetime del owner**
   - limpiar owner/surface al salir de state 5/8;
   - limpiar si Sub0 deja de ser válido o cambia la sesión;
   - auditar states 6/7 y retornos/nested menus;
   - no dejar owner=1 persistente.

3. **Auditar callers alternativos de state 5/8**
   - PR #19 cubre sus seis sitios canónicos;
   - la state machine tiene otras transiciones; comprobarlas antes de declarar cobertura total.

4. Después: Genesis/acciones restantes -> scheduler/QTE/scripts -> muerte/revive/checkpoints -> cutscenes/cámaras forzadas -> escenas sin partner -> instalación conjunta y prueba de campaña.

## Reglas

- No generar EXE/ZIP binarios sin petición expresa.
- No subir EXE/DLL/PDB/assets/.obj.
- No reemplazar Self, serial, GameMode, mStartPadNo o Network globalmente.
- No llamar gameplay validado a CI/mocks.
- Preflight remoto y preservar trabajo paralelo.
