# Estado actual — HUD estructural publicado y adjuntado en el siguiente BEGIN

26 de septiembre de 2026. Solo fuentes/tests/evidencia; no se ha generado otro EXE ni instalado hooks.

## Flujo conectado

El mantenimiento estructural del HUD queda separado por fase del motor:

1. END estructural: LifetimeSource captura vida; StructuralCoordinator valida originales, configura JanuaryBackend y ejecuta prepare/publish, o stop/collect si la vida cambió.
2. El ticket Live publicado queda pendiente.
3. Siguiente BEGIN aceptado por PipelineClock: BeginActivator valida owner/frame_base y ejecuta una sola vez ManagerDriver::attach(ticket).
4. Las fases de Cockpit/MiniMap usan ManagerDriver + NativeViewScope + Lifecycle para los clones P2.
5. Un attach fallido solo revoca; la destrucción espera al siguiente END estructural.

No se hace attach durante END ni allocate/destroy durante callbacks GUI.

## Pruebas del activador

Run 36273213216:
- native: 8 escenarios / 59 aserciones, PASS;
- ASan/UBSan: 8 escenarios / 59 aserciones, PASS;
- gateways i386: 2 gateways / 3 invocaciones, PASS.

El gateway BEGIN solo llama al activador después de un PipelineClock aceptado. END no lo llama y un BEGIN rechazado tampoco.

Detalle: docs/44-next-begin-hud-activation.md.

## Lo que el render ya hereda

NativeViewScope valida VIEW_1/BOTTOM y Viewport.mRegion → cDraw+0xBC; no se aplica un 0.5 manual. La máscara stock de P1 evita duplicar los tres originales con clon P2 en view1. StructuralWindow prueba render-worker CPU drenado, no GPU idle.

## Punto exacto siguiente

El cableado fuente del HUD queda conectado de END a BEGIN y a las fases GUI. Falta instalación efectiva/runtime, pero el siguiente análisis funcional se mueve a los sistemas de gameplay aún compartidos:

- pausa e inventario por jugador;
- comandos scheduler/QTE restantes;
- muerte/reanimación/checkpoints;
- cutscenes/cámaras forzadas;
- escenas sin compañero/creación de P2;
- instalación y validación conjunta en campaña.

Estado anterior preservado en docs/history/CURRENT_STATUS-before-begin-activator.md.
