# 44 — Activación del HUD publicado en el siguiente BEGIN

26 de septiembre de 2026. Continúa el coordinador estructural integrado. Solo fuentes/tests/evidencia; no genera ni modifica un EXE.

## Motivo de la separación

El coordinador crea/publica clones en END, dentro de StructuralWindow. ManagerDriver::attach no puede ejecutarse allí porque PipelineFrameProvider::sample exige PipelineClock abierto.

BeginActivator se ejecuta exclusivamente después de que el gateway BEGIN haya sido aceptado por PipelineClock. Comprueba site, owner y frame_base contra el PipelineStamp vigente y consume el ticket Live publicado por StructuralCoordinator.

## Reglas

- ticket 0: no hay HUD publicado; limpia estado local del activador.
- ticket igual al ya adjunto: no repite attach.
- ticket nuevo: llama una sola vez a ManagerDriver::attach(ticket).
- attach fallido: llama ManagerDriver::stop para revocar el HUD, pero no destruye nada; el siguiente END estructural hará collect.
- el ticket rechazado no se reintenta dentro del mismo ciclo/vida.
- cuando el coordinador retira/reemplaza ese ticket, el rechazo se limpia y un ticket posterior puede adjuntarse.
- END nunca llama al activador.
- un BEGIN rechazado por PipelineClock tampoco llama al activador.

## Gateway

pipeline_gateways.S llama rev_hud_begin_activation_event solo en la rama BEGIN y únicamente si rev_hud_pipeline_event devolvió éxito. Toda la llamada ocurre dentro del bloque de preservación GPR/EFLAGS/x87/MMX/XMM/MXCSR ya existente; después se reproducen las instrucciones originales.

## Pruebas

Run 36273213216:
- activator native: 8 escenarios / 59 aserciones, PASS;
- activator ASan/UBSan: 8 escenarios / 59 aserciones, PASS;
- gateway i386 real: 2 gateways / 3 invocaciones, PASS, con receptor/continuaciones sintéticos.

El harness verifica que solo el primer BEGIN aceptado llama al activador; END llama al estructural y un BEGIN cuyo pipeline callback devuelve false no activa nada.

No se ejecutó el motor del juego.

## Siguiente bloque

Con esto queda conectado el ciclo fuente completo de HUD:
END estructural crea/publica -> siguiente BEGIN adjunta -> manager phases actualizan/dibujan -> siguiente END puede retirar.

Quedan instalación efectiva y validación runtime, pero el siguiente análisis funcional debe salir del HUD: pausa/inventario por jugador y comandos scheduler/QTE restantes, seguido de muerte/checkpoints/cutscenes y escenas sin partner.
