# Mando del actor en uPcsInput

08/10/2026. Continúa PR67, integrado en `2b243d8af3814caa6aa58eeb1d70fc30b053500f` con 21 workflows PASS.
Original January SHA256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.

El tercer callback de miembro, `uPcsInput::02DEE8D0`, devolvía siempre cero.
Los dos callbacks de FSM ya corregidos no cubrían esta clase. El nuevo callback
devuelve uno cuando el scheduler del input pertenece exclusivamente a un PcsSub
del Sub0 local observado, con el mismo actor y serial. No depende de que el HUD
haya terminado de crearse. Las demás situaciones conservan el cero nativo.

## Relación comprobada en el binario

- Constructor `02DEE4C0`: guarda ECX, instala VT `04E15EF4` y registra
  `01C0758A -> 02DEE8D0` con ese mismo objeto en `02DEE53C`. El comando se
  instala en `this+50` mediante `02DEE629`.
- El constructor base inicializa `uPcs+30` a cero en `02DEE727`.
  `02DEDF30` busca un scheduler que contenga el input: llama al getter de hijo
  en `02DEDFF4`, compara el hijo con this y guarda el scheduler en `02DEE004`.
- Inicialización `02DEDC60`: resuelve ese padre y consulta `sPcsManager`.
  El resultado guardado en `uPcs+34` es un **grupo de guion**, no un mando.
  El router no usa ese campo como índice de jugador.
- `sPcsManager` tiene VT `04E13248` y singleton `05566570`. Su resolver nativo
  `02DD0C80` compara el scheduler de 16 Main (array `+23C`, stride `1070`)
  y 17 Sub por cada uno de los 16 grupos (punteros `+240`, stride `1080`).
  El getter `02DD0E40` devuelve `cFsmActionPcs+F90`.
- Se exigen las VTs exactas Main `04DCBFAC` y Sub `04DCF41C`.
  Actor `Sub+1078` y serial `Sub+1074` siguen el contrato de docs/21 y docs/22.

## Alcance de la implementación

El router lee las tablas en el hilo propietario, rechaza matches Main o múltiples
Sub y vuelve a comprobar los punteros de las tablas, el parent del input, el
registro seleccionado y el actor/serial/epoch. No llama a operaciones del motor,
no modifica tablas y no conserva una asociación entre llamadas. Reentrada,
tablas parciales, lectura fallida o pérdida del propietario devuelven cero.

El hook E9 de nueve bytes sustituye la función completa. Reenvía ECX como
argumento del resolver y mantiene el `ret 4` original; el argumento original no
se utiliza. El runtime y el instalador incluyen este hook: 426 en total, más
diez reemplazos inline existentes.

Esto depende de que el vínculo padre mantenido por el motor siga siendo válido.
No añade observadores de lifetime de schedulers, ni demuestra todas las rutas de
reparenting/reutilización o ejecución desde workers. No resuelve por sí solo
QTE, prompts compartidos, historial de comandos o gates globales. Recorre hasta
288 FSM por consulta; su coste jugando está pendiente de medición. No se ha
abierto ni ejecutado el juego, ni instalado nada en Steam. El movimiento de J2
y el cooperativo completo siguen pendientes de validación jugando.

## Comprobaciones

- Componente con memoria sintética: 1.224 escenarios / 1.784 comprobaciones;
  incluye cada grupo/slot, ambigüedad, fallo de cada lectura y reasignación.
- Runtime compuesto: 231 escenarios / 50.064 comprobaciones, incluyendo
  funcionamiento antes de crear HUD, pérdida del actor y modos CPU/Network.
- ABI x86: ECX, resultado 0/1, registros no volátiles y limpieza exacta de pila.
  Se ejecuta solo código original del mod y stubs sintéticos.
- Auditoría del original: 28 testigos de instrucciones y cuatro tipos RTTI.
- Contrato de instalador: dos pruebas; módulo/instalador: cinco pruebas.
- Compilación LLVM 22.1.8 y reversión exacta de la copia acumulativa.

Módulo SHA256: `d2afa835d86c66ddcc1194aa5adb9094dbf8ac0794bd9a18b03d165e27762737`.
Copia experimental SHA256: `82a72094a10edeba17e9d579e0bd5e7c10daae53b971cfe6e9e677b98d8675c1`.
Solo se publican fuente, pruebas y documentación; ningún ejecutable del juego.
