# 40 — Admisión concreta del JanuaryBackend

26 de septiembre de 2026. Continúa PR #13 / `e8954343a498665a959bbc1856edc41dadb45b33`. Solo fuentes/tests/evidencia. No se genera ni modifica un EXE.

## Problema cerrado

`JanuaryBackend` ya conocía las llamadas exactas de construcción, inicialización, fases y destrucción, pero su `JanuaryHost::permit` seguía siendo un contrato abstracto. Un callback que devolviese true permanentemente habría eliminado precisamente las garantías de ownership/fase que el resto del trabajo intenta preservar.

`JanuaryAdmission` materializa ese contrato sin inventar un punto seguro:

- `NativeOp::Phase` exige el `ManagerDriver::phase_scope` exacto del widget/fase/contexto y que `NativeViewScope` esté activo.
- Las escrituras internas de flags/time-scale exigen `phase_unit_scope`: misma unidad clon, manager y evento admitido; no basta estar dentro de cualquier fase.
- Las lecturas se permiten en un punto estructural externo demostrado o durante un evento+scope nativo admitido.
- `Structural/Initialize/Destroy` exigen hilo correcto, identidad de imagen, ausencia de evento de manager y scope de vista, y un callback externo `structural_safe`.
- `fresh_allocation` solo se consulta bajo ese mismo punto estructural.

No existe un default `structural_safe=true`; esa pieza permanece deliberadamente pendiente hasta identificar un límite real de motor.

## Adaptador real del driver

`DriverViewAdmission` une `ManagerDriver` y `NativeViewScope`. Se añade al driver `phase_unit_scope(kind,unit)`, más débil que `phase_scope` solo en fase/contexto, pero igual de estricto en ticket, hilo, manager y dirección exacta del clon. Se usa exclusivamente para las escrituras que JanuaryBackend realiza internamente durante un dispatch ya admitido.

No autoriza otra unidad, otro manager ni una escritura fuera del evento.

## Pruebas

La suite propia comprueba Structural/Initialize/Destroy, Phase/Write, identidad de imagen/hilo, lectura, fresh allocation y rechazos cruzados. La regresión de ManagerDriver comprueba `phase_unit_scope` desde su callback `checked_phase` real y rechaza otra dirección.

La CI de push del commit `61d9bd13d8a4b880dd741f3198e01662146ff080` terminó con success antes de abrir el PR. El PR repite todas las regresiones del repositorio.

## Siguiente punto

Cerrar `structural_safe` con evidencia del motor. No usar PipelineClock como GPU fence: identifica la invocación CPU, no trabajo gráfico drenado. La estrategia siguiente es localizar puntos de nacimiento/destrucción de los gestores antes/después de sus recursos y contrastarlos con begin/end del renderer, scheduling sUnit y cualquier wait/fence nativo antes de permitir Allocate/Initialize/Destroy.

También queda por cerrar el comando final de viewport/scissor del dispositivo. Nada de este cambio ejecuta gameplay.
