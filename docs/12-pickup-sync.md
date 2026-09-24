# 12 — Pickup cooperativo: cPickupItemSyncData y bloqueo local de Sub0

Build principal: **BioRevHD 30-Enero-2013.exe**.

Base experimental vigente al iniciar este bloque: **v10 CLEAN SPLIT COOP ITEMBOX**.

## 1. cPickupItemSyncData

RTTI/string interna:

```text
sItem::cNetSyncData::cPickupItemSyncData
VA string ~0x04D2FC5C
```

Registro global alrededor de `0x049A5300`:

```text
size = 0x1C
DTI global = 0x05578DF0
```

Constructor real:

```text
0x01C89A3A -> 0x0245A120
vtable = 0x04D2F344
```

Inicialización observada:

```text
+0x08 = 0x80000000
+0x0C = 0x80000000
+0x10 = -1
+0x14 = 0x80000000
+0x18 = 0
```

Setters de payload:

```text
0x01C672D7 -> 0x0244D0F0 -> +0x08 DWORD
0x01C3072D -> 0x0244D140 -> +0x0C DWORD
0x01B879ED -> 0x0244D190 -> +0x10 DWORD
0x01C0D8B8 -> 0x0244D1E0 -> +0x14 DWORD
0x01C2CDA3 -> 0x0244D230 -> +0x18 byte
```

## 2. Sender nativo

Sender:

```text
0x01BD0B93 -> 0x0244CF30
```

Construye el paquete y lo envía mediante el net-sync de Item.

Callers observados:

```text
0x02434322
0x02460890
0x02460B3E
```

En la ruta de pickup alrededor de `0x02460854..0x02460890`, los cinco valores se obtienen del actor/item actual:

```text
packet +0x08:
  0x01C53859 -> 0x027F1200
  ID lógico/playerID/chara-style ID del actor

packet +0x0C:
  0x01BEDBB7 -> 0x01CB7610
  devuelve [object+0xE3C]

packet +0x10:
  0x01C69771 -> 0x02437330
  devuelve [object+0xF28]

packet +0x14:
  0x01C1729B -> 0x02437530
  devuelve [object+0xF40]

packet +0x18:
  0x01C1A1D0 -> 0x024374F0
  devuelve byte [object+0xF3D]
```

No se asignan nombres semánticos a `+0xF28/+0xF40/+0xF3D` hasta encontrar metadata adicional.

## 3. Receptor nativo

Dispatcher de red alrededor de:

```text
0x02449820
```

compara el DTI entrante con:

```text
cPickupItemSyncData DTI 0x05578DF0
```

y deriva al handler:

```text
0x01B9F142 -> 0x02459C20
```

Getters del paquete usados por el receptor:

```text
0x02459E20 -> +0x08
0x02459E60 -> +0x0C
0x02459EA0 -> +0x10
0x02459EE0 -> +0x14
0x02459F20 -> +0x18
```

## 4. El pickup remoto termina en el inventario del actor correcto

Dentro del receptor, después de localizar el actor objetivo, la ruta usa:

```text
0x01BCC327 -> 0x01D336A0
```

que ya estaba demostrada como acceso:

```text
uNpc + 0x1524 -> cBioItemPack
```

Después llama a:

```text
0x01B8695D -> 0x0243D100
```

para aplicar el item al pack.

Por tanto el pipeline remoto queda:

```text
cPickupItemSyncData
 -> resolver actor
 -> uNpc+0x1524
 -> cBioItemPack del actor
 -> aplicar item
 -> actualizar/eliminar world item
```

**CONFIRMADO:** el pickup remoto del partner no escribe el inventario de P1; aplica el objeto al pack del actor remoto/partner.

## 5. En local, la aplicación ocurre antes de la replicación de red

La función de pickup local alrededor de:

```text
0x02460610
```

ejecuta primero la lógica local del item y solo después consulta el estado Coop/Network y, si corresponde, llama al sender `0x0244CF30`.

Ejemplos:

```text
0x024607CB / 0x024607D9 / 0x02460A87
 -> 0x01C639B6
 -> 0x024646F0        ; aplicación local

después:
0x024607E1...
 -> condición de sincronización
 -> 0x02460890        ; sender

segunda rama:
0x02460A8F...
 -> condición
 -> 0x02460B3E        ; sender
```

Esto demuestra que la red es una capa de replicación posterior; no es necesaria para aplicar el pickup local.

## 6. Bloqueo real para Sub0 local

Hay **dos gates explícitos de categoría player** en la ruta local.

### Gate exterior

En `0x02460610`:

```asm
mov ecx,[this+0xF48]       ; actor asociado
call 0x01C53859            ; obtiene ID
push eax
call 0x01C8C9A1            ; is category "pl"
...
je exit
```

Call exacto al predicado:

```text
0x02460664 -> 0x01C8C9A1
```

### Gate interior

La aplicación real:

```text
0x01C639B6 -> 0x024646F0
```

empieza con:

```asm
if (!actor) exit
actorID = actor->getID()
if (!isPl(actorID)) exit
```

Call exacto:

```text
0x02464726 -> 0x01C8C9A1
```

## 7. Por qué esto funciona online y falla en local

Online:

```text
PC A:
jugador local = pl
 -> pickup local permitido
 -> sender

PC B:
partner remoto = np
 -> cPickupItemSyncData receiver
 -> aplica al cBioItemPack del partner
```

Local co-op en una sola instancia:

```text
P1 = pl
Sub0/P2 = np + ThinkMode::Pad
```

Aunque v10 ya proporciona:

- PadData[1];
- Sub0 en ThinkMode::Pad;
- sItemBoxCoop;
- cBioItemPack propio del partner;

Sub0 sigue fallando en estos dos gates porque su categoría continúa siendo `np`.

## 8. Parche mínimo previsto

No se debe convertir globalmente `np` en `pl` ni permitir pickups a todos los NPC.

El wrapper previsto para esos **dos callsites concretos** será:

```text
isPickupPlayer(id):
    if stock_isPl(id):
        return true

    if !gLocalCoopActive:
        return false

    if !gSub0Npc:
        return false

    return gSub0Npc->getID() == id
```

Se reutilizan:

```text
gSub0Npc          = 0x057D9184
gLocalCoopActive  = 0x057D9188
getID thunk       = 0x01C53859 -> 0x027F1200
stock isPl thunk  = 0x01C8C9A1 -> 0x01D77EB0
```

Solo se redirigirán:

```text
0x02460664
0x02464726
```

a ese wrapper.

Así P1 conserva el comportamiento stock, online conserva el comportamiento stock y únicamente el Sub0 exacto puede atravesar los gates `pl` cuando el cooperativo local está activo.
