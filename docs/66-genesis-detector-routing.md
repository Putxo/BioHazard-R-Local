# Genesis: recorrido de detección para J2

27/09/2026. Continúa PR37. No se ha ejecutado el juego.

Los seis callers auditados de `01BBECD1 -> 0281FB30` conservan una llamada stock
con su ECX original. Solo con sesión, frame, hilo, cámara de J2 y clon activo
acreditados se admite una segunda pasada síncrona sobre ese mismo gestor de
objetivos. El gestor pertenece a cada productor; no se sustituye por un singleton.
Una llamada anidada recibe su pasada stock con las lecturas de J2 suspendidas;
al volver se restaura el contexto exterior y no se duplica de nuevo el recorrido.

Durante la segunda pasada, los getters de cámara, actor y scanner devuelven la
cámara Sub0, el actor observado y el clon propio. Los setters nativos de posición
y enfoque publican en TargetViews, sin escribir `target+10/+24`. Escribir posición
inicia una clasificación sin enfoque; los setters posteriores la completan.
Las lecturas nativas del detector consumen esa misma muestra. El envío de un
candidato exige su lifetime observado y una muestra válida. Los getters de cámara
y scanner conservan las salidas nulas que el código nativo comprueba.

La precondición del clon valida vtable y métodos, actividad, skip, desconexión de
sUnit, recursos congelados y separación respecto al scanner original. No concede
permiso de asignación ni de fase HUD. Fuera del contexto productor, incluyendo
otros hilos, setters y getters siguen el camino stock. Un fallo durante la pasada
impide utilizar los datos de J1 como sustitución; al salir se detiene el clon.

El filtro de la primera entrada sigue consultando índice 0 del singleton nativo.
El índice 1 no representa a J2: el accessor limita la tabla a una sola instancia.
Self, mStartPadNo, GameMode, serial y Network globales no cambian.

**Límite deliberado:** el clon continúa con la inicialización nativa inactiva.
Todavía falta conectar activación/cierre, bits de vista del actor y arma, retirada
síncrona de referencias de objetivos y efectos. La segunda pasada está conectada
pero no se activa en juego por este cambio. Tampoco se ha cerrado la auditoría de
los estados persistentes `target+20` y notificaciones del mundo durante una segunda
pasada: se conserva el código nativo y debe resolverse antes de activar el clon.
No se considera un Génesis funcional ni un cooperativo terminado.

Validación: Runtime Win32 42 escenarios / 6712 comprobaciones, incluyendo stock,
publicación de posición/enfoque, objetivos no observados, cámara ausente, actividad,
hilo, imagen, alias de recursos, frame cerrado, reentrada y fallo de lectura.
21 rutas ABI sintéticas para siete gateways, pila, registros, argumentos y
trampolines. 48 testigos SHA-pinned. Enlace freestanding y cuatro pruebas del
instalador PASS; 108 hooks con reversión exacta en copia separada.

SHA256 módulo: `767b339995e8ab86b2395a0318346a80ea19ed8842efdf6b3b488f99ede73a57`.
SHA256 copia: `792e94011b49a18318a4a1d1739677044ce793a738f594a83f8e7b0e7303ac0f`.
No se instala ni se publica ningún ejecutable del juego.
