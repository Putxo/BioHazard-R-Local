# Apertura y cierre del visor del J2

27/09/2026. Continúa PR43. Juego nunca ejecutado.

La rama de segunda cámara de 0279CF80 ya alimenta el Scope separado al apuntar.
El gateway conserva también el segundo argumento de la función original:
solo su byte bajo controla la selección de animación. El actor y el arma siguen
usando únicamente el bit de visibilidad de View1. Si falla una escritura,
no se publica una orden de visor parcialmente aplicada.

La orden conserva la identidad completa de sesión/actor y el ticket del HUD,
sin retener el arma del evento. Durante fase8 del Scope se vuelve a resolver
el arma equipada del J2, independiente de los quince slots del J1. Se consulta
la clase mediante virtual+10 y DTI+1C como el original: 80051B30 es Genesis.
El identificador de ítem usado por el panel es una consulta diferente.

Arma ordinaria y byte de apuntado distinto de cero llaman 01BE9C6F con el clon,
el arma y el segundo argumento. Al soltar, sin arma o con Genesis, se llama
con arma nula y flag1. La activación nativa usa sincrónicamente el arma para
elegir panel; el cuerpo examinado no la retiene. La validación de recursos del
Scope envuelve también la activación, antes de comprobar si el widget está
activo: así se puede abrir una instancia inicialmente inactiva.

Se comprueban propietario, frame, ticket, cámara y arma antes y después de las
llamadas. Un cambio revoca la fase. La última orden pendiente sustituye a las
anteriores; si llega otra durante una llamada, queda pendiente para la fase
siguiente. Las fases sin cambios de orden/arma/clase no reinician la animación.
Un ticket nuevo descarta órdenes del HUD anterior aunque el actor conserve
la dirección. El Scope original y Cockpit+54 no se sustituyen.

Límites: las lecturas de identidad no son un pin de vida del arma ni resuelven
ABA de arma en la misma dirección. Se conserva la serialización del hilo del
motor; la destrucción concurrente/reentrante de armas no está demostrada.
La cobertura de los trece productores de apuntado necesita todavía revisión
por acción. Este bloque no activa Scanner/Genesis, cuyos objetivos, efectos,
recompensas y retirada durante fases nativas siguen pendientes. Tampoco
demuestra que el cooperativo completo funcione jugando.

## Validación

- Runtime: 103 escenarios /21464 comprobaciones. Apertura desde inactivo,
  cierre, low bytes, arma nula/Genesis, cambio de arma/clase, coalescencia,
  ausencia de reinicio repetido, reentrada, recursos sustituidos, owner/cámara
  revocados, fallo de escritura y recreación del HUD con otro ticket.
- ABI x86 sintética: los tres argumentos del adaptador, registros, flags,
  pila y los dos argumentos originales con RET8.
- 26 testigos estáticos de Scope en el EXE fijado por SHA256.
- Enlace freestanding y cuatro pruebas del instalador:112 hooks, reversión exacta.

SHA256 módulo: `607a797db2f4bf03c4b668c4d9e347552da9af26f37358d449b0c9e5988d3103`.
SHA256 copia separada: `7efd9e6d224d368a09240f34c626c6e7eb25347eb3929995bd1efce0592937c3`.
Copia sin instalar ni ejecutar. Solo se publican fuentes, pruebas y documentación.
