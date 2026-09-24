# 12 — Pickup cooperativo y cPickupItemSyncData

Build principal: **BioRevHD 30-Enero-2013.exe**.

Este documento continúa la línea canónica vigente: **v10 = v9 + selección local de sItemBoxCoop**.

## 1. Corrección del punto donde se cortó el turno

Durante el turno que la interfaz interrumpió se estaba desmontando la inicialización global alrededor de:

```text
0x049A8023
```

La revisión completa corrige la atribución:

- alrededor de `0x049A8022/0x049A8027` se registra/inicializa el DTI global asociado a la string literal **`"uItem"`**;
- el DTI de **`sItem::cNetSyncData::cPickupItemSyncData`** se inicializa en otra rutina global alrededor de **`0x049A5300`**.

La dirección del turno interrumpido era por tanto una pista cercana de `uItem`, no el registro exacto de `cPickupItemSyncData`.

## 2. Tipo de sincronización de pickup

String:

```text
sItem::cNetSyncData::cPickupItemSyncData
VA 0x04D2FC5C
```

RTTI decorado:

```text
.?AUcPickupItemSyncData@cNetSyncData@sItem@chara@game@app@@
```

DTI global observado:

```text
0x05578DF0
```

Tamaño del objeto:

```text
0x1C bytes
```

Constructor real:

```text
0x0245A120
```

Factoría/DTI:

```text
0x02443DA0
 -> thunk 0x01C89A3A
 -> 0x0245A120
```

Vtable del objeto:

```text
0x04D2F344
```

## 3. Layout observado

El constructor inicializa:

```text
+0x08 = 0x80000000
+0x0C = 0x80000000
+0x10 = 0xFFFFFFFF
+0x14 = 0x80000000
+0x18 = 0
```

Getters:

```text
0x02459E20 -> +0x08
0x02459E60 -> +0x0C
0x02459EA0 -> +0x10
0x02459EE0 -> +0x14
0x02459F20 -> +0x18 byte
```

Setters usados por el emisor:

```text
0x0244D0F0 -> +0x08
0x0244D140 -> +0x0C
0x0244D190 -> +0x10
0x0244D1E0 -> +0x14
0x0244D230 -> +0x18 byte
```

Thunks:

```text
0x01C672D7 -> 0x0244D0F0
0x01C3072D -> 0x0244D140
0x01B879ED -> 0x0244D190
0x01C0D8B8 -> 0x0244D1E0
0x01C2CDA3 -> 0x0244D230
```

## 4. Emisor nativo

Rutina:

```text
0x0244CF30
thunk 0x01BD0B93
```

Construye un `cPickupItemSyncData` temporal y escribe sus cinco campos desde cinco argumentos.

Después lo envía por:

```text
0x01BC43D4
```

con el canal/mensaje:

```text
0x10
```

Callsites localizados:

```text
0x02434322
0x02460890
0x02460B3E
```

### Callsite 0x02460890

Los cinco argumentos se extraen del actor actual:

```text
arg1:
  0x01C53859 -> 0x027F1200
  ID del actor/player, con override uPlayer+0xE38 o ID derivado

arg2:
  0x01BEDBB7 -> 0x01CB7610
  [actor+0xE3C]

arg3:
  0x01C69771 -> 0x02437330
  [actor+0xF28]

arg4:
  0x01C1729B -> 0x02437530
  [actor+0xF40]

arg5:
  0x01C1A1D0 -> 0x024374F0
  byte [actor+0xF3D]
```

En el mismo bloque aparecen strings locales:

```text
dropVal
dropID
param
```

Se conservan como pista para nombrar los otros campos, pero todavía no se les asigna semántica exacta sin cerrar el flujo.

## 5. Dispatcher de recepción

El dispatcher de `sItem::cNetSyncData` compara el DTI entrante con:

```text
0x05578DF0
```

alrededor de:

```text
0x024499AF
```

Cuando es `cPickupItemSyncData`, crea el delegate:

```text
0x01B9F142 -> 0x02459C20
```

## 6. Handler de pickup remoto 0x02459C20

El handler lee los cinco campos mediante los getters del paquete.

El dato decisivo es `+0x08`.

El sender llena `+0x08` con:

```text
0x027F1200
```

que es el mismo getter de ID de actor/player ya identificado en la investigación de ItemBox.

El receptor vuelve a obtener ese campo mediante:

```text
0x01C008CA -> 0x02459E20
```

y lo usa para resolver un objeto/actor.

En la rama válida:

```text
packet +0x08
 -> lookup/resolución de actor
 -> actor resuelto
 -> 0x01BCC327
 -> 0x01D336A0
 -> actor/uNpc + 0x1524
 -> cBioItemPack
```

Por tanto la recepción cooperativa de un pickup **no aplica el objeto indiscriminadamente al inventario global/P1**: identifica un actor mediante el ID serializado y llega al pack asociado a ese actor.

Esto encaja con el diseño local actual:

```text
Sub0 sigue siendo un actor uNpc
 -> posee su cBioItemPack
 -> ItemBoxCoop tiene rama/bag np
 -> las acciones PcsSub ya aceptan np
 -> el pickup sincronizado también resuelve actor antes de tocar el pack
```

## 7. Otros campos todavía en investigación

En `0x02459C20`:

```text
+0x0C -> getter 0x02459E60
+0x10 -> getter 0x02459EA0
+0x14 -> getter 0x02459EE0
+0x18 -> getter 0x02459F20
```

Se usan para resolver el item/drop, parámetros auxiliares y condiciones de aplicación.

Todavía no se renombran hasta cerrar sus consumidores uno por uno.

## Estado

**CONFIRMADO estáticamente:**

- existe un paquete específico de pickup cooperativo;
- contiene el ID del actor en +0x08;
- el receptor resuelve ese actor;
- la ruta termina en el `cBioItemPack` del actor resuelto.

**PENDIENTE:**

1. nombrar con precisión +0x0C/+0x10/+0x14/+0x18;
2. demostrar si la interacción local de Sub0 genera directamente la misma ruta sin necesitar transporte de red;
3. comprobar munición/hierbas/objetos especiales por los mismos caminos;
4. determinar si v10 necesita un hook adicional o si las rutas actor-local ya bastan.
