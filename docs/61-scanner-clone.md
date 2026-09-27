# Clon Genesis, recursos y consumidores de actor

27/09/2026. Continúa PR32. Integración para January; no se ha ejecutado el juego.
**Genesis J2 todavía no se activa desde el mando.** Este bloque prepara el clon
y conecta sus consumidores; quedan los productores y la vida de los objetivos.

Scanner es el séptimo tipo HUD: vtable 04DE3D6C, tamaño 400, original en
Cockpit+48. Se construye separado y se inicializa con las funciones nativas.
Conserva el estado inactivo de la inicialización hasta recibir una activación.
La máscara temporal limita el original a View0 y restaura sus bits al salir.

Los 26 sitios de progreso usan el contador independiente de PR32 cuando el
registro certifica el clon, Sub0, su sesión y el hilo. El original conserva
el getter/setter/sumador stock. Se redirigen seis búsquedas Self y seis búsquedas
del actor seleccionado al propietario del widget; ambas familias respetan ret4.
Estas rutas cubren los consumidores identificados de inventario/recompensa, pero
no habilitan aún el ciclo completo de activación, escaneo y recompensa.

## Propiedad de recursos

Además de la raíz y los nodos comunes, la captura especializada enumera la
tabla de animaciones (+224 count, +230 puntero), cuatro paneles anidados y sus
tablas/nodos/animaciones, y dos pools de 21 elementos: 48 y 28 bytes con cookies
de 8 y 4 bytes respectivamente. Las plantillas de recurso permanecen compartidas.
Los bloques mutables se comprueban por rangos y no pueden solaparse con la
captura del scanner original. Se conservan snapshots para detectar sustituciones
antes y después de fases nativas y antes del destructor.

Se verifican 15 cachés de elementos, 14 de animaciones y 11 de nodos anidados.
Los siete nodos del panel +35C usan índices **4,3,2,10,9,8,7**; los cuatro
restantes usan +370/index1, +374/index2 y +378/index2/3. No son índices sucesivos.
El accessor 01C0636F también lee la tabla de nodos, aunque verifica otra clase.
Una referencia cacheada que no corresponde a su propia tabla rechaza la fase.

El destructor escalar retira sus pools y llama a la rutina de limpieza propia;
no se sustituye por la clausura de gameplay, que puede otorgar una recompensa.
La captura cubre la GUI inicial y pools. La unión de objetivos, los efectos
dinámicos (incluido +2F0), el arma +2FC y las notificaciones de retirada siguen
pendientes antes de conectar los productores. Tampoco se cambia el gate que
impide a J2 entrar en la función de visibilidad ni sus bits de vista del equipo.

## Verificación

- Runtime: 25 escenarios / 3861 comprobaciones, clon registrado, contador J2,
  actor destruido, hilo incorrecto y rechazo de una animación cacheada prestada.
- Recursos: 134 comprobaciones; el fixture mínimo contiene 63 regiones propias.
  Incluye alias de pools, cookies, cachés, nodos anidados y captura incompleta.
- Máscaras: 16 escenarios / 58 comprobaciones y restauración de Scanner.
- ABI sintética de ambas familias de actor, stock/local/hidden, ECX, argumentos,
  pila y registros preservados. No carga funciones nativas del juego.
- Auditor de recursos: 54 testigos del original. Auditor HUD: 130 comprobaciones.
- Compilación freestanding y cuatro pruebas del instalador. Copia separada con
  90 enganches, reversión exacta y sin ejecución ni instalación en Steam.

Módulo: ddbc174bc7257192a99b772c2df62a31ae8f99ba51623a9ce9faaee2e4f20f27.
Copia: 18702dbd86af8e559d7deee4f607ece1e2b871ef268029483fc705f74f2feb8f.

El cooperativo completo sigue en desarrollo. Las comprobaciones jugando las
realizará el propietario cuando se complete la implementación.
