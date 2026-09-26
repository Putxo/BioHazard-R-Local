# 43 — Coordinador estructural de creación, publicación y retirada

26 de septiembre de 2026. Continúa el ledger de asignaciones propias. Solo fuentes/tests/evidencia; no genera ni modifica un EXE.

## Separación de END estructural y BEGIN activo

La ventana estructural de `StructuralWindow` ocurre en END, después del render worker CPU. Ahí se permiten las operaciones estructurales sobre clones detached: configure, allocate/certify, construct, initialize, publish y collect.

`ManagerDriver::attach()` **no** se ejecuta aquí. `PipelineFrameProvider::sample` requiere un `PipelineClock` abierto, por lo que el attach se difiere al siguiente ciclo BEGIN. El coordinador publica un ticket Live y lo conserva para ese activador posterior.

## Creación inicial

Dentro de `StructuralWindow::task`:

1. `LifetimeSource::capture` produce Session y generaciones de Cockpit/MiniMap.
2. Se validan los originales prestados por slots exactos: Reticle cockpit+0x90, MainEquipWin cockpit+0x5C y MapBaseAndHerb minimap+0x30; minimap+0x40 debe ser el mismo mapa.
3. El ledger debe estar vacío.
4. `JanuaryBackend::configure` fija padres/originales.
5. `Lifecycle::prepare` ejecuta allocate → fresh_allocation → construct → initialize de los tres clones.
6. `Lifecycle::publish` cambia Prepared → Live.
7. El ticket resultante queda pendiente para el siguiente ciclo.

No se insertan clones en listas nativas.

## Cambio de vida y retirada

Si desaparece la captura o cambia epoch, actor o generación de manager mientras Lifecycle está Live, el mismo END solo revoca y recoge: `ManagerDriver::stop` seguido de `Lifecycle::collect`.

No se recrea una sesión nueva dentro de esa misma tarea. La siguiente creación se permite en un END posterior. Esto separa destrucción y nueva asignación incluso si LifetimeSource ya ve el reemplazo.

Quarantine, ledger retenido después de collect o destructor rechazado son fallos cerrados. No se reutiliza una dirección dudosa.

## Pruebas

Run de push `36272849073`:
- native: 17 escenarios / 49 aserciones, PASS;
- ASan/UBSan: 17 escenarios / 49 aserciones, PASS;
- cuatro pruebas Python de contrato de fuentes, PASS;
- sintaxis freestanding i386, PASS.

El primer run falló únicamente al enlazar el workflow porque faltaba `pipeline_clock.cpp`, dependencia de `structural_window.cpp`; se corrigió y el segundo run pasó. Las llamadas de motor en la suite son simuladas; no se ejecutó gameplay.

## Siguiente bloque exacto

Implementar el activador del siguiente BEGIN: consumir el ticket Live publicado por el coordinador y ejecutar una sola vez `ManagerDriver::attach(ticket)` mientras `PipelineClock` está abierto y antes de las fases GUI.

El activador debe distinguir “ticket todavía no adjunto”, “ya adjunto” y “ticket reemplazado/revocado”; no debe intentar attach durante END ni fingir un ManagerFrame fuera del ciclo.
