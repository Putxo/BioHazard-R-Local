# Genesis: fronteras de propiedad que hay que separar

27/09/2026. Hallazgos estáticos tras PR30. **No implementa todavía Genesis J2**.
Auditor: `scripts/audit_genesis_ownership.py ORIGINAL.exe`, 36 testigos fijados
al SHA de January. El juego no se ha ejecutado.

## Widget y consumidores

Cockpit construye `uGUI_GadgetScanner` (400 bytes) en 02B486FD y lo almacena en
+48. Su vtable es 04DE3D6C. El getter 01B86273 -> 01F134D0 devuelve ese campo.
Se verificaron estas diez llamadas directas al getter:

| Sitio | Operación observada |
|---|---|
| 01F13310 | Consulta activo y notifica al widget |
| 02658129 | Consulta activo desde otra unidad; propietario todavía por resolver |
| 0279D0C6 | Activa con dos argumentos, uno es el arma encontrada |
| 0279D0EA | Consulta activo antes de cerrar |
| 0279D100 | Cierra el escáner |
| 0281F0CA | Consulta activo y cambia el estado de otro objeto |
| 028206F5 | Envía un elemento encontrado |
| 02820738 | Recorre una colección y envía elementos |
| 02823A8B | Entrega una referencia al widget antes de la llamada final |
| 02B07432 | Consulta global de disponibilidad, sin actor en los argumentos |

La función de visibilidad del jugador 0279CF80 compara su identidad resuelta
con la identidad principal en 0279CFAD..0279CFBD y retorna si difiere. También
modifica el bit de vista 0 del actor y su equipo. Admitir J2 en esta función
requiere remapear ese bit a su vista, además de redirigir el widget; cambiar
solo la comparación produciría interferencia visual con View0.

La activación 01B9E86E -> 02B20D70 almacena el segundo argumento en +2FC. La
clausura 01C6EE1F -> 02B211C0 lo limpia, vuelve a buscar Self en 02B214B8 y
le aplica una operación en 02B214CB. Los tres callbacks deben pertenecer al
mismo actor vivo. La clausura tiene efectos de gameplay y no puede usarse
como una simple rutina de liberación de GUI al retirar clones.

## Progreso y recompensa

01B7AD9C -> 01D5B3C0 obtiene el gestor mediante 01B883CF -> 01D5B4B0.
El accessor comprueba index < 1 y calcula 05563050 + index*4. No demuestra un
segundo gestor válido para J2. El getter 01B82362 -> 02A94F30 lee su campo +74.

La clausura consulta ese valor y compara con 100 en 02B211EF..02B211F7.
En 02B21230..02B21244 resuelve un actor mediante el personaje seleccionado
globalmente; en 02B2127A..02B21286 aplica una operación con argumento 1 a su
pack. En 02B21299..02B212A7 vuelve a escribir el progreso del gestor único.
Por ello, un segundo widget por sí solo seguiría compartiendo progreso y
podría entregar el resultado al pack equivocado. La identidad global no debe
reescribirse temporalmente para resolverlo.

## Recursos y siguiente integración

Además de la raíz GUI, el scanner retiene el arma, colecciones de objetivos,
elementos y animaciones. Los campos +380 y +3B8 se rellenan con consultas nativas
al GUI (01C27BF5 y 01B8ABDE). Falta completar la prueba de pertenencia de esas
referencias y su retirada antes de activar un clon.

La integración debe conectar el actor propietario en cada productor, separar
el progreso y su consumo, dirigir la recompensa al pack de ese actor y retirar
las referencias a objetivos antes de liberar el widget. La consulta sin actor
necesita una política explícita; no puede inferir J2 por proximidad ni por el
mando que se leyó más recientemente. Escaneo simultáneo, recompensas, cambio
de arma, transición de escena y destrucción del objetivo necesitan pruebas
sintéticas y la posterior comprobación jugando del propietario.
