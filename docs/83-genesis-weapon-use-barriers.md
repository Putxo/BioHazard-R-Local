# Lecturas intermedias del arma del Génesis

27/09/2026. Continúa PR53. No se ha ejecutado el juego.

Las comprobaciones al entrar y salir de una fase no cubrían los accesos al
arma después de llamadas nativas dentro de esa fase. Se han conectado **40**
lecturas de `Scanner+2FC` que obtienen el modelo y ordenan cambios en él, más
el wrapper de reactivación `02B20D26`. Total acumulativo: **178 hooks**.

Cada admisión local comprueba owner/sesión/frame, ticket, efectos privados,
retención observada e inventario actual. Para un comando exige también un
modelo no nulo en `uWpScanner+EC4`, antes del getter que ejecutaría un assert
si falta. Repite la comprobación de arma y frame después de esas lecturas.
Una destrucción, reciclado, cambio de inventario o invalidación revoca los
siguientes accesos. Stock conserva el recorrido nativo. La ruta off-thread
sigue pasando stock; no se declara soporte local en workers.

Los 40 sitios tienen exactamente dos argumentos `push imm8` pendientes, un
getter sin argumentos, `mov ECX,EAX` y una llamada `ret 8`. El gateway `CALL`
sustituye la carga de seis bytes (con NOP de relleno). Si admite, repite la
carga y vuelve al getter. Si rechaza, consume esos ocho bytes y continúa tras
ambas llamadas, dentro de la misma función. Así se conservan los destructores
locales y epílogos originales. No se salta arbitrariamente fuera de la fase.
El wrapper todavía no tiene argumentos pendientes y salta a su epílogo normal.
Los registros y flags se restauran antes de continuar por cualquiera de las rutas.

El fallo pone el Scanner en cuarentena y revoca los gestores sin liberar los
objetos sincrónicamente. La retirada se procesa en su frontera estructural.
Esto **no fija físicamente la memoria del arma o modelo durante callbacks**.
Los comandos de modelo todavía contienen llamadas virtuales/indirectas; la
seguridad de toda esa cadena y los gestores visuales globales siguen pendientes.
La activación J2 permanece deshabilitada hasta resolver esos límites.

Validación sin juego: 194 escenarios Runtime / 45.495 comprobaciones, cuatro
rutas ABI sintéticas (comando y wrapper, admitidos/rechazados), 95 testigos
estáticos ligados al SHA January, enlace freestanding, cuatro pruebas de ELF
/instalador y reversión exacta de los 178 enganches en una copia independiente.

Módulo: `d396788c8639f270db96584955fbd75075723a167fc6ae03950136cbf4b5616d`.
Copia: `0b27bdeda743e75359fc729ba832fe12d8f0a5757a78ffd454dcdd27a749ac10`.
Ningún binario propietario se publica o instala en Steam.
