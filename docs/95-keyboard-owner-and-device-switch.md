# Teclado de J1 y conservación del mando de J2

08/10/2026. Continúa PR66, `289b4895583f2885b1b89fe11a24b1e613013afb`.
January 30 2013. No se ha abierto ni ejecutado el juego.

PR64 conservaba el bloque físico distinto a `mStartPadNo` durante la limpieza
del teclado. Con `mStartPadNo=1`, eso todavía permitía que el teclado borrase y
reescribiese la entrada destinada a J2. Además, el cambio automático de dispositivo
mezclaba una consulta de conexión del mando principal con botones del slot0.

Las dos llamadas nativas de actualización ahora admiten un ámbito local con
propiedad observada. Dentro de él el cambio de dispositivo y la síntesis de
teclado pertenecen al slot lógico 0, mientras el slot1 conserva su estado físico.
`mStartPadNo` sigue intacto. Fuera de esa admisión se mantiene el comportamiento
stock, incluidas sus limitaciones.

## Integración

| Llamada de sGamePad::update | Destino nativo | Función |
|---|---|---|
| `02DAC787` | `01BF908E -> 02DB3210` | Cambio automático de dispositivo |
| `02DAC8A1` | `01C2742F -> 02DB3590` | Síntesis de teclado/ratón |

La llamada intermedia `02DAC78F -> 01C57120 -> 02DB2E30` ajusta el modo de ratón;
no es la función de síntesis y no se sustituye por ella.

Se dirigen 196 cargas de `+970`: 190 en la síntesis, dos en la elección de
dispositivo, una en la consulta de conexión y tres en la restauración del layout.
Cinco variantes de gateway conservan registros, flags, pila y estado FP/SIMD,
cambiando únicamente el registro destinatario de cada MOV. El inventario completo
es `tools/input_owner_sites.py`; los bytes esperados se comprueban al instalar.

Los doce sitios de setters introducidos en PR66 consultan primero el ámbito de
entrada y después el de opciones. Una aplicación de opciones conserva prioridad
para su dueño. Fuera de ambos ámbitos, esas cargas usan el valor stock.

Antes de elegir dispositivo se ejecuta para el slot1 la restauración nativa
`01C3DD97 -> 02DAF2E0`, con su contrato thiscall `ret 4`. Esa función solo cambia
el layout cuando es 6, recuperando el valor anterior. Así J2 no conserva un layout
de teclado heredado de antes de unirse a la sesión. Luego el ámbito vuelve al
slot0 para la elección y para escribir la entrada del teclado.

La limpieza interceptada en PR64 conserva ahora el otro bloque respecto al slot
fijado por este ámbito, no respecto al mando principal global. Los seis filtros
de botones del final de la síntesis ya escriben explícitamente en slot0 y encajan
con esta elección. No se omiten las limpiezas generales de foco/pausa de la
actualización principal.

## Admisión

LifetimeSource comprueba el hilo propietario, la pareja de actores observados,
PCS, seriales, modo local, singleton/vtable del mando y lecturas repetidas de su
índice principal. No requiere HUD listo. Cada llamada síncrona conserva su slot
hasta volver, aunque se revoque el actor durante un callback; la siguiente llamada
necesita una nueva admisión. Se rechaza la reentrada en ese mismo hilo. No se
afirma soporte para carreras arbitrarias entre hilos o destrucción concurrente
del propio sGamePad.

## Comprobaciones

- 214 testigos fijados por SHA; 196 registros/offsets contrastados con los cinco
  gateways y confinados a las cuatro rutinas auditadas.
- LifetimeSource: 134 escenarios / 2498 comprobaciones. Incluye ambos índices
  principales, restauración de layout6, revocación, reentrada, hilo distinto y
  transporte sintético poll/clear/escritura de teclado con J2 conservado.
- Grafo Runtime: 222 escenarios / 49937 comprobaciones.
- 21 gateways x86 probados con callbacks sintéticos; selección de opciones tiene
  prioridad sobre la selección de teclado. No se carga código del motor.
- Módulo freestanding enlazado; cinco tests de instalador. Copia acumulativa con
  425 hooks y diez reemplazos inline; reversión exacta a la base aceptada.

Módulo: `698232ce908218b3842535d269f82cc6c52da19244c8d3fd43fbd17494ee1a0c`.
Copia local: `9f823502d3a40b461cebc16b690f0bcbffb809f2f4012fb027b0858777f19425`.
No instalada en Steam ni distribuida.

## Límites

Esta corrección presupone el slot físico/lógico 1 para J2. No reasigna al slot1
un único mando físico que el backend haya conectado al slot0, ni demuestra
reconexión de dispositivos. El coste de los gateways no se ha medido jugando.
El modo global de ratón, avisos de dispositivo y demás UI compartida necesitan
seguir auditándose. Tampoco valida el resultado jugando del informe de J2 inmóvil.
Continúan pendientes acciones completas, Genesis, HUD, QTE, muerte/checkpoints,
cámaras forzadas y escenas sin compañero.
