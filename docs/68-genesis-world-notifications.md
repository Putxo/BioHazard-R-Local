# Genesis: contexto de eventos y duplicados entre las dos pasadas

27/09/2026. Continúa PR39. El juego no se ha ejecutado.

El detector invoca un delegado del mundo en `028206D2` cuando su enfoque vale
2 o 3. `01C315EC -> 020CA500` llama la función guardada en delegate+4, con el
delegado como argumento. Ese callback puede volver al motor, cambiar sesión o
destruir un objetivo. No debe heredar los getters/setters globales de la pasada
de J2 ni recibir una segunda notificación idéntica solo por añadir otra vista.

El gateway obtiene target de EBP-1B8, owner del mundo de EBP-300 y el delegado
del ECX nativo. Conserva todas las llamadas stock. Durante la pareja de recorridos
stock/J2 registra las notificaciones por slot y generación observada del objetivo,
owner y función del delegado. J2 omite una coincidencia ya entregada; un objetivo,
owner o función diferente conserva su notificación. El registro se limpia en cada
recorrido exterior nuevo, incluso si ocurre dentro del mismo frame: no es una
deduplicación global de eventos, ni cambia el número de llamadas originales.

Para un evento nuevo de J2 suspende su contexto de scanner y ejecuta el delegado
nativo de forma síncrona. Los callbacks ven getters y setters stock. Una llamada
recursiva al detector no inicia otra pasada de J2. Al regresar se restaura la
identidad del contexto exterior y se comprueba de nuevo sesión y frame.
No se guarda un puntero a la copia temporal del delegado para después del retorno.

Si el callback destruye el objetivo, el observador revoca su generación y la
limpieza de PR39 puede retirar la referencia mientras el backend está libre.
La siguiente admisión de candidato lo rechaza sin volver a introducirlo en la
lista. Si cambia la sesión, se invalida el contexto y se detiene el clon.
Esto cubre la reentrada desde el delegado del detector, **no** demuestra soporte
para destrucción durante una fase nativa del propio scanner o en workers.

El estado target+20 sigue compartido. Tiene productores ajenos al enfoque:
un constructor derivado lo inicia en 1; la rutina 02823940 y la operación
02823AF0 pueden establecer 2. No se copia ese estado por jugador ni se interpreta
su valor como porcentaje de escaneo. Siguen pendientes las restantes operaciones
de progreso/recompensa por tipo de objetivo, efectos, activación/cierre y vistas.
El clon conserva la inicialización inactiva; aún no es un Génesis funcional.

Validación: Runtime 54 escenarios / 9295 comprobaciones, incluyendo evento
stock/J2 coincidente, nuevo recorrido, owner/función distintos, llamadas anidadas,
destrucción del objetivo dentro del delegado y cambio de sesión. ABI x86 sintética
para locales EBP, ECX, argumentos, pila, registros y rutas stock/handled. 77 testigos
SHA-pinned; enlace freestanding y cuatro pruebas del instalador PASS. 110 hooks y
reversión exacta en una copia separada, no instalada ni ejecutada.

SHA256 módulo: `85a2f6655960147b0317320030bc8d26e1657ce371e49cad2027106571a244a0`.
SHA256 copia: `612649d99b7deeca4fd7cde2d49765edb8000b1261ddb6013e349f58c1d34fac`.
