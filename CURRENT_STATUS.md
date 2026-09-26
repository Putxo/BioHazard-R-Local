# Estado actual — viewport nativo validado para el HUD P2

26 de septiembre de 2026. Continúa PR #12 / `627c352689946cb4b43adc3f35b0f545228ea5b8`. Solo fuentes, pruebas y evidencia en GitHub. No se ha generado/modificado otro EXE ni instalado hooks.

## Avance

Se demuestra la cadena normal `sCamera::Viewport.mRegion -> cDraw+0xBC -> GUI`. El loop de sCamera escribe además el índice de Viewport en el byte bajo de `cDraw+0x158`. El split local existente configura Partner como VIEW_1/BOTTOM.

`NativeViewScope` valida el contexto ya preparado por el motor en lugar de aplicar un escalado manual: cDraw exacto, VIEW_1, sCamera/Viewport1 visible en BOTTOM, display 0, tracker Sub0 y rectángulo mRegion idéntico a cDraw+BC. Al salir vuelve a verificar esos anclajes. No escribe viewport, proyección, scissor ni coordenadas.

`NativeViewScope::services()` llena los PipelineServices que estaban pendientes en PipelineFrameProvider. Las fases 8/9 se emparejan sin contexto de dibujo; fase11 exige la ruta nativa P2 completa.

## Rectificación

`0x02DC16F0` no pertenece al cDraw usado por los managers; se elimina como supuesto setter de view index. El setter real es `0x0328CE80` y se alimenta desde el loop normal de sCamera.

## Evidencia

Auditor hash-pinned del original: 15 ventanas/relaciones concretas, SHA intacto. Detalle en `docs/39-native-viewport-region-to-cdraw.md` y `research/reports/hud-native-view-scope-validation.json`.

## Siguiente punto

Conectar este scope con el host de admisión de JanuaryBackend: `NativeOp::Phase` debe exigir `ManagerDriver::phase_scope` más este scope nativo; `Structural/Initialize/Destroy` continúan necesitando un punto seguro real y vida exclusiva de asignaciones. En paralelo, seguir el scissor/device-state final para confirmar que no existe una segunda preparación necesaria.

Pausa/inventario por jugador, scheduler, muerte/checkpoints/cutscenes, escenas sin compañero, instalación real y validación conjunta siguen abiertos. El cooperativo local completo no está terminado.
