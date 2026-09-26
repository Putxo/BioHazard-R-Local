# 41 — Ventana estructural tras el render worker

26 de septiembre de 2026. Continúa PR #14 / `3f956e01010841735ce4888cb91e8cf79c0c9135`. Solo fuentes/tests/evidencia. No se genera/modifica un EXE.

## Sincronización nativa demostrada

En `sRender::begin 0x033BFD80`, cuando el render threaded path está activo:

- `0x033BFEC3..` hace `ResetEvent(sRender+0xD4)`;
- `0x033BFEDC..` hace `SetEvent(sRender+0xD0)`.

El worker `0x033C66B0` espera indefinidamente en `+0xD0`, resetea ese evento y, en sus rutas de finalización, hace `SetEvent(sRender+0xD4)`.

`sRender::end 0x033BFFC0` consulta/espera `+0xD4` en `0x033C0052` y `0x033C008C`. Solo después continúa a la llamada virtual de dispositivo en `0x033C00CD..`, que contiene la ruta de Present.

El ciclo exterior `sSkeletonMain::0x02F50680` llama a `sRender::end` en `0x02F50E7E` y todavía ejecuta callbacks post-render hasta `0x02F50F44`. El gateway END existente está en `0x02F50F4B`, después de ambos.

Conclusión limitada: al alcanzar ese endpoint el **render worker CPU del ciclo ha terminado** y los callbacks posteriores del ciclo exterior han retornado. No se afirma que la GPU esté idle ni que sea un fence D3D de finalización.

## StructuralWindow

El gateway END llama a `rev_hud_structural_event` solo si `rev_hud_pipeline_event` aceptó primero el END. Así la ventana no puede abrirse por un END huérfano o un ciclo en fault.

`StructuralWindow` exige hilo propietario, `PipelineClock::quiescent()`, un frame cerrado nuevo y una tarea registrada. `safe()` solo es verdadero mientras esa tarea se está ejecutando dentro del callback. Al retornar se cierra siempre.

Si la tarea abre otro ciclo de forma reentrante, cambia de hilo o falla, la ventana se invalida. No existe un booleano persistente entre frames.

Este scope es adecuado para operaciones CPU sobre los clones **desacoplados de las listas nativas**, pero no se eleva a prueba de GPU idle. Destruir recursos GPU que requieran un fence adicional seguirá necesitando evidencia específica.

## Pruebas

El componente prueba apertura/cierre válido, tarea fallida, cambio reentrante del PipelineClock, hilo incorrecto, END sin ciclo y múltiples ciclos. El harness i386 real del gateway comprueba que el callback estructural solo ocurre en END válido y que el replay de instrucciones conserva estado.

El auditor hash-pinned verifica 12 ventanas del original: Reset/Set de eventos, worker wait/reset/completion, wait de sRender::end, Present posterior y posición del endpoint exterior.

## Siguiente punto

Usar esta ventana como implementación concreta de `AdmissionServices::structural_safe` y cerrar `fresh_allocation` con un ledger que solo acepte bloques retornados por nuestras propias llamadas al allocator durante la ventana. Después conectar el orquestador Lifecycle prepare/publish/collect en ese mismo punto.

La ventana no instala hooks y no ejecuta gameplay.
