# Visor de arma independiente

27/09/2026. Continúa PR42. Juego nunca ejecutado.

El widget en Cockpit+54 se identifica por RTTI como uGUI_Scope: es el visor,
no una segunda instancia del retículo ni de Main/SubEquip. Se incorpora como
WidgetKind::Scope (índice7), el octavo tipo del runtime. Asignación, constructor,
inicialización, destructor y fases usan exclusivamente las direcciones del
EXE de enero verificadas por el auditor. Se construye separado del original,
fuera de las listas nativas, asociado al actor J2 y View1.

La captura de recursos comprueba el árbol GUI, sus tres nodos cacheados en
+2A0/+2A4/+2A8 (índices0/1/2) y la animación cacheada en +2AC (índice0).
Congela las identidades del contenedor de animaciones y de todas sus entradas,
con límite del adaptador de32. Dos lecturas rechazan reemplazos ordinarios;
no constituyen un bloqueo contra destrucción concurrente. Comprueba también
que las animaciones no se compartan con el Scope original, y revalida el grafo
original y el clon en las fases y antes de destruir. El template puede compartirse;
los nodos, tabla e instancias mutables deben ser distintos.

La fase8 nativa obtenía Self en 02B41E36 y usaba actor+1540 para el estado de
apuntado. El gateway selecciona el actor admitido del clon durante su fase.
Si la identidad ha sido revocada, salta al epílogo original sin continuar con
un puntero nulo: no hay comprobación nula en el consumidor nativo. El visor
original mantiene su finder. La máscara temporal de originales incluye ahora
Scope: en la vista del J2 se dibuja su instancia, conservando el visor del J1.

01BF35CB, que aparece en esta fase, es una conversión x87 a entero, no un selector
global de jugador ni una preferencia del visor. No se redirige. La condición
compartida de dibujo del mundo tampoco cambia de índice.

El inicializador deja Scope inactivo. La alimentación de activación/cierre
al apuntar se incorporó después en [el bloque de eventos](72-scope-aim-events.md),
usando el arma actual del J2 y su segundo argumento. Genesis sigue pendiente.

## Validación

- Runtime: 82 escenarios /14376 comprobaciones, con ocho widgets, retirada y
  rechazo de cachés/nodos/animaciones del visor reemplazados o compartidos.
- Recursos de Scope: 142 comprobaciones; fallos de cada lectura, cambios entre
  capturas, límites, duplicados, alias y conservación de la salida en fallo.
- ABI x86 sintética: stock/local/revocado, selección nula, argumento original,
  ECX, registros, pila y salida directa por el epílogo del propietario.
- 145 testigos del backend y16 de Scope en el original fijado por SHA256.
- Enlace freestanding y cuatro pruebas del instalador:112 hooks, reversión exacta.

SHA256 módulo: `74b9d7db961b802cd5b2bb9983221fdd1d307855ffd47954e21d48e7dbd0fd6c`.
SHA256 copia separada: `7f7273673f8c7af43cd44716c83ca233aa6640639d50c018eac5d75703c9e799`.
La copia no se ha instalado ni ejecutado. El cooperativo completo sigue pendiente.
