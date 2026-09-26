# Estado actual — ventana estructural CPU después del render worker

26 de septiembre de 2026. Continúa PR #14 / `3f956e01010841735ce4888cb91e8cf79c0c9135`. Solo fuentes, tests y evidencia.

## Avance actual

La admisión exacta del JanuaryBackend está integrada. Se añade ahora `StructuralWindow` en el endpoint END del ciclo exterior de sSkeletonMain.

El camino nativo demuestra: sRender::begin resetea el evento de finalización (+D4) y despierta el worker (+D0); el worker espera +D0 y señaliza +D4 al terminar; sRender::end espera/pollear +D4 antes de la llamada de Present; el endpoint 0x02F50F4B ocurre después de sRender::end y de los callbacks post-render restantes.

`StructuralWindow::safe()` solo es true dentro de la tarea ejecutada desde ese END válido, con PipelineClock ya cerrado y en el hilo propietario. No queda habilitada entre ciclos. Cambio reentrante, hilo distinto o fallo de tarea la revocan.

Esto prueba render-worker CPU drenado, **no GPU idle/fence**.

## Conexión pendiente inmediata

Conectar `StructuralWindow::safe()` a `AdmissionServices::structural_safe` y certificar `fresh_allocation` mediante un ledger de las llamadas de allocator que nosotros mismos iniciamos. Los clones siguen detached y no se insertan en listas nativas.

Después conectar un coordinador en la propia tarea estructural para prepare/publish/collect de Lifecycle.

## Render

NativeViewScope ya demuestra VIEW_1/BOTTOM y mRegion -> cDraw+BC. El comando final de device viewport/scissor sigue en análisis; no se añade escalado 0.5 ni se escriben rectángulos.

Pausa/inventario, scheduler, muerte/checkpoints/cutscenes, escenas sin compañero, instalación y gameplay siguen abiertos.
