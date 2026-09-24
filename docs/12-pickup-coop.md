# 12 — Pickup cooperativo: Self / Partner y cBioItemPack

Build principal: **BioRevHD 30-Enero-2013.exe**.

Este documento continúa la línea canónica actual: **v10**. No se añade aquí ningún parche nuevo; se documenta la ruta de pickup ya existente.

## 1. cPickupItemSyncData — pickup remoto cooperativo

RTTI:

```text
app::game::chara::sItem::cNetSyncData::cPickupItemSyncData
vtable 0x04D2F344
```

Constructor/init aproximado:

```text
0x0244D050
```

Layout confirmado por setters/getters:

```text
+0x08 dword
+0x0C dword
+0x10 dword
+0x14 dword
+0x18 byte
```

Sender:

```text
0x01BD0B93 -> 0x0244CF30
```

El sender construye el paquete a partir de `uItem`.

El primer DWORD (`+0x08`) procede de:

```text
0x01C53859 -> 0x027F1200
```

y en el receptor se usa como ID/tipo del item recogido.

## 2. Receptor remoto

El dispatcher de red reconoce el DTI de `cPickupItemSyncData` y registra:

```text
0x01B9F142 -> 0x02459C20
```

como callback de recepción.

La rutina resuelve el actor participante mediante su identidad/sesión y termina localizando un `uNpc` vivo.

Después:

```text
uNpc
 -> 0x01BCC327 -> 0x01D336A0
 -> uNpc+0x1524
 -> cBioItemPack
```

y llama a:

```text
0x01B8695D -> 0x0243D100
```

pasando el ID de item de `cPickupItemSyncData+0x08`.

**CONFIRMADO:** un pickup remoto del partner se aplica al `cBioItemPack` del actor remoto/partner, no al pack de P1.

---

## 3. FsmPickupItem local/común

String:

```text
FsmPickupItem
```

Registro de acción:

```text
~0x02A9F0DA
callback thunk 0x01B7A487 -> 0x029971C0
```

Parameter type:

```text
cFsmAction::cPickupItemParameter
size 0x34
vtable 0x04DBC9D4
MyDTI 0x0558D340
constructor 0x029569D0
```

Inicialización relevante:

```text
+0x04 byte = 0
+0x05 byte = 1
+0x08 dword = 0
+0x0C dword = 0
```

Los nombres exactos de esos miembros no se inventan si no aparecen en metadata recuperada.

## 4. El target es un uItem real

`FsmPickupItem` usa `+0x08/+0x0C` para localizar el objeto de mundo y valida su DTI contra:

```text
uItem
DTI global 0x05579584
string literal "uItem"
size registrado 0x10F0
```

La validación/cast pasa por:

```text
0x01C174F8 -> 0x01FFF070
```

## 5. +0x05 selecciona Self o Partner

Dentro de `0x029971C0`:

```text
if parameter+0x05 != 0:
    resolver A
else:
    resolver B
```

### Resolver A = Self

Ruta:

```text
0x01BDF98B -> 0x01CB51A0
0x01C2C7F4 -> 0x01CB7400
```

`0x01CB7400` recorre la colección y usa el predicado:

```text
0x01BB3192 -> 0x01CB7560
```

La identidad del predicado queda demostrada por un assert de otra ruta del mismo ejecutable, alrededor de `0x0204710B`:

```text
call 0x01BB3192(candidate)
...
"(chara::getSelf()( pPlayer ))"
"chara::getSelf()( pPlayer )"
```

Por tanto el predicado es literalmente el usado por `chara::getSelf`.

**CONFIRMADO:** `parameter+0x05 != 0` selecciona Self.

### Resolver B = Partner

Ruta:

```text
0x01BDF98B -> 0x01CB51A0
0x01BCA0C2 -> 0x01D115B0
```

`0x01D115B0` recorre la misma colección con un predicado distinto:

```text
0x01C8731B -> 0x01D116C0
```

El predicado:

- compara el ID del candidato contra el mismo ID usado por Self;
- exige que **no** sea Self;
- añade comprobaciones de validez/estado;
- devuelve el otro actor elegible.

Existe además código que llama consecutivamente:

```text
getSelf  -> 0x01C2C7F4
Partner  -> 0x01BCA0C2
```

y usa ambos actores simultáneamente, por ejemplo alrededor de `0x01D46194..0x01D461C6`.

**CONFIRMADO por estructura y uso emparejado:** el segundo resolver es la contraparte Partner.

Así:

```text
cPickupItemParameter+0x05 = 1 -> Self
cPickupItemParameter+0x05 = 0 -> Partner
```

El constructor pone `+0x05=1`, por lo que Self es el valor por defecto.

## 6. El pickup se aplica al pack del actor seleccionado

Después de escoger Self/Partner:

```text
selected actor
 -> 0x01BCC327 -> 0x01D336A0
 -> cBioItemPack del actor
```

A continuación `FsmPickupItem` invoca el virtual `+0x18` del pack con:

```text
arg1 = 0x0F
arg2 = uItem*
arg3 = 0
```

**CONFIRMADO:** la acción común de pickup no está fijada a P1; contiene un selector Self/Partner y opera sobre el pack del actor seleccionado.

## 7. Implicación para v10

Ya existen dos mecanismos nativos consistentes:

```text
pickup local/common:
  selector Self/Partner
  -> pack del actor

pickup network:
  participant remoto
  -> uNpc remoto
  -> pack del actor
```

Por tanto no se justifica introducir un parche de inventario para "copiar" pickups de P1 a P2.

### Pendiente

La siguiente comprobación es la ruta de **pickup interactivo normal en gameplay**:

- si Sub0 en ThinkMode::Pad puede iniciar esa recogida directamente;
- qué valor/selector usa al crear la acción;
- si la lógica local reutiliza el selector Partner ya existente;
- si alguna condición sigue dependiendo de sesión de red.

Hasta cerrar eso, no se crea v11.
