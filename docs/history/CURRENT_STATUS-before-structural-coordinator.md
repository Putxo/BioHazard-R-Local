# Estado actual — ventana estructural + ledger de asignaciones propias

26 de septiembre de 2026. Continúa desde PR #15 / `ec7f8314fdc0cf585efdd2b5bed3167faa3011f3`. Solo fuentes/tests/evidencia; no se ha generado otro EXE ni instalado hooks.

## Avance actual

La ventana estructural post-render-worker está integrada. Se añade `JanuaryAllocationLedger`: envuelve exclusivamente los tres allocators de Reticle/MainEquip/MapHerb, registra el retorno inmediato como Pending y permite una única transición Pending→Certified mediante fresh_allocation bajo la misma ventana estructural.

El constructor exacto solo se reenvía para un bloque Certified de la clase correcta. El destructor escalar exacto retira la identidad después de retornar. Dirección/tamaño distintos, segunda certificación, desalineación, solapamiento, allocator no esperado o cambio de ventana fallan cerrado.

`AllocationAdmissionBridge` conecta el gate estructural y el ledger con `AdmissionServices::structural_safe/fresh_allocation`, reenviando las comprobaciones existentes de memoria, hilo, imagen y escritura. No sustituye ninguna restricción previa por true.

## Render

NativeViewScope ya valida VIEW_1/BOTTOM y la igualdad `Viewport1.mRegion == cDraw+0xBC`. La cadena nativa demuestra que cDraw recibe ese mRegion antes de la GUI; no se aplica un factor 0.5 manual. El comando final de dispositivo/scissor sigue como evidencia adicional, no como requisito para mutar el scope actual.

## Punto exacto siguiente

Conectar un coordinador ejecutado dentro de `StructuralWindow::task` que haga prepare/publish/collect de Lifecycle y configure JanuaryBackend/Admission/Ledger en el orden correcto. Debe separar creación inicial, publicación y retirada, sin destruir durante callbacks de fase.

Después: pausa/inventario por jugador, scheduler compartido, muerte/checkpoints/cutscenes, escenas sin compañero, instalación efectiva y validación de campaña.

Estado anterior preservado en `docs/history/CURRENT_STATUS-before-allocation-ledger.md`.
