# Estado actual — ledger + coordinador estructural del HUD P2

26 de septiembre de 2026. Continúa desde el allocation ledger. Solo fuentes/tests/evidencia; no se ha generado otro EXE ni instalado hooks.

## Avance actual

`StructuralCoordinator` ejecuta dentro de `StructuralWindow::task` el mantenimiento estructural de los tres clones. Con captura válida de `LifetimeSource` y Lifecycle Empty valida los originales stock, exige ledger vacío, configura `JanuaryBackend` y ejecuta `prepare → publish`.

Originales verificados por ownership:
- Reticle = Cockpit+0x90;
- MainEquipWin = Cockpit+0x5C;
- MapBaseAndHerb = MiniMap+0x30, con alias +0x40 idéntico.

El ticket Live resultante queda almacenado para el siguiente ciclo.

Si desaparece/cambia la vida del partner o de los managers, el END actual solo ejecuta `ManagerDriver::stop → Lifecycle::collect`. No destruye y vuelve a crear en la misma tarea. Quarantine, collect incompleto o ledger retenido fallan cerrado.

## Separación END / BEGIN

`ManagerDriver::attach(ticket)` **no** se ejecuta durante END estructural. `PipelineFrameProvider::sample` exige `PipelineClock` abierto. El siguiente componente debe consumir el ticket publicado en el próximo BEGIN y adjuntar el driver una sola vez antes de las fases GUI.

## Pruebas

Run `36272849073`: native y ASan/UBSan PASS, 17 escenarios / 49 aserciones en cada modo; cuatro pruebas de contrato de fuentes PASS; sintaxis i386 PASS. Motor simulado, sin gameplay.

Detalle: `docs/43-structural-coordinator.md`.

## Estado previo

`StructuralWindow`, `JanuaryAdmission`, `NativeViewScope`, `JanuaryAllocationLedger`, la máscara P1 y el frame provider permanecen intactos. El estado anterior se conserva en `docs/history/CURRENT_STATUS-before-structural-coordinator.md`.

## Punto exacto siguiente

Implementar el activador del siguiente BEGIN: ticket Live pendiente → `ManagerDriver::attach(ticket)` dentro de un ciclo Pipeline abierto. Debe rechazar ticket reemplazado/revocado, evitar attach repetido y nunca fabricar un ManagerFrame fuera del ciclo.

Después siguen pausa/inventario por jugador, scheduler/QTE compartidos, muerte/checkpoints/cutscenes, escenas sin compañero, instalación efectiva y validación conjunta en campaña. El cooperativo local completo aún no está terminado.
