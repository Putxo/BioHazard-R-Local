# Estado actual — pausa global separada del ownership de inventario/submenu

27 de septiembre de 2026. Continúa el estado consolidado previo. Solo fuentes/tests/evidencia; no se ha generado otro EXE ni instalado hooks.

## Hallazgo actual

La pausa global pertenece a una única state machine `sIDCockpit`. Esta instancia posee Cockpit, MiniMap, GameOver, PauseHD, PauseBlur y uSubMenuManager. No se duplicará PauseHD para P2.

Estado 5 activa PauseHD y estado 8 activa uSubMenuManager. `uGUI_InGameFile+0x2B8` guarda/restaura el estado anterior; no es el opener real del submenu.

`uGUI_PauseHD` no decide directamente el pad físico en las rutas auditadas: su virtual 8 consulta sGameFlags y su update resuelve Self para reflejar estado del actor.

`uGUI_SubMenu` tampoco contiene ownership de jugador. Dos rutas distintas están hardcodeadas a Self -> cBioItemPack:
- 0x02B6DE91 / 0x02B6DEA7
- 0x02B6EFDE / 0x02B6EFF4

Duplicar stock mostraría P1 dos veces. No se tocará Self global.

## Punto exacto siguiente

Seguir el wrapper público de sIDCockpit::setState y localizar el caller que solicita estado 8. Identificar la fuente real del input/evento y si transporta selector de miembro. Ese punto será el owner externo de inventario/submenu P1/Sub0.

Después sustituir únicamente las resoluciones Self->pack de uGUI_SubMenu por el owner seleccionado y mantener la pausa global separada.

Detalle completo: docs/45-pause-inventory-routing.md.

Siguen abiertos scheduler/QTE restantes, muerte/reanimación/checkpoints, cutscenes/cámaras forzadas, escenas sin compañero, instalación y validación conjunta en campaña. El cooperativo completo no está terminado.
