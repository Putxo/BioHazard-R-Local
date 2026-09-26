# 42 — Ledger de asignaciones propias para fresh_allocation

26 de septiembre de 2026. Continúa PR #15 integrado en `ec7f8314fdc0cf585efdd2b5bed3167faa3011f3`. Solo fuentes/tests/evidencia; no se genera ni modifica un EXE.

## Problema

`JanuaryBackend::create` exige que el puntero recién devuelto por el allocator sea una asignación nuestra y exclusiva antes de ejecutar el constructor. Comprobar después que el puntero “parece válido” no demuestra procedencia ni evita reutilizar una dirección antigua.

## Evidencia de los tres allocators

Los thunks usados por JanuaryType son:

- Reticle `0x01C11706 -> 0x02B3FDD0`;
- MainEquipWin `0x01C15775 -> 0x02B3AD10`;
- MapBaseAndHerb `0x01C7855A -> 0x02B61520`.

Las tres implementaciones siguen el mismo patrón: obtienen el allocator nativo de la clase, obtienen su DTI, empujan alineación y tamaño recibidos, llaman al virtual `+0x1C` del allocator y devuelven directamente el EAX resultante. El auditor hash-pinned fija los tres thunks y las tres ventanas de llamada.

## JanuaryAllocationLedger

El backend recibe `ledger.callbacks()` en lugar de la tabla nativa cruda.

Solo acepta los tres pares exactos allocator/tamaño y alineación 0x10. El retorno se registra como `Pending`. No puede emitirse una segunda asignación mientras exista otro Pending. `fresh_allocation(address,size)` exige ventana estructural vigente y consume exactamente ese Pending una sola vez, pasando a `Certified`.

El constructor se reenvía solo cuando dirección, clase y constructor coinciden con un registro Certified. Un puntero distinto, tamaño distinto, segunda certificación, retorno desalineado, solapamiento con otro bloque propio o cambio del scope estructural bloquean el ledger.

El wrapper de `method1` reconoce únicamente los tres destructores escalares exactos con argumento 1 para retirar el registro después de que la llamada nativa retorne. Un destructor solicitado sobre memoria no certificada no se ejecuta: se prefiere fuga/cuarentena a liberar una dirección cuya procedencia ya no está demostrada.

## Conexión con JanuaryAdmission

`AllocationAdmissionBridge` reenvía Reader/thread/image/write existentes y reemplaza `structural_safe` por la conjunción del gate estructural demostrado y cualquier restricción previa. `fresh_allocation` primero conserva cualquier certificador previo opcional y después consume el Pending del ledger.

`structural_allocation_gate(window)` enlaza directamente con `StructuralWindow::safe()`; no usa PipelineClock abierto, callbacks GUI ni constantes true.

## Límite

El ledger acredita procedencia y consumo único de las asignaciones que nosotros solicitamos; no convierte la ventana CPU post-render en GPU idle. Los clones continúan detached de las listas nativas. Un bloque que queda en estado incierto se conserva/rechaza en vez de reutilizarse.

Siguiente paso: conectar el coordinador de `Lifecycle::prepare/publish/collect` dentro de la tarea de StructuralWindow usando el ledger+admission, y verificar el orden create→init→publish y stop→collect→destroy. Después seguir pausa/inventario y flujos de escena.
