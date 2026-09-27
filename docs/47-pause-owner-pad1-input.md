# 47 — PauseHD: navegación J2 por Pad 1

27 de septiembre de 2026. Continuación directa de `docs/46-pause-global-submenu-handoff.md`.

## Binario exacto

Análisis de solo lectura sobre:

```text
BioRevHD 30-Enero-2013
SHA-256 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69
60.748.800 bytes
```

No se genera ni modifica un EXE.

## Hallazgo

Las rutas reales de selección de PauseHD no son los índices 0/1/2 de `sGameFlags` descartados anteriormente.

Hay dos funciones de selección activas que obtienen `sPad` y leen dos campos mediante wrappers que seleccionan `mStartPadNo`:

```text
Ruta A
0x02C3183B -> 0x01B9B457 -> wrapper -> Pad[mStartPadNo]+0x1A0
0x02C31848 -> 0x01BD9F18 -> wrapper -> Pad[mStartPadNo]+0x1AC

Ruta B
0x02C31FDC -> 0x01B9B457 -> wrapper -> Pad[mStartPadNo]+0x1A0
0x02C31FE9 -> 0x01BD9F18 -> wrapper -> Pad[mStartPadNo]+0x1AC
```

Los getters directos son:

```text
0x01CB3140 -> Pad[index]+0x1A0
0x01CB31F0 -> Pad[index]+0x1AC
stride       0x2F8
mStartPadNo  sPad+0x970
```

## Consumo de input demostrado

`+0x1A0` participa en las decisiones/confirmación/cancelación de ambas rutas.

`+0x1AC` alimenta la navegación/repetición de selección. Entre los masks observados:

```text
Ruta A: 0x10010 y 0x40040
Ruta B: 0x50050
```

Esto separa el input real de PauseHD de la consulta de `sGameFlags` documentada antes.

## Implementación fuente

`MenuOwnerRouter` añade:

```text
pause_word_1a0()
pause_word_1ac()
```

Regla:

- `surface != Pause` -> comportamiento stock;
- `owner == 0` -> comportamiento stock / Pad seleccionado por el juego;
- `owner == 1` y Sub0 exacto sigue válido/Pad -> leer Pad 1;
- si Sub0 deja de ser válido -> fallback stock;
- no escribir `mStartPadNo`;
- no sustituir `Self`;
- no duplicar `PauseHD`;
- una sola pausa global sigue perteneciendo a `sIDCockpit`.

Los cuatro callsites se añaden al source map de `patches/menu_routing/menu_sites.json` y usan gateways con el ABI original `ret 4`.

## Verificación

Localmente:

- componente C++: 10 escenarios / 36 aserciones, PASS;
- ASan/UBSan: 10 escenarios / 36 aserciones, PASS;
- sintaxis freestanding i386: PASS;
- auditor de imagen exacta: 12 comprobaciones de bytes, PASS;
- el ejecutable de gateways i386 se enlaza localmente; su ejecución se deja a CI porque el runtime local no dispone de compatibilidad ELF i386.

No se ha ejecutado gameplay.

## Siguiente bloque

Cerrar lifetime del owner:

1. limpiar `owner/surface` al salir de state 5/8;
2. limpiar al invalidarse Sub0 o cambiar de sesión;
3. auditar states 6/7 y retornos/nested menus;
4. comprobar callers alternativos de las transiciones 5/8.

Después continuar con Genesis/acciones restantes, QTE/scripts, muerte/reanimación/checkpoints, cutscenes/cámaras forzadas y escenas sin partner.
