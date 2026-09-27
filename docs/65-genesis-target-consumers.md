# Genesis: getters de objetivos durante la fase del clon

27/09/2026. Continúa PR36. No se ha abierto el juego.

Los getters de enfoque `0281F560` y posición `02823220` ahora consultan
TargetViews durante una llamada nativa del scanner de J2. Fuera de esa llamada
usan el trampolín stock, que reproduce exactamente los nueve bytes del prólogo.
Otros hilos también mantienen el camino stock sin leer el estado mutable del
scope del hilo propietario.

JanuaryBackend abre el contexto después de verificar admisión, recursos,
actividad y vista. Lo cierra después del delegado nativo de fase 8/9/11, antes
de comprobar sus postcondiciones. Runtime exige el unit exacto admitido por
ManagerDriver, widget vivo y sesión/frame estables. El contexto se limpia incluso
cuando falla la validación de cierre, para no afectar a posteriores calls stock.
No se abre un contexto para las fases que el motor omitiría por actividad o vista.

El getter de enfoque devuelve el valor acreditado de J2 o cero. El getter de
posición conserva el destino pasado por el caller, copia x/y/z y fija w=0; vuelve
con `ret 4`. Una muestra ausente/caducada/retirada entrega posición cero, sin tomar
la de J1. Las escrituras conservan los cuatro valores anteriores e intentan
restaurar las ya hechas si falla otra. El fallo se propaga al cierre y el backend
retiene el clon en cuarentena. No se modifica `target+10/+24`.

**Pendiente:** los setters del detector todavía no alimentan TargetViews. Queda
conectar su recorrido por propietario, actor/cámara y envíos al scanner; también
activación, bits de vista, eliminación de referencias y efectos. El clon conserva
la inicialización nativa inactiva. Estos getters no convierten por sí solos el
scanner en funcional.

Validación: Runtime 31 escenarios / 5190 assertions, con llamadas sintéticas
dentro de las tres fases, stock fuera de fase, muestras ausentes y rollback;
seis rutas ABI x86 (stock/local/hidden para ambos getters), ECX, destino, pila,
registros y trampolines; 38 testigos SHA-pinned; enlace freestanding y cuatro
pruebas del instalador. Copia separada con 95 hooks y reversión exacta, nunca
instalada ni ejecutada.

SHA256 módulo: `e07167447da4197528cc8b635fbdc7b7183e777c7bbcc930d23fa83641a8fba2`.
SHA256 copia: `5593b59acbe6f764bc6d4a1e45cf425df555fafff151338c6ef62deae8808604`.
