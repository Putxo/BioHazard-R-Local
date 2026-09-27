# 48 — Lifetime del owner de pausa/submenu

27 de septiembre de 2026. Continúa PR #21 y `docs/47-pause-owner-pad1-input.md`.

## Hallazgo central

El thunk `0x01C365BF` salta a `0x02CDD180`, el transition body de `sIDCockpit`. La función termina escribiendo el nuevo state en `sIDCockpit+0x24`.

El propio motor trata `5/6/7/8` como una familia pausada en el bloque de transición. Además:

- state 5: PauseHD;
- state 8: uSubMenuManager;
- state 6: usado por `uGUI_InGameFile`; guarda el state anterior en `+0x2B8`, entra en 6 y al cerrar restaura exactamente el state guardado;
- state 7: estado pausado/nested agrupado por la misma transición; no se fuerza una identidad UI no demostrada.

La ruta de episodio cierra tanto state 5 como state 8 entrando en state 1.

## Política de lifetime

`MenuOwnerRouter` conserva la superficie raíz que abrió el jugador:

- Pause owner -> state 5;
- SubMenu owner -> state 8;
- state 6/7 -> preservar temporalmente el owner para permitir nested UI y retorno;
- volver al state raíz 5/8 correcto -> preservar;
- pasar a gameplay/otro state o cruzar Pause<->SubMenu sin opener -> limpiar owner/surface.

J2 queda además ligado a la identidad exacta con la que abrió:

```text
Sub0 pointer + actor serial
```

Si cambia el puntero, cambia el serial, deja de estar activo local o deja de estar en ThinkMode::Pad, el owner se limpia en vez de transferirse al nuevo partner.

## Hook fuente

`0x01C365BF` es un thunk de cinco bytes:

```text
E9 BC 6B 0A 01 -> 0x02CDD180
```

El gateway observa `new_state`, restaura registros/ABI y tail-jump al body stock. No sustituye la state machine ni duplica PauseHD.

## Evidencia exacta

`scripts/audit_menu_owner_lifetime.py` fija ocho ventanas del ejecutable January SHA-256 canónico, incluyendo thunk central, familia 5-8, commit de state, InGameFile save/restore y cierres 5/8 -> 1.

No se ha ejecutado gameplay y no se genera ningún EXE.

## Siguiente bloque

Con el owner de menú ya acotado por input + identidad + state lifetime, el siguiente trabajo funcional es la pasada de Genesis/acciones específicas restante, seguida de scheduler/QTE/scripts, muerte/reanimación/checkpoints, cutscenes/cámaras forzadas y escenas sin partner.
