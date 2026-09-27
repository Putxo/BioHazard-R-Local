# Visibilidad de J2 al apuntar

27/09/2026. Continúa PR41. Juego nunca ejecutado.

La rutina 0279CF80 salía por 0279CFBD cuando la cámara del actor era distinta
de la primaria. El gateway nuevo ocupa únicamente ese salto de cinco bytes y
continúa en el epílogo original 0279D11A. La rama admitida originalmente para
J1 no atraviesa el gateway. ECX, registros, flags, EBP y los dos argumentos
originales se conservan.

En esa salida secundaria, el runtime admite solo el actor Sub0 vivo de la sesión
local, con scanner registrado y cámara secundaria vinculada a ese actor. Resuelve
el arma desde su inventario usando PR41. Reproduce el predicado nativo
01BB2FE4 para determinar si también debe cambiar la visibilidad del arma; consume
su resultado AL como booleano y revalida después cámara, frame, actor e inventario.
Una llamada anidada no entra por segunda vez.

Cuando el primer argumento vale 1 en su byte bajo, limpia bit17 de object+0C
(View1); en los demás casos lo restaura. Es la adaptación a View1 del getter y
setter stock que cambian View0. Conserva View0, las otras ocho vistas y todos los
flags ajenos a visibilidad. Si falla la escritura del arma, restaura el personaje;
si la restauración también falla, bloquea esta ruta y detiene el lifecycle del HUD.

El personaje puede actualizarse sin arma equipada; ese caso no invoca el
predicado del arma. No se guardan punteros a personaje/arma para otras llamadas.
Esta ruta añade la visibilidad local al apuntar, pero todavía no activa el scanner
ni el widget de arma Cockpit+54. Los efectos de Genesis y su ciclo de vida siguen
pendientes. Tampoco sustituye la comprobación visual que hará el propietario.

## Validación

- Runtime: 77 escenarios / 12717 comprobaciones. View0 intacta, ocultar/restaurar,
  predicado falso, arma nula, cambio de equipo/sesión/cámara durante el predicado,
  hilo incorrecto, alias con J1, reentrada y fallos de lectura/escritura/restauración.
- ABI x86 sintética: tres valores del argumento, registros, flags, frame original,
  salto de continuación y limpieza ret8. No carga ni ejecuta código del juego.
- 100 testigos estáticos contra el EXE de enero fijado por SHA256.
- Enlace freestanding, cuatro pruebas del instalador y reversión exacta: 111 hooks.

SHA256 módulo: `025e38fc22289c3e04d8aead75283feb5ce80c9452de8c1d63bfa70f89c12fad`.
SHA256 copia separada: `3844af26a23e88d1c031025cb2759727e157e95a44637f835bf759008eae1001`.
No se ha instalado ni ejecutado la copia.
