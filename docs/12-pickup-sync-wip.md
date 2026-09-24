# 12 — Pickup sync / inventario P2 — WIP exacto al cierre del chat

Build de referencia: **BioRevHD 30-Enero-2013.exe**.

Este documento guarda el punto exacto donde quedó la investigación cuando la interfaz cortó la respuesta por tiempo.

## Objetivo

Cerrar el ciclo completo de inventario del partner local:

```text
Pad 2 controla Sub0
 -> Sub0 recoge item/munición/hierba
 -> el juego identifica al actor correcto
 -> el item termina en el cBioItemPack / bag np del partner
 -> sin depender obligatoriamente de una sesión de red
```

Ya está confirmado por otros documentos que:

- Sub0 exacto está en ThinkMode::Pad en v10;
- usa PadData[1];
- AddWeapon/ClearWeapon/NpcSetWeaponSlot/NpcResetWeaponSlot/NpcChangeEquipSlot aceptan categoría `np`;
- esas rutas alcanzan `uNpc+0x1524 -> cBioItemPack`;
- sItemBoxCoop añade acceso a bag categoría `np = 0x80020000`.

Lo que falta es la **recogida física/sincronizada del item**.

---

## 1. Tipos/strings localizados

Strings internos:

```text
sItem::cNetSyncData::cPickupItemSyncData
uItem::cNetSyncData::cGetItemSyncData
sItem::cNetSyncData::cDropItemSyncData
```

También aparecen:

```text
mEnablePickup
mbPickupOnce
mIsFoundInvisibleItem
mDropItem
mDropItemID
mIsDropItem
mCharaID
mItemList
```

El archivo fuente embebido relevante:

```text
e:\bhr\source\biorevhd\prog\game\chara\item\sitem.cpp
```

---

## 2. cPickupItemSyncData — layout parcial

Se localizaron cuatro setters simples:

```text
0x0244D0F0 -> [this+0x08] = arg
0x0244D140 -> [this+0x0C] = arg
0x0244D190 -> [this+0x10] = arg
0x0244D1E0 -> [this+0x14] = arg
```

Y getters simétricos:

```text
0x02459E30 -> return [this+0x08]
0x02459E70 -> return [this+0x0C]
0x02459EB0 -> return [this+0x10]
0x02459EF0 -> return [this+0x14]
```

**No asignar nombres semánticos todavía.**

Los campos pueden contener item/chara/player/serial/flags u otra combinación. La correspondencia exacta sigue abierta.

---

## 3. Constructor / metadata relacionado

En la zona alrededor de:

```text
0x0244D050
```

se observó un constructor que instala una vtable:

```text
0x04D2F344
```

La atribución exacta a `cPickupItemSyncData` debe terminarse mediante RTTI/registro global antes de fijarla definitivamente.

El trabajo se interrumpió precisamente mientras se desmontaba la inicialización global asociada.

### Punto exacto del timeout

La interfaz mostró:

```text
Desassemblant la inicialització global a 0x49A8023
```

Por tanto:

```text
0x049A8023
```

es un punto pendiente explícito.

Objetivo al retomarlo:

- identificar qué DTI/vtable se registra;
- enlazarlo formalmente con `cPickupItemSyncData`;
- recuperar tamaño y herencia si existe;
- enlazar constructor/serializer/receiver.

---

## 4. Productor / construcción candidata

Bloque relevante:

```text
0x0244CF80
```

La función:

1. construye un objeto temporal en stack;
2. llama a varios setters;
3. usa argumentos externos;
4. finalmente pasa el objeto a una ruta de sincronización/envío.

Fragmentos relevantes observados:

```text
0x0244CF85 -> call 0x01C3072D
0x0244CF91 -> call 0x01B879ED
0x0244CF9D -> call 0x01C0D8B8
0x0244CFAA -> call 0x01C2CDA3
...
0x0244CFB9 -> call 0x01B95665
0x0244CFC0 -> call 0x01BC43D4
```

La función retorna con:

```text
ret 0x14
```

por lo que recibe cinco argumentos de stack en esta ABI.

Pendiente:

- mapear cada setter a offset +0x08/+0x0C/+0x10/+0x14;
- identificar qué argumento corresponde a item/player/chara/etc.;
- decidir si uno de los campos es playerID.

---

## 5. Receiver/callback prioritario

Thunk:

```text
0x01B9F142 -> 0x02459C20
```

La implementación `0x02459C20` es el punto de mayor prioridad.

### Flujo parcial observado

Comienza con un argumento en `[ebp+0x08]`.

Llama a varios getters/validadores y obtiene objetos asociados.

Alrededor de:

```text
0x02459D05
```

entra en una rama relevante.

Después:

```text
0x02459D31 -> call 0x01BCC327
```

El thunk `0x01BCC327` ya está identificado en las rutas PcsSub como:

```text
0x01BCC327 -> 0x01D336A0
uNpc+0x1524 -> puntero a cBioItemPack
```

Inmediatamente después:

```text
0x02459D38 -> call 0x01B8695D
```

La semántica exacta de `0x01B8695D` queda pendiente.

**Hipótesis fuerte, aún no cerrada:** esta rama del receiver está aplicando el pickup al item pack del actor correspondiente.

No marcar como confirmado hasta identificar:

- cómo obtiene el actor;
- qué campo del paquete usa para elegirlo;
- qué hace `0x01B8695D`.

---

## 6. Otros getters del receiver

Se observaron getters pequeños:

```text
0x02459B80 -> byte [obj+0x0D]
0x02459BC0 -> byte [obj+0x0E]
0x02459C00 -> dword [obj+0x10]
```

Además de los cuatro getters +0x08/+0x0C/+0x10/+0x14 descritos antes.

Esto sugiere que existen estructuras relacionadas pero no necesariamente el mismo tipo.

No mezclar offsets de objetos distintos sin cerrar RTTI.

---

## 7. Campos de uItem observados en productor/sender

Grupo de getters alrededor de `0x024372D0`:

```text
0x02437350 aprox -> [uItem+0xF28]
0x02437390 aprox -> [uItem+0xF20]
0x024373D0 aprox -> [uItem+0xF24]
0x024374D0 aprox -> byte [uItem+0xF3C]
0x02437510 aprox -> byte [uItem+0xF3D]
0x02437550 aprox -> [uItem+0xF40]
```

También existe inicialización/limpieza en:

```text
uItem+0xF14
```

No poner nombre final a estos campos sin seguir metadata o consumidores.

---

## 8. Otra ruta importante: 0x02460900

Bloque:

```text
0x02460900
```

Contiene varias llamadas al getter real de playerID:

```text
0x01C53859 -> 0x027F1200
```

Ejemplos observados alrededor de:

```text
0x02460AAA
0x02460ABD
0x02460AD5
0x02460AED
0x02460B2C
```

También escribe:

```text
uItem+0xF14 = 2
```

en torno a:

```text
0x02460B52
```

Y accede repetidamente a:

```text
0x01BCC327 -> uNpc+0x1524 -> cBioItemPack
```

Esta función es candidata fuerte a formar parte del flujo de “item recogido/adjudicado”, pero todavía no está nombrada formalmente.

---

## 9. Strings útiles para cerrar semántica

Dentro del EXE aparecen:

```text
charaID
itemID
playerID
mPlayerID
[cNetSyncParamSerial] PlayerID=%d, SerialID=%d(%s:%s)
sItem::createUnit : invalid charaID
sItem::createUnitWeapon : invalid charaID
sItem::createUnitBullet : invalid charaID
uItem::cNetSyncData::cGetItemSyncData
cBioItemPack::addItem : invalid charaID
cBioItemPack::addItem : invalid randomBullet charaID
```

Conviene cruzar xrefs de estos strings con `0x0244CF80`, `0x02459C20` y `0x02460900`.

---

## 10. Próximo procedimiento recomendado

No crear parche todavía.

Orden exacto:

1. volver a `0x049A8023` y cerrar DTI/registro global;
2. identificar formalmente la vtable `0x04D2F344`;
3. mapear setters/getters de `cPickupItemSyncData`;
4. desmontar completo `0x0244CF80`;
5. desmontar completo `0x02459C20`;
6. identificar `0x01B8695D`;
7. seguir cómo se obtiene el actor/owner del pickup;
8. demostrar si el actor `np/Sub0` llega a `uNpc+0x1524 -> cBioItemPack`;
9. comprobar si la ruta depende de Network o también funciona offline;
10. solo entonces decidir si hace falta v11.

---

## 11. Criterio de éxito

Se considerará cerrado cuando esté demostrado uno de estos dos casos:

### Caso A — no hace falta parche

```text
Sub0 local
 -> FsmPickupItem / sItem
 -> actor np correcto
 -> cBioItemPack propio
 -> sItemBoxCoop np bag
```

sin dependencia de Network.

### Caso B — hace falta hook mínimo

Si el código correcto existe pero solo se ejecuta cuando hay sincronización de red:

```text
gLocalCoopActive
 -> reutilizar localmente el mismo receiver/aplicador
 -> sin falsificar GameMode global
 -> sin activar Network(3)
```

La preferencia es siempre reutilizar la ruta nativa más corta.
