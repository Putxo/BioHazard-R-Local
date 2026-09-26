# Estado actual — JanuaryBackend conectado a admisión real de fase

26 de septiembre de 2026. Continúa PR #13 / `e8954343a498665a959bbc1856edc41dadb45b33`. Solo fuentes, tests y evidencia; no se ha generado otro EXE ni instalado hooks.

## Avance actual

`JanuaryAdmission` implementa el host que JanuaryBackend necesitaba. `NativeOp::Phase` solo pasa si el ManagerDriver está en el widget/fase/contexto exactos y el NativeViewScope está activo. Las escrituras internas requieren la unidad exacta admitida mediante `phase_unit_scope`.

Lectura, identidad de imagen e hilo se validan en cada llamada. Creación/inicialización/destrucción NO se autorizan por estar en una fase: requieren un callback externo `structural_safe`, fuera de eventos y scopes de render. Fresh allocation hereda ese mismo requisito.

No se ha rellenado `structural_safe` con true ni con el contador del pipeline. PipelineClock sigue siendo identidad temporal CPU, no fence de GPU.

## Piezas conectadas

La cadena de fuentes queda:
`LifetimeSource + PipelineClock + NativeViewScope -> PipelineFrameProvider -> ManagerDriver -> Lifecycle -> JanuaryBackend`, con `JanuaryAdmission` como política del backend.

El split nativo sigue usando VIEW_1/BOTTOM y el HUD P1 original se enmascara solo durante view1 para las tres clases clonadas.

## Pruebas

CI específica de JanuaryAdmission en verde en el head de código. La suite de ManagerDriver verifica también que `phase_unit_scope` acepta solo la unidad clon actualmente admitida. El PR de integración vuelve a ejecutar las regresiones completas.

## Punto exacto siguiente

Demostrar un punto estructural real para Allocate/Initialize/Destroy y una certificación real de la asignación. Examinar manager birth/death, sUnit scheduling y render begin/end/fences; no confundir “fuera del callback HUD” con “GPU drenada”.

En paralelo, terminar la ruta final de viewport/scissor del dispositivo para confirmar que NativeViewScope puede permanecer no mutante.

Pausa/inventario por jugador, scheduler, muerte/checkpoints/cutscenes, escenas sin compañero, instalación efectiva y validación de campaña siguen pendientes.
