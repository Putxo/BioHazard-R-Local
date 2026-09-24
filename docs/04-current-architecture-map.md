# 04 — Mapa técnico actual

Build de referencia: **BioRevHD 30-Enero-2013.exe**.

Base experimental canónica actual: **v13 SYMMETRIC AMMO RELIEF**.

SHA-256 v13:

`3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa`

Estado: **verificado estáticamente; runtime pendiente**.

---

## 1. Arquitectura canónica

```text
uPcsPlayerMain
uPcsPlayerSub0
uPcsPlayerSub1
      |
      v
sPcsManager
      |
      +--> exact Sub0 uNpc
             |
             +--> ThinkMode::Pad
             +--> selector 1
             +--> PadData[1]
             +--> own cBioItemPack
             +--> Partner uCameraManage target
             +--> local pickup target when selected

Pad físico 0 -> sPad::Pad[0] -> PadData[0] -> P1
Pad físico 1 -> sPad::Pad[1] -> PadData[1] -> Sub0

Self uCameraManage    -> VIEW_0 -> TOP
Partner uCameraManage -> VIEW_1 -> BOTTOM

gLocalCoopActive
  |
  +--> persistent split
  +--> Coop ItemBox slot 1
  +--> exact-Sub0 pickup exception
  +--> uItem ActionCommand member 1
  +--> symmetric Coop ammo-relief rule
```

---

## 2. Roles PCS

Vtables:

```text
uPcsPlayerMain  0x04E1642C
uPcsPlayerSub0  0x04E1649C
uPcsPlayerSub1  0x04E1650C
```

Índices nativos:

```text
Main=0
Sub0=1
Sub1=2
```

Actor vivo:

```text
uPcsPlayerSub0+0x44 -> exact uNpc*
```

Globals propios del parche:

```text
gSub0Npc          = 0x057D9184
gLocalCoopActive  = 0x057D9188
```

---

## 3. ThinkMode

```text
Invalid=0
Pad=1
Cpu=2
Network=3
```

Transición canónica del partner:

```text
exact Sub0 in Cpu(2)
 -> 0x01BB8B60
 -> 0x0278CC40
 -> 0x027F1290
 -> Pad(1)
```

Network(3) queda intacto.

---

## 4. Entrada física y lógica

Backend PC confirmado con dos pads DirectInput.

```text
device
 -> socket 0/1
 -> logical Pad 0/1
 -> sPad::Pad[index]
 -> PadData[index]
 -> sGamePad(selector)
```

Indexer:

```text
0x02DBE720
index < 2
stride 0xC0
```

v7 restaura el selector en los sitios conocidos donde la implementación PC forzaba:

```text
mStartPadNo @ +0x970
```

Resultado:

```text
P1   -> selector 0
Sub0 -> selector 1
```

---

## 5. Cámara heredada de v9

Managers:

```text
Self    = sGameCamera+0xCE0
Partner = sGameCamera+0xCE4
```

Target setter:

```text
0x01C275EC -> 0x02066AB0
```

Viewports:

```text
Self    -> VIEW_0 -> TOP(2)    -> display 0
Partner -> VIEW_1 -> BOTTOM(3) -> display 0
```

Helper persistente:

```text
0x01C94FC0..0x01C950FA
ensure_split = 0x01C9504C
```

Con `gLocalCoopActive=0`, v9 preserva la política de cámara stock.

---

## 6. ItemBox heredado de v10

Slots:

```text
0x05562878 + index*4
slot 0 = sItemBox
slot 1 = sItemBoxCoop
```

`sItemBoxCoop` se crea automáticamente por el sistema doble.

GameMode stock:

```text
Campaign=0
Coop=1
```

v10 NO cambia GameMode global.

Wrapper:

```text
0x01C95100
```

Semántica:

```cpp
return stock_isCoop() || gLocalCoopActive;
```

Callsite:

```text
0x01D067AF
```

Categorías:

```text
0x80010000 = pl
0x80020000 = np
```

`cBagCoop+0xB8/+0xBC` son estado `cCoopSkill`, no player IDs.

---

## 7. PcsSub / cBioItemPack

Contexto Sub:

```text
cFsmActionPcsSub+0x1078
```

Fuente:

```text
sPcsManager+0xB44
```

Acciones confirmadas para `pl` o `np`:

```text
AddWeapon
ClearWeapon
NpcSetWeaponSlot
NpcResetWeaponSlot
NpcChangeEquipSlot
```

Ruta:

```text
PcsSub context
 -> actor pl/np
 -> uNpc
 -> uNpc+0x1524
 -> cBioItemPack
```

Campos pack:

```text
+0xA0 mSubWeapon[5]
+0xB4 mSubBulletSlot[4]
+0xC8 mHerbNum
+0xCC mCoopKeyNum
```

El cambio rápido de arma del Sub0 ya usa PadData[1].

---

## 8. Pickup canónico v11/v12

### uItem

DTI:

```text
global 0x05579584
string "uItem" @ 0x04D30838
size 0x10F0
```

Target local stock:

```text
uItem+0xF48
```

Stock solo encuentra/acepta `pl`.

### v11 canónico

SHA:

`2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69`

Añade exclusivamente el exacto Sub0 como candidato local adicional:

- conserva P1 stock;
- compara distancia P1/Sub0 al mismo `uItem`;
- guarda el más cercano en `+0xF48`;
- gates posteriores admiten stock `pl` o exact Sub0 con local activo;
- no habilita NPC genéricos.

### FsmPickupItem

`cFsmAction::cPickupItemParameter`:

```text
+0x04 mIsDrawMessage
+0x05 mIsAddMainPlayer
+0x08 mListNo
+0x0C mDataNo
+0x10 mArrange
```

`mIsAddMainPlayer=1` selecciona Self/Main.

`mIsAddMainPlayer=0` selecciona partner/non-Self.

Ambas ramas terminan en:

```text
actor
 -> uNpc+0x1524
 -> cBioItemPack
 -> addItem
```

### v12

SHA:

`5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8`

Corrige el `cActionCommand` de `uItem`.

Stock selector:

```text
0x01BA99B7 -> 0x026D7AD0
```

v12:

```text
target P1   -> member 0 -> PadData[0]
target Sub0 -> member 1 -> PadData[1]
```

Además `0x02DB2B50` pasa a respetar el selector en vez de recargar `mStartPadNo`.

**CONFIRMADO estáticamente:** el pickup de Sub0 llega a su propio `cBioItemPack`.

---

## 9. v13 — symmetric ammo relief

Ruta stock:

```text
0x027ABFE0
```

Grupo normalizado:

```text
0x80040200
```

En Coop real reparte/bonifica munición al otro actor.

Multiplicador stock Coop:

```text
[0x05479970] = 1.5
```

Metadata:

```text
入手弾数倍率
```

v13 replica esa regla bajo local co-op sin cambiar `mGameMode`.

Helper:

```text
0x01C95220...
```

Comportamiento:

```text
P1 recoge   -> other = exact Sub0
Sub0 recoge -> other = P1 stock
```

Acepta `pl` recíproco bajo local co-op y conserva comportamiento stock cuando flag=0.

---

## 10. Punto actual: puertas 2P

La investigación actual está en:

```text
uDoor2pBase
```

Infraestructura duplicada:

```text
+0x1010 mReadyFlag[0]
+0x1011 mReadyFlag[1]

+0x1018 mGuestStatusFlag[0]
+0x1019 mGuestStatusFlag[1]

+0x1024 mLocalFlag[0]
+0x1025 mLocalFlag[1]
```

Setter actor-específico:

```text
0x01C9217B -> 0x02588D20
```

Callsites directos:

```text
0x0257185D
0x0257198A
```

Deriva índice 0/1 del actor mediante:

```text
0x01BEDBB7 -> 0x01CB7610
```

No filtra categoría `pl/np`.

Ready setter:

```text
0x02589040
```

recibe índice 0/1 y actualiza estado paralelo.

La lógica de la puerta consulta ambos slots.

### Incógnita actual

Hay otras rutas que usan:

```text
0x01C85962 -> 0x02DA3050
```

como índice local global.

No está demostrado si esas lecturas son:

- gameplay crítico;
- red;
- visual/feedback local;
- o una mezcla.

No tratarlas como bloqueo hasta seguir los callers.

---

## 11. Próximo paso exacto

1. identificar owner/state de `0x0257185D` y `0x0257198A`;
2. reconstruir actor pasado a `0x02588D20`;
3. comprobar si PcsSub/Sub0 llega de forma nativa;
4. separar gameplay de red/feedback;
5. no modificar globalmente el índice local;
6. crear v14 solo si aparece un bloqueo concreto.

---

## 12. Rutas históricas descartadas

No reutilizar:

```text
mCameraList[0]/[1] -> uObjModel
mPadViewportNo     -> vibration
uCameraManage::mPadNo -> physical pad selector no demostrado
VIEW_4 -> not Partner stock
```

No usar v8 como base.

No usar una v11 no canónica para reconstruir v12/v13.

El punto `0x049A8023` ya está resuelto como DTI de `uItem`; no volver a tratarlo como pendiente.
