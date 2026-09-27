# Genesis: retirada síncrona de objetivos del clon

27/09/2026. Continúa PR38. No se ha ejecutado el juego.

El destructor del objetivo solo eliminaba su referencia del scanner original.
El nuevo gateway en `02823AA8` llama a la limpieza de J2 antes de reenviar siempre
al cleanup base `01C919B5`. Esta frontera es incondicional: también funciona si no
existe el scanner original o si la condición de modo del bloque stock lo omite.
La revocación de TargetLifetime sigue ocurriendo antes, en `02823A53`.

Se elige esta frontera porque `target+8` ya es cero. El reset de la entrada nativa
puede llamar a `01C7EAF9 -> 02823C60`, que notifica materiales de su owner; con
owner nulo retorna sin recorrerlos. El preflight exige ese cero y la vtable base
del objetivo. No se encola un puntero para utilizarlo después de su liberación.

El backend certifica el clon retenido aunque esté inactivo, con ForceSkip o fuera
de un frame abierto. Comprueba recursos propios y las cuatro listas intrusivas:
iconos libres/activos y objetivos libres/activos. Cada uno de los 21 elementos de
cada pool debe aparecer exactamente una vez. Verifica sentinelas, enlaces en ambos
sentidos, vtables, punteros de datos, referencias entre los pools e índices GUI
0..4 o -1. No desreferencia los objetivos del mundo para certificar estas listas.
Dos capturas detectan cambios ordinarios; no constituyen un bloqueo entre hilos.

Solo si el objetivo está presente llama una vez a `01BB4079 -> 02B29F60` sobre el
clon. Después exige que desaparezcan esa referencia y su icono, conservando las
restantes. Las listas del scanner original no se escriben. Una segunda llamada
para el mismo objetivo ya retirado no repite la operación nativa. Un fallo de
precondiciones, recursos o postcondiciones pone el clon en cuarentena, evitando
su destructor nativo sobre referencias cuya validez no se ha demostrado.

El certificado de listas también se aplica antes/después de fases del scanner,
antes de su destructor y en la admisión del productor de detección. Si el
observador de targets está en fallo, no se admite una nueva fase del clon.

**Pendiente antes de activar el clon:** destrucción reentrante durante una fase
nativa del propio scanner, efectos, activación/cierre, bits de vista y revisión de
notificaciones y estados persistentes en la segunda pasada. El backend rechaza
la limpieza mientras está ocupado; eso no demuestra que una pila nativa que ya
está ejecutándose sea segura frente a cualquier destrucción reentrante. Muertes
fuera del hilo propietario no manipulan las listas: el observador invalida la
admisión. No se atribuye soporte general a destrucción concurrente en workers.
El clon sigue inactivo por inicialización nativa.

Validación: 848 comprobaciones de colecciones (0..21 objetivos, campos alterados,
ciclos, referencias cruzadas, duplicados, índices GUI y cambios entre capturas);
Runtime 48 escenarios / 8366 comprobaciones; ABI sintética de orden, ECX, pila,
registros y retorno del cleanup base. 70 testigos SHA-pinned, enlace freestanding
y cuatro pruebas del instalador PASS. Copia separada: 109 hooks, reversión exacta.

SHA256 módulo: `eb19da2939508ba1a4944b9d7c4d4ff44031979552b28175371e623094bca64e`.
SHA256 copia: `278b2fe72149c3129f988df24d2453b201af2f41e1c6f7e16d4d88e9b996c601`.
No se instala ni se publica ningún ejecutable del juego.
