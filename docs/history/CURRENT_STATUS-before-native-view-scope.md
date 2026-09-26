# Estado actual — ciclo/vida del HUD + máscara P1 scoped por vista

26 de septiembre de 2026. Continúa desde PR #11 / `3d624e39aec66063e67208107e587abffdd3062f`. Solo fuentes, pruebas y evidencia en GitHub. No se ha generado/modificado otro EXE ni instalado hooks.

## Cambio actual

Se añade `P1ViewMask`: cuando el despacho manual de P2 en fase11/vista1 termina correctamente, elimina temporalmente el bit de vista1 únicamente de los tres originales que ya tienen clon P2: MainEquipWin (cockpit+5C), Reticle (cockpit+90) y MapBaseAndHerb (minimap+30, alias +40). Los demás widgets stock permanecen intactos.

El scope se restaura al terminar el bucle stock de cada manager, recuperando solo los diez bits mDrawView y preservando otros cambios de cUnit. Una identidad de slot/vtable cambiada o un fallo de escritura provoca revocación del ManagerDriver, no una liberación durante el draw.

Los gateways siguen sin instalarse. Este cambio no modifica coordenadas ni añade un escalado 0.5: la ruta común de GUI ya calcula ajustes de resolución y sigue pendiente demostrar el significado exacto del contexto+BC/viewport/scissor.

## Evidencia y continuación

Auditor local: 16 comprobaciones exactas del original, hash intacto. Detalle: docs/38-p1-scoped-view-mask.md y research/reports/hud-p1-view-mask-validation.json.

Punto siguiente: seguir el productor del contexto de dibujo que contiene +158 (índice de vista) y +BC (estructura de dimensiones), y sus consumidores de proyección/scissor. Después implementar el scope nativo de render y validar que TOP/BOTTOM ya transforman el HUD antes de cualquier ajuste adicional.

Siguen pendientes pausa/inventario por jugador, scheduler compartido, muerte/checkpoints/cutscenes, escenas sin compañero, instalación real y validación conjunta en campaña. El cooperativo local completo no está terminado.

## Estado anterior preservado

El estado de PR #11 queda en docs/35–37 y en el historial Git; no se rehacen PipelineClock, LifetimeSource, Lifecycle, JanuaryBackend ni ManagerDriver.
