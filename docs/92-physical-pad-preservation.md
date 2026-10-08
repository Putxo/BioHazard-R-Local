# Conservar el mando físico durante la síntesis de teclado/ratón

08/10/2026. Continúa PR63. January 30 2013. No se ha ejecutado el juego.

## Fallos y cambios

El update nativo primero sondea sPad, después ejecuta `02DB3590` y finalmente
actualiza los dos PadData. Cuando sGamePad+974 es distinto de cero, la síntesis
teclado/ratón borra 44 bytes de **ambos** bloques raw: +198 y +490. El código
posterior genera sus entradas principalmente en el slot elegido por +970.
Por ello, incluso después de corregir los lectores de ejes, la entrada física
del otro mando puede desaparecer antes de llegar a PadData.

Dos gateways en las llamadas memset `02DB36A1/02DB36B6` conservan únicamente
el bloque del slot distinto a mStartPadNo. El bloque del teclado sigue la
llamada original. La decisión exige:

- hilo propietario y pareja Main/Sub0 observada, viva, distinta y aún local;
- asociación PCS, seriales, generaciones y modos coherentes, sin binder abierto;
- objeto sGamePad idéntico al singleton y vtable exacta 04E11D30;
- índice principal 0/1, síntesis activa y destino exacto del otro bloque;
- llamada de borrado de 44 bytes con valor cero;
- revisión estable, doble lectura del estado de input y nueva comprobación de
  los actores después de esas lecturas.

No depende del HUD. La política puede conservar el mando durante Invalid0,
igual que la propiedad local de PR62; no activa acciones durante la suspensión.
Con evidencia incompleta, modos CPU/red, otro hilo, muerte/reasignación o una
llamada distinta se ejecuta el memset nativo. No se escribe mStartPadNo.
El gateway conserva registros, flags y FP/SIMD. La ruta que omite el borrado
devuelve el destino, como memset, y deja al llamador limpiar sus tres argumentos.

También se corrigen las consultas de flags PadData 3/4 en `02DAE728/02DAE898`:
los productores `02DBA970/02DBA6D0` transmiten el índice, pero el getter lo
ignoraba y seleccionaba el PadData principal. Ahora usa [EBP+8]. No se asigna
un nombre de acción a esos bits sin evidencia adicional.

## Validación

- 20 testigos sobre el original January SHA-pinned: orden del update, singleton,
  memsets y su convenio cdecl, getters y transporte del índice.
- LifetimeSource: 100 escenarios / 1.897 comprobaciones PASS. Incluye ambos
  slots principales, suspensión, ausencia de HUD, argumentos inválidos, modos
  CPU/red y muerte reentrante durante las lecturas del dispositivo.
- Gateway i386: rutas stock/conservar, argumentos, retorno, pila, flags,
  registros y FP/SIMD PASS; llamadas nativas simuladas.
- Cuatro pruebas Python de input PASS, con las nuevas consultas por índice.
- Runtime: 217 escenarios / 49.858 comprobaciones PASS con operaciones simuladas.
- Enlace completo y cinco pruebas ELF/instalador PASS: 187 hooks y diez
  reemplazos inline, todos disjuntos; candidato con reversión exacta.

Módulo SHA256: `8af171f78a6a8906b55f058331bab50ac9fc26f7a2b4afd3a27033a99ee07893`.
Candidato local: `e467c459a7cf3a4041f4a3831c1a3a99d9afb750e54469b2c6f37b6d6e8d0107`.
No instalado en Steam ni publicado como EXE.

## Pendiente

Esto elimina el borrado auditado; no demuestra aún todo el uso de teclado/ratón
más mando. Faltan revisar el cambio de dispositivo en 02DB3210 y los ajustes
por jugador. Otras consultas/setters indexados entre 02DAE020 y 02DAEF80 todavía
usan +970; varios son rutas de opciones, que deben conectarse al dueño del menú
antes de cambiar su comportamiento. El filtro final de hit-test de ratón en
02DB54E3..55F1 opera siempre sobre seis words del pad0; tampoco se ha modificado.
Continúan pendientes acciones completas, scripts, cámaras, Genesis y el resto
del objetivo. Ninguna prueba sintética se presenta como validación jugando.
