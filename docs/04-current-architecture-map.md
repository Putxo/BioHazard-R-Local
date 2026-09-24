# 04 — Mapa técnico actual

Build de referencia: **BioRevHD 30-Enero-2013.exe**.

Base experimental canónica: **v10 CLEAN SPLIT COOP ITEMBOX**.

SHA-256 v10:

`5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6`

Estado: **verificado estáticamente; runtime pendiente**.

---

## 1. Arquitectura actual

```text
uPcsPlayerMain
uPcsPlayerSub0
uPcsPlayerSub1
      |
      v
sPcsManager
      |
      +--> actor uNpc exacto de Sub0
             |
             +--> ThinkMode::Pad
             +--> selector 1
             +--> PadData[1]
             +--> cBioItemPack propio
             +--> Partner uCameraManage target

Pad físico 0 -> sPad::Pad[0] -> PadData[0] -> P1
Pad físico 1 -> sPad::Pad[1] -> PadData[1] -> Sub0

Self uCameraManage    -> VIEW_0 -> TOP
Partner uCameraManage -> VIEW_1 -> BOTTOM

gLocalCoopActive
  |
  +--> cámara persistente
  +--> ItemBox Coop slot 1
```

---

## 2. Roles PCS

Vtables:

```text
uPcsPlayerMain  0x04E1642C
uPcsPlayerSub0  0x04E1649C
uPcsPlayerSub1  0x04E1650C
```

Slot de identidad nativo:

```text
Main=0
Sub0=1
Sub1=2
```

Actor vivo Sub0:

```text
uPcsPlayerSub0+0x44 -> exact uNpc*
```

Tracker local:

```text
gSub0Npc = 0x057D9184
```

---

## 3. ThinkMode

```text
Invalid=0
Pad=1
Cpu=2
Network=3
```

v6+ usa la ruta canónica:

```text
Sub0 exacto en Cpu(2)
 -> 0x01BB8B60
 -> 0x0278CC40
 -> 0x027F1290
 -> Pad(1)
```

Network(3) queda intacto.

---

## 4. Input físico y lógico

Dos pads físicos DirectInput confirmados.

```text
dispositivo
 -> socket 0/1
 -> Pad lógico 0/1
 -> sPad::Pad[index]
 -> PadData[index]
 -> sGamePad(selector)
```

Helper PadData:

```text
0x02DBE720
index < 2
stride 0xC0
```

v7 restaura el selector en los sitios PC que originalmente forzaban:

```text
mStartPadNo @ +0x970
```

Resultado:

```text
P1 -> selector 0
Sub0 exacto -> selector 1
```

---

## 5. Cámara v9

Managers:

```text
Self    = sGameCamera+0xCE0
Partner = sGameCamera+0xCE4
```

Flag:

```text
gLocalCoopActive = 0x057D9188
```

Solo se activa después de la transición canónica de Sub0 a Pad.

Target Partner:

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

Cuando el flag local está apagado, v9 preserva la inicialización de cámara stock.

---

## 6. ItemBox v10

Slots globales:

```text
0x05562878 + index*4
slot 0 = sItemBox
slot 1 = sItemBoxCoop
```

`sItemBoxCoop` se crea automáticamente por el sistema doble.

Selector stock:

```text
cSystemData<Game>::mGameMode
Campaign=0
Coop=1
```

v10 no cambia GameMode global.

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

---

## 7. Bags cooperativos

Categorías:

```text
0x80010000 = pl
0x80020000 = np
```

`sItemBoxCoop` aporta:

```text
pSharedBag -> 0x80010000
pMyBag     -> 0x80011000
NPC/partner accessors -> 0x80020000
```

`cBagCoop+0xB8/+0xBC` son estado `cCoopSkill`, no IDs de jugador.

---

## 8. PcsSub / equipamiento

PcsSub tiene 77 acciones y las 77 tienen equivalente Main.

Contexto Sub:

```text
cFsmActionPcsSub+0x1078
```

proviene de una tabla de punteros reales de:

```text
sPcsManager+0xB44
```

Acciones relevantes que aceptan `pl` o `np`:

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

El cambio rápido de arma de Sub0 ya usa PadData[1].

---

## 9. Caminos descartados

No usar como hooks principales:

```text
mCameraList[0]/[1] -> uObjModel
mPadViewportNo     -> sVibration
uCameraManage::mPadNo -> no demostrado como selector físico
VIEW_4 -> no es Partner stock
```

No reutilizar v8 como base.

---

## 10. Punto pendiente actual

La siguiente capa es **pickup/item/ammo/herb del partner**.

Tipo:

```text
sItem::cNetSyncData::cPickupItemSyncData
```

Receiver prioritario:

```text
0x01B9F142 -> 0x02459C20
```

Dentro alcanza:

```text
0x01BCC327 -> 0x01D336A0
 -> uNpc+0x1524
 -> cBioItemPack
```

Punto exacto adicional pendiente:

```text
0x049A8023
```

Ver:

`docs/12-pickup-sync-wip.md`

No crear v11 hasta demostrar que la ruta de pickup necesita un cambio de código.
