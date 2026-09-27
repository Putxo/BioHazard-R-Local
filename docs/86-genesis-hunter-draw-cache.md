# Hunter: reconstrucción de comandos para cada vista local

27/09/2026. Continúa PR57. No se ha ejecutado el juego.

El cambio visual del Hunter al usar el Genesis termina en materiales del mismo
actor. El dibujo de `uModel` puede reutilizar un objeto de comandos guardado en
`model+200+4*passKind`. Separar solamente el getter del booleano de Genesis no
separa esas instrucciones de dibujo ni sus parámetros.

Se conecta un enganche en `034F1EEC`. Evalúa primero el predicado stock
`01BE0142 -> 01FA2660` (bit 8 de `resource+4C`). Para un `uHunter` concreto
(`VT 04D09FBC`) dentro de View0 o View1 locales devuelve verdadero. El caller
selecciona entonces el valor nativo 5 y salta en `034F1F2E` a `034F21C1`, donde
reconstruye el dibujo con las partes actuales mediante virtual `+6C`.

La selección exige snapshot local en el hilo propietario, sesión/frame,
actores distintos con lifetime/serial válidos y contexto `VT 04F79014` con
vista 0/1. Repite las lecturas de tipo/contexto/vista y el snapshot. Si falla,
conserva el predicado stock. Otros tipos, vistas auxiliares y workers no
reciben la selección local. El enganche no modifica flags del recurso,
materiales ni punteros de caché; su coste es reconstruir comandos para cada
dibujo local de Hunter en lugar de aprovechar esa caché.

El gateway usa los slots reales del caller (`EBP-4` modelo, `EBX+8` contexto),
normaliza AL y preserva registros y flags posteriores al getter stock. No
retiene esos punteros después de la llamada. Total acumulativo: **182 hooks**.

## Materiales que faltan por separar

`021FD970` es una actualización de estado, no una función pura de dibujo.
Su llamada al getter `01C333C9 -> 021FEE80` lee `sHunterManager+65` y cambia
el alfa local de 1.0 a 0.99 cuando Genesis está activo. También avanza
osciladores, escribe vectores en `Hunter+1AE0/1AF0/1B00`, altera alfa de
`uModel+158`, flags de visibilidad y `Hunter+1B24`. Ejecutar toda esa
actualización dos veces por frame produciría cambios de estado adicionales.

Las partes de modelo se obtienen de `+F8/+FC`. El material `6C8011F4` recibe
tres vectores RGB dependientes del alfa en índices 44, 0 y 40; `EFCA322B`
recibe el escalar alfa en índice 1. Los setters `021FED90/021FEDF0` escriben
en buffers CPU obtenidos por `036C8980`; no prueban por sí solos cuándo el
renderizador copia o consume esos buffers. El bind por parte pasa por
`01C3B12D -> 03700980` y virtual `+30` del material. Ese consumo, las flags
de visibilidad y la restauración por vista siguen pendientes.

Esta entrega elimina la reutilización del objeto de comandos en el recorrido
local auditado. **No declara terminada la separación de materiales Hunter**
ni el clipping de filtros; la activación Genesis J2 continúa deshabilitada.

## Validación sin juego

- 1.108 comprobaciones de selección Hunter, cambios entre snapshots, vistas,
  lecturas fallidas y preservación de la ruta stock; 797 del selector sUnit.
- Tres rutas ABI sintéticas, con stubs que alteran registros volátiles y flags.
- 21 testigos estáticos del caller y del recorrido sin caché, ligados al SHA
  del ejecutable January original mediante `audit_genesis_hunter_cache.py`.
- Runtime: 217 escenarios / 49.858 comprobaciones; cuatro pruebas de enlace e
  instalador; reversión exacta de 182 enganches en una copia separada.

Módulo: `2dfca3b397bc2e2cc920e12992ea2a12520f1cb458d3b1059506ec40271e7300`.
Copia: `aa3bec4029cc85ab1de2d20c2768b9442aa858cb385519a39ac02beb805c870f`.
No se instala en Steam ni se publican binarios propietarios.
