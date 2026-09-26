# 39 — Viewport nativo → cDraw → GUI: scope de P2 sin escalado artificial

26 de septiembre de 2026. Continúa PR #12 / `627c352689946cb4b43adc3f35b0f545228ea5b8`. Solo fuentes, tests y evidencia. No se genera/modifica un EXE ni se instalan hooks.

## Rectificación

`0x02DC16F0` no es el setter del índice de vista del contexto GUI. Sus offsets `+0x154/+0x158` pertenecen a otra clase y almacenan callback/usuario. Se descarta esa asociación.

El setter real del índice de vista de `cDraw` es `0x0328CE80`: sustituye únicamente el byte bajo de `cDraw+0x158`. El loop normal de `sCamera`, en `0x0328A980`, recorre ocho Viewport de tamaño `0x190` y en `0x0328AA8A..0x0328AA91` pasa el índice `i` a ese setter antes de ejecutar el Viewport.

## Cadena completa de región

La metadata/código de Viewport sitúan `mMode` en `+0x13` y `mRegion` como rectángulo de 16 bytes en `+0x18`. `0x03289C00` reconstruye mRegion según el modo.

En la ruta normal:

1. `0x032891FB -> 0x03289C00` recalcula `mRegion`.
2. `0x03289203 -> 0x02494B10` copia los 16 bytes de `Viewport+0x18` a un local.
3. Ese mismo local se pasa como tercer argumento al método de `cDraw` en `0x03289469..0x0328947B`.
4. Dentro de `0x03709720`, `0x0370979E..0x037097A5` pasa ese tercer argumento a `0x037083B0`.
5. `0x037083B0` instala el rectángulo en `cDraw+0xBC`, mantiene una copia en `+0xCC` y recalcula sus escalas.

La GUI común obtiene esos 16 bytes desde `cDraw+0xBC`. Por tanto un factor 0.5 añadido manualmente a Reticle/Equip/Herb duplicaría una transformación que el motor ya prepara por Viewport.

## Split local

La fuente histórica `camera_persistence_v8.S`, reutilizada por v9 y descendientes, configura Self como VIEW_0/TOP y Partner como VIEW_1/BOTTOM; Viewport 1 queda visible, modo 3 y display 0.

El singleton de `sCamera` está en `0x05799D3C`: el constructor publica `this` y el destructor lo limpia. Su vtable es `0x04EC9AA8`. `cDraw` instala vtable `0x04F79014`.

## NativeViewScope

`NativeViewScope` no modifica estado gráfico. Para fase11 exige sesión Sub0 vigente, contexto cDraw exacto, view index 1, sCamera/Viewport1 vivo en BOTTOM, display 0 y coincidencia DWORD a DWORD entre `Viewport1.mRegion` y `cDraw+0xBC`. El rectángulo debe ser no vacío.

Al salir vuelve a comprobar hilo, contexto, cámara, tracker y rectángulo. Un cambio durante el draw produce rechazo. Para fases 8/9 no hay contexto de dibujo y solo se empareja entrada/salida.

`services()` produce directamente los `PipelineServices` requeridos por `PipelineFrameProvider`; no hay callback de scope que devuelva true sin evidencia.

## Límite

Este módulo demuestra que la transformación de área activa puede heredarse del camino nativo. Aún no concede `NativeOp::Structural/Destroy`, no instala hooks y no demuestra GPU/render drenado. El siguiente paso es conectar la admisión de JanuaryBackend y seguir el comando final de scissor/device state para confirmar que no se necesita una segunda preparación.

No se ejecutó gameplay.
