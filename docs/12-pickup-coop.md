# 12 — Pickup cooperativo y cPickupItemSyncData

Build principal: **BioRevHD 30-Enero-2013.exe**.

Este documento continúa la investigación de inventario/equipamiento sobre **v10**. Registra únicamente hallazgos estáticos confirmados en esta conversación.

## 1. Recuperación del punto donde se cortó la respuesta

La dirección que aparecía en la captura del turno interrumpido:

```text
0x049A8023
```

pertenece a la inicialización global del DTI de **uItem**.

En la zona `0x049A7FF0` se observa el registro de la clase usando la string:

```text
uItem
VA 0x04D30838
```

La siguiente string relevante es:

```text
uItem::cNetSyncData::cGetItemSyncData
VA 0x04D30850
```

Por tanto el análisis que quedó cortado seguía dentro de la ruta de objetos/pickup; no era una rama distinta.

---

## 2. cPickupItemSyncData

Nombre interno:

```text
sItem::cNetSyncData::cPickupItemSyncData
VA 0x04D2FC5C
```

DTI global:

```text
0x05578DF0
```

La inicialización global alrededor de `0x049A5300` registra el tipo con tamaño:

```text
0x1C bytes
```

Factoría:

```text
0x02443DA0
```

- reserva `0x1C`;
- llama al constructor:
  `0x01C89A3A -> 0x0245A120`.

Constructor `0x0245A120`:

```text
vtable 0x04D2F344
+0x08 = 0x80000000
+0x0C = 0x80000000
+0x10 = 0xFFFFFFFF
+0x14 = 0x80000000
+0x18 = 0
```

## 3. Setters/getters del paquete

Setters confirmados:

```text
+0x08 -> thunk 0x01C672D7 -> 0x0244D0F0
+0x0C -> thunk 0x01C3072D -> 0x0244D140
+0x10 -> thunk 0x01B879ED -> 0x0244D190
+0x14 -> thunk 0x01C0D8B8 -> 0x0244D1E0
+0x18 -> thunk 0x01C2CDA3 -> 0x0244D230   (BYTE)
```

Getters confirmados:

```text
+0x08 -> thunk 0x01C008CA -> 0x02459E20
+0x0C -> thunk 0x01BEF3C2 -> 0x02459E60
+0x10 -> thunk 0x01C936F2 -> 0x02459EA0
+0x14 -> thunk 0x01BB1540 -> 0x02459EE0
+0x18 -> thunk 0x01B9072D -> 0x02459F20
```

Los nombres semánticos exactos de los cinco campos todavía no se fijan; se documentan por offset hasta cerrar todos sus productores/consumidores.

---

## 4. Sender nativo de pickup

Rutina:

```text
0x0244CF30
thunk 0x01BD0B93
```

Construye un `cPickupItemSyncData` local, rellena sus cinco campos y lo envía por el canal de sincronización del `sItem/uItem`.

Tipo/mensaje observado al enviar:

```text
0x10
```

Callers directos localizados:

```text
0x02434322
0x02460890
0x02460B3E
```

En los callers `0x02460890` y `0x02460B3E`, los campos proceden del mismo `uItem`.

Mapeo de getters de ese `uItem`:

```text
packet +0x08
  0x01C53859
  getter de ID usado también en otras rutas de objetos/jugador

packet +0x0C
  0x01BEDBB7 -> 0x01CB7610
  retorna [this+0xE3C]

packet +0x10
  0x01C69771 -> 0x02437330
  retorna [this+0xF28]

packet +0x14
  0x01C1729B -> 0x02437530
  retorna [this+0xF40]

packet +0x18
  0x01C1A1D0 -> 0x024374F0
  retorna BYTE [this+0xF3D]
```

Esto demuestra que el paquete describe el objeto recogido y su estado asociado; todavía no se atribuyen nombres finales a esos cinco miembros.

---

## 5. Dispatcher de red y handler receptor

Dispatcher entrante:

```text
0x02449820
```

Dentro de esa rutina, la rama de `cPickupItemSyncData` aparece alrededor de:

```text
0x024499AF
```

y compara el DTI entrante contra:

```text
0x05578DF0
```

Al reconocer el paquete instala/llama el delegate:

```text
0x01B9F142 -> 0x02459C20
```

Por tanto:

```text
0x02459C20
```

es el handler receptor de `cPickupItemSyncData`.

## 6. Consumo inicial del paquete en 0x02459C20

El handler recibe:

```text
arg1 = cPickupItemSyncData*
```

y consume los cinco campos a través de sus getters.

Entre las rutas observadas:

```text
+0x0C -> 0x01BEF3C2
+0x18 -> 0x01B9072D
+0x14 -> 0x01BB1540
+0x08 -> 0x01C008CA
+0x10 -> 0x01C936F2
```

Después realiza búsquedas/validaciones de objetos antes de aplicar el pickup.

La semántica exacta de cada campo y, sobre todo, qué actor/inventario recibe finalmente el objeto siguen bajo análisis.

---

## 7. Rutina central de pickup

Se identificó además una rutina grande:

```text
0x0244D3D0
thunk 0x01B880E6
```

retorna con:

```text
ret 0x24
```

por lo que consume un bloque amplio de argumentos relacionado con la operación de pickup.

Observaciones iniciales:

- inspecciona un objeto/target recibido;
- obtiene IDs mediante `0x01C53859`;
- ejecuta tests de categoría;
- entra en lógica de jugador/actor e inventario;
- es una candidata prioritaria para demostrar cómo se enruta el pickup hacia Main o partner.

Todavía no se atribuyen nombres a sus argumentos hasta reconstruir su flujo completo.

---

## 8. Evidencia textual muy prometedora pendiente de xref

En .rdata aparece literalmente una condición del código fuente:

```text
(game_id::Category::isPlayer( target->getDTI().getID() ) ||
 game_id::Category::isNpc( target->getDTI().getID() ))
```

Esta string constituye una pista directa de que alguna ruta de pickup admite tanto categoría player como NPC.

El siguiente paso es localizar su xref exacto y demostrar si pertenece a la misma ruta `FsmPickupItem/0x0244D3D0`.

No se considera aún prueba suficiente hasta seguir el xref ejecutable.

---

## Estado

**CONFIRMADO:**

- existe un paquete de sincronización de pickup específico;
- tiene factoría, RTTI, layout y sender reales;
- existe un handler receptor real;
- el sender se alimenta del estado del `uItem`;
- el juego contiene lógica de pickup que contempla categorías player/NPC.

**PENDIENTE INMEDIATO:**

1. localizar el xref de la condición Player || Npc;
2. reconstruir `0x0244D3D0`;
3. identificar target/receptor del pickup;
4. seguir la llamada final hacia `cBioItemPack/sItemBoxCoop`;
5. comprobar si Sub0 local usa esa misma ruta sin parche adicional.
