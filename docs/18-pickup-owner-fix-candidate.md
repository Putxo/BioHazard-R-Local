# 18 — Candidato experimental: corrección del owner de pickup

Fecha: 2026-09-25. Continúa la evidencia de `docs/17-actioncommand-owner-correction.md`.

## Cambio implementado

Se construyó y verificó **pickup-owner-fix-experimental** sobre la v14 paralela exacta, sin renumerarla ni sustituir su estado canónico automáticamente.

SHA-256 del candidato:

```
e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a
```

Tamaño: **60.748.800 bytes**. Formato: PE32 i386.

Base v14 preservada:

```
73fe1255697c47a624025ba40b33bab8df5812e76021bdc2d9acdeec3daf56ea
```

La misma salida se obtiene tanto desde la v14 exacta como desde el v13 adjunto. En el segundo caso, el constructor reproduce primero la v14 en memoria y exige su hash antes de continuar.

| VA | Corrección | Bytes efectivamente distintos |
|---|---|---:|
| `0x024605D0` | Instala el selector en el callback que uItem realmente enlaza a su ActionCommand | 46 |
| `0x026D7AD0` | Restaura el callback común de uObjModel a su cuerpo stock | 45 |

**Total: 91 bytes**, limitados a estos dos cuerpos de 46 bytes. El selector nuevo tiene 37 bytes de instrucciones y nueve bytes de padding CC.

Regla implementada:

```
owner nulo -> 0
modo local inactivo -> 0
tracker Sub0 nulo -> 0
uItem+0xF48 == Sub0 exacto -> 1
cualquier otro candidato -> 0
```

Se conserva `ret 4`. No se modifica el índice de red, el predicate global isPlayer ni ThinkMode. Los dos cambios de la v14 de puertas y el getter de sGamePad corregido por v12 permanecen intactos.

## Verificación efectuada

`patches/test_pickup_owner_fix.py` pasó **44 comprobaciones**. Estas incluyen identificación de owners por RTTI, binding de delegates, hashes, exclusividad de los rangos modificados, restauración del callback de uObjModel, preservación de v14 y reversión byte por byte al v14 exacto.

Ocho de las comprobaciones interpretan las instrucciones del selector extraídas del candidato: owner nulo, local inactivo, tracker nulo, candidato nulo, Sub0 exacto, P1, otro NPC y flag local no cero. Se verifican el retorno, la conservación de ECX, las lecturas de memoria efectuadas y la limpieza modelada de `ret 4`.

El intérprete es deliberadamente limitado a esas instrucciones; **no es un emulador general ni ejecuta el motor del juego**.

Además se comprobaron cuatro rechazos de CLI: salida igual a entrada, salida existente, informe igual a entrada y build original no parcheada como entrada. Todos devolvieron código 2 sin modificar la entrada ni producir un EXE de error.

Código fuente publicado y copia local probada coinciden por hash de blob:

```
builder 28e8c9374b217f3071e5b2046d387dbeb1446f46
tests   8afd5c4ca2a8f0ab7bd9e453bfff3195f75b87d3
```

Informe: `research/reports/pickup-owner-fix-validation.json`.

## Límites y siguiente punto exacto

**No se ha ejecutado gameplay.** Este candidato corrige una asignación de callback demostrablemente equivocada; no acredita que el cooperativo esté terminado ni que todas las interacciones funcionen.

La afirmación histórica de que v12 ya había cerrado el botón de pickup de Sub0 se sustituye por esta evidencia: v12 parcheaba el selector de uObjModel; el de uItem permanecía stock. Se conservan los documentos antiguos para trazabilidad, pero no deben usarse como prueba en contra de la cadena de RTTI/delegate verificada.

Siguen abiertos los gates Self/pl de door_gimmick. El siguiente trabajo no es rehacer el pickup: es coordinar el candidato de proximidad en `0x02590280`, el serial guardado en `WaitState+0x10`, los cinco usos del finder pl-only y el owner del ActionCommand de uObjModel. En esa clase `+0xF48` NO es el puntero de pickup de uItem; no se volverá a reutilizar ese layout sin evidencia.

No se suben EXE, DLL, assets ni objetos de compilación a GitHub. El repositorio contiene el constructor, assembly original de la corrección, tests, metadatos y evidencia.
