# Ejes del mando 2: productores que ignoraban el índice

08/10/2026, continuación de PR62. January 30 2013 solamente. No se ha abierto
ni ejecutado el juego. El movimiento jugando sigue pendiente de prueba del propietario.

## Defecto confirmado

El update de sGamePad recorre dos PadData (`02DAC8B8`, stride C0, base +668)
y llama `02DB70C0` con el índice del slot. Ese productor transmite el índice a
seis funciones que generan los dos pares de ejes y la alternativa digital.
Las seis reciben un argumento (RET4), pero ocho lecturas internas usaban
`mStartPadNo` (+970) en vez de ese argumento. Las correcciones históricas de
los consumidores no cubrían este productor: ambos PadData podían recibir los
ejes del mando principal. La copia local de PR62 conservaba los ocho sitios.
Esto es un defecto adicional a ThinkMode; no demuestra qué ejecutable probó
el propietario ni que explique por sí solo todos los síntomas de IA.

| Sitios | Lectura | Sustitución |
|---|---|---|
| 02DB1205, 02DB1295 | Primer par de ejes | EDX = [EBP+8] |
| 02DB1325, 02DB13B5 | Segundo par de ejes | EDX = [EBP+8] |
| 02DB143D, 02DB1461 | Cruceta X, bits 2000/8000 | ECX = [EBP+8] |
| 02DB14ED, 02DB1511 | Cruceta Y, bits 1000/4000 | ECX = [EBP+8] |

Los lectores inferiores sí usan el argumento: `01CB32A0/3350/3400/34B0`
leen +1B8/+1BC/+1B0/+1B4 con stride 2F8, y `01CB2FE0` lee los botones +198.
Se conserva el gate de ejes +982 y el cálculo nativo de magnitud/deadzone.
Cada reemplazo ocupa los seis bytes originales (MOV y tres NOP); conserva
flags, pila y registro destino. No escribe mStartPadNo ni fuerza un pad global.

## Integración y comprobación

`tools/runtime_image.py` instala los ocho reemplazos después de los 185 hooks,
con comprobación de bytes, ausencia de solapamientos, inspección posterior y
reversión exacta. No se reescribe la cadena histórica de hashes del builder.
`tools/check_local_input.py` informa por separado del binder y de los ejes:
`PAD_ARGUMENTS_PRESENT`, `LEGACY_GLOBAL_AXES` o `MIXED_OR_UNKNOWN_AXES`.
Su estado del binder no certifica acciones ni dispositivos físicos.

- Auditoría del original SHA-pinned: 33 testigos, sin ejecutar código nativo.
- Tres pruebas Python: 320 combinaciones de transporte de bytes nuevos,
  16 reproducciones del fallo anterior y detección de sitios mezclados/alterados.
  Usan el intérprete restringido ya existente; las llamadas al motor son stubs.
- Cinco pruebas de ELF/instalador: PASS, incluyendo separación entre hooks
  e instrucciones inline. Módulo compilado de PR62 sin cambios.
- Copia acumulativa: 185 hooks, ocho reemplazos inline, reversión exacta PASS.

SHA256 candidato local: `96cb422a44c39cc86372ca56d0c3e50b9449ff7ec35657e37358124954807b7f`.
Módulo: `8cf96b582c419b7e235b21d23b6876f84206a4c51d9343c8bbecce297f3306e5`.
Base: `152bce5dba9eb1a270d2fd392921883e772bfc42682b23497ae72452747814c7`.
No se instala en Steam ni se publica el EXE.

## Límites y siguiente revisión

Falta validar los dispositivos del propietario y el EXE que está usando.
La síntesis de teclado/ratón `02DB3590` tiene estado compartido (+974): cuando
está activa borra ambos bloques raw (+198/+490) antes de sintetizar entradas.
Ese comportamiento merece una corrección separada con evidencia completa;
esta modificación no promete todavía teclado/ratón J1 más mando J2.
También faltan cobertura total de acciones, transiciones de modo/escena y el
resto del cooperativo enumerado en CURRENT_STATUS.md. No equivale a entrega final.
