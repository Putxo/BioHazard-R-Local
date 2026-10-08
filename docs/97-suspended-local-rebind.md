# Conservar el mando local tras un reenlace durante suspensión

08/10/2026. Continúa PR68, merge `61b704d67b2916a9fc5eeba250fc4ab7d804ae39`,
20 workflows PASS. Original January SHA256
`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.

Se reprodujo un fallo con memoria sintética y el LifetimeSource real:
una pareja local previamente enlazada devuelve Pad1 ante una petición Cpu3;
poner temporalmente al compañero en Invalid0 mantiene esa propiedad; reenlazar
el mismo PCS al mismo actor durante Invalid0 la pierde. La siguiente petición
Cpu3 devuelve 3. Esta secuencia es un fallo confirmado del código, pero no se
ha confirmado que fuese la secuencia de la partida del propietario.

## Corrección en las dos capas

El binder de la base limpiaba el indicador local antes del reenlace y solo lo
restauraba para modo 1. Ahora admite modo 0 o 1 en la ruta de **conservación**,
siempre que coincidan el indicador previo, el tracker previo y el actor que ya
estaba en `PCS+44`. Un actor inicialmente Invalid0 no se convierte. Network2,
modos desconocidos, otro actor y enlaces sin propiedad previa se rechazan.
La ruta inicial Cpu3 -> Pad1 conserva su setter nativo completo.

La modificación de ensamblador es JNE -> JA después de comparar con 1. La
función sigue midiendo 108 bytes y mantiene llamadas, continuaciones y pila.
El modo 0 no se escribe ni se transforma durante el reenlace. La ruta de
conservación mantiene su llamada existente a ensure_split; no añade una nueva
política para las cámaras de cutscenes.

LifetimeSource conserva la Role previamente observada si el mismo PCS y actor
están en Invalid0 y la pareja sigue cumpliendo las comprobaciones de identidad.
Comparte con local_think_mode la validación de tokens/generaciones, seriales,
VTs, ambos `PCS+44`, tracker, indicador local y lecturas repetidas de modo.
Se verifica la revisión antes de publicar el resultado. Un evento de muerte
durante una lectura no puede restaurar una Role ya revocada.

Conservar propiedad no activa gameplay: capture continúa exigiendo Pad1, igual
que el callback de guion para el actor de J2. Al reanudar, la petición Cpu3 del
mismo actor puede convertirse a Pad1 mediante el wrapper existente. La primera
adopción de un actor Invalid0, la recuperación tras Network y los cambios de
identidad no se habilitan por esta conservación.

## Binario y entrega

Las dos VTs NPC/Player tienen `01BB8B60 -> 0278CC40` en el virtual `+110`.
La ruta nativa de `027EDC00` incluye una petición de modo 0 en `027EDD27` y su
dispatch en `027EDD31`. El binder `02DF4F30` se llama desde setters de serial,
asignación y override; no se debe interpretar como una conversión por frame.
El auditor de ThinkMode fija ahora 26 testigos al SHA del original.

`check_local_input.py` distingue las bases Cpu2 históricas, las Cpu3 anteriores
y las que conservan el reenlace suspendido. El instalador acumulativo rechaza
las anteriores con una indicación de reconstrucción; también verifica de nuevo
el binder tras instalar el runtime. No acepta una coincidencia parcial de bytes.

La cadena completa desde el original se ha reconstruido con LLVM 22.1.8 y
coincide byte a byte con una construcción independiente desde el v13 existente.
El perfil de entrega nuevo registrado es LLVM. Los perfiles GNU históricos
siguen identificados como históricos; un nuevo hash de entrega GNU requiere
reconstrucción verificable y no se presupone equivalente.

| Artefacto local | SHA256 |
|---|---|
| Routing antes de serial | `170ce3fa623fa728728bef6d80974578e73134f5eb77decb37b01d1cd3291ee1` |
| Base acumulativa | `cd15455ad9e854ef5ef19ed1bdbfc3aa43a9c4c724ec97c7f8b61618840ece1f` |
| Runtime ELF | `644639afb54652d2a2c0fb4519c5ce7ed761ea1c92afd950e82c0aecf7da87b4` |
| Copia experimental | `d86c63a781d904e8fee05d83e514c56b81a3456a397cadf3e2bf22595751f58a` |

## Pruebas y límites

- LifetimeSource: 163 escenarios / 3.017 comprobaciones. Reenlace simple y
  anidado, suspensión de Main/Sub, ausencia de HUD, rechazo por cambio de vida,
  serial, tipo, PCS o modo, y muerte durante lecturas.
- Runtime: 234 escenarios / 50.264 comprobaciones, incluida suspensión,
  reenlace, reanudación y creación posterior del HUD.
- Bridges compilados: 28 escenarios / 435 comprobaciones; solo se interpretan
  instrucciones originales del mod, con llamadas nativas sustituidas por stubs.
- Inspector: 100 comprobaciones contra el módulo compilado, incluyendo
  versiones anteriores y corrupción; seis pruebas del instalador/ELF.
- 426 hooks y diez reemplazos inline; reversión exacta de la copia generada.

El juego no se ha abierto ni ejecutado. No se ha instalado nada en Steam ni
publicado ejecutables o assets. Sigue pendiente la comprobación jugando del
movimiento y acciones del J2. Esto no completa el cooperativo, las transiciones
de cutscene/checkpoint ni la adquisición inicial fuera del binder auditado.
