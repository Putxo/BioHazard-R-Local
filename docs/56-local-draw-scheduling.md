# Dibujo de vistas locales en el hilo propietario

27/09/2026. Continúa PR28. No se ha ejecutado el juego.

El HUD y ActionIcon necesitan el hilo propietario del ciclo CPU. `sUnit::draw`
(`0x03268560`) dispone de dos recorridos nativos: invocación inmediata de slots
11/15 y envío de tareas. El segundo podía llevar el draw a un hilo donde los
adaptadores locales lo rechazan por falta de su contexto de frame/lifetime.

El nuevo bridge reemplaza **solo** la llamada `0x0326862E -> 0x01BCD42A`.
Primero obtiene el resultado stock. Para un contexto exacto `0x04F79014`, View0
o View1, frame abierto y dos actores Pad vivos en sesión local, selecciona la
ruta inmediata ya existente. La prueba del hilo se realiza mediante el snapshot
del Runtime antes de admitir el modo local. Contextos auxiliares, sesiones
incompletas, hilos distintos y modo no local conservan la decisión stock.

La ruta inmediata llama al setup nativo con `(context,0,0,true)`, dibuja las
unidades filtradas y llama al cierre nativo con `(context,0,0)`. Converge con la
otra ruta en `0x032689BD`. Se conservan sus filtros de actividad, modo y vista.
No se reescribe el selector global `0x057A729C`, el número de workers, los
contenedores de tareas ni los recursos del renderer. No se presupone que una
tarea gráfica pendiente haya terminado por tener un frame CPU válido.

Esto serializa el recorrido de unidades para las vistas locales admitidas.
Puede reducir el paralelismo y su rendimiento no se ha medido. Tampoco da
soporte genérico a llamadas de HUD arbitrarias desde workers: evita ese envío
en el recorrido auditado, mientras las comprobaciones de hilo siguen activas.

Validación: 22 testigos SHA-pinned en `audit_local_draw_schedule.py`; 797
comprobaciones del selector; grafo Runtime 17 escenarios/1338 comprobaciones;
ABI sintética para contexto `EBP+8`, resultado AL stock, pila y registros;
enlace freestanding y cuatro pruebas del instalador. Copia separada con 46
enganches y reversión exacta; no instalada en Steam ni ejecutada.

SHA256 módulo LLVM 22.1.8:
`2852ab4d52e1e0381fa1faae25b61d981f4b694e86e2520ff21f42e1add4d47c`.
SHA256 copia de trabajo:
`5feadfed0b92791975b5372fdf5a4ac5bf5d8fb6e294f14ef12a5632343e7ab4`.

Continúa pendiente completar los gates/flags de interacciones, historial y
productores 3D, HUD/HP/Genesis, acciones, scripts, muerte/checkpoints y cámaras.
La comprobación real de funcionamiento queda reservada al propietario.
