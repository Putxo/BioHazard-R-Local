# Vida observada del arma y retención del Scanner

27/09/2026. Continúa PR52. Juego no ejecutado; activación J2 todavía pendiente.

Comparar `Scanner+2FC` con el slot equipado no detectaba una destrucción seguida
de una nueva asignación en la misma dirección. El arma concreta del Génesis es
`uWpScanner`, VT `04D38EE4`, DTI `80051B30`, tamaño `12B0`.
Ahora se observa su construcción en `0248731E` y su destrucción en `02487426`.
El observador concede generaciones monotónicas y revoca antes de la limpieza
nativa. No lee objetos que se están destruyendo ni expulsa entradas vivas al
llenarse sus 64 slots. Nacimientos duplicados, reentrada y eventos fuera del
hilo propietario invalidan la admisión; no se supone que sean vidas nuevas.

El tercer gateway está en la primera escritura de la activación,
`02B20D99`. Antes de guardar el arma exige la instancia observada, inventario
propio, propietario/sesión/frame válidos, ticket vivo del conjunto y efectos
privados admitidos. Conserva la generación, owner, Scanner y ticket para las
fases posteriores. Si falla, salta al epílogo original `02B21097`, con pila y
registros restaurados, antes de los cambios de activación. Stock conserva su
escritura y recorrido. Los tres enganches elevan el total a **137**.

Las admisiones de fase/detector/completado vuelven a comprobar esa retención.
Una dirección reciclada queda rechazada hasta una nueva retención válida.
La identidad sigue siendo válida entre frames de la misma sesión; no se
transfiere a un nuevo ticket. El caso nativo `+2FC == 0` sigue permitido.

Esto **no mantiene físicamente viva el arma durante callbacks** ni sustituye
las barreras que necesitan sus usos nativos intermedios. Se localizaron accesos
adicionales a `+2FC` en activación, cierre y estados del Scanner; quedan por
conectar esas barreras antes de habilitar la activación. Tampoco convierte este
observador concreto en un observador de todas las armas usadas por Scope.

Validación: 120 comprobaciones del observador, 180 escenarios Runtime / 43.710
comprobaciones, muerte/reciclado/falta de observación/arma ajena y retención entre
frames. ABI sintética de nacimiento, muerte y retención admitida/rechazada;
15 testigos estáticos; enlace freestanding; cuatro pruebas del instalador y
reversión exacta de la copia independiente. Ningún binario propietario subido.

Módulo: `ab4aa77e366289b209f7c51e9d2839deb0dad9887eb8fa9faf16cf88a451c2d1`.
Copia: `443f3b3261e94195e1b17d243a2fe266d284ec1c6f45f078985c2e02a9d21625`.
