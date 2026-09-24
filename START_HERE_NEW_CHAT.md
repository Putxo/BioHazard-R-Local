# START HERE — estado canónico completo para un chat nuevo

> **Ámbito:** exclusivamente la investigación de cooperativo local de Resident Evil Revelations 1 realizada en este chat.
>
> **Estado real más reciente:** la base estática canónica es **v13 SYMMETRIC AMMO RELIEF**. Después de v13 la investigación avanzó a **puertas/interacciones 2P**. No volver a empezar por pickups ni por v10.

---

# 1. Objetivo final

Conseguir, en una sola instancia del juego:

- P1 y P2 locales reales;
- dos mandos físicos independientes;
- Sub0/partner controlado localmente;
- dos cámaras y pantalla partida;
- inventario/equipamiento/pickups correctos;
- reparto de munición equivalente a Coop;
- puertas/interacciones/QTE/transiciones/cutscenes compatibles;
- preservar stock/online cuando el modo local no está activo.

No subir EXE/DLL/PDB/assets propietarios.

---

# 2. Build fuente principal

`BioRevHD 30-Enero-2013.exe`

SHA-256:

`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`

Es `FullDebugWin32` y conserva RTTI, menús debug y lógica eliminada posteriormente.

Builds auxiliares de comparación:

```text
23-Feb-2013 prototype
f67742e6f8d0f6ade1defc2dcdc34ba06e9e95d2abdc630a605a7da9b437a041

17-May-2013 prototype
a015f15a92dd4d242404d83d012a5319d677c15195a0fe862091ebb827dafe34

07-Feb-2024 retail
1573b79eb921b1e7f571e6deb71c39961c7f1940fad6d19283fc3aea7fca48aa
```

Enero es la base principal porque conserva lógica real detrás de sistemas que febrero ya deja en stubs.

---

# 3. Base canónica actual: v13

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP v13 SYMMETRIC AMMO RELIEF.exe`

SHA-256:

`3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa`

Base directa:

`v12 SUB0 PICKUP PAD2`

SHA v12:

`5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8`

Builder:

`patches/build_v13_ammo_relief.py`

Assembly:

`research/patches/ammo_relief_v13.S`

Manifest:

`research/manifests/local-coop-v13-ammo-relief.json`

Estado:

**STATICALLY VERIFIED ONLY — runtime pendiente.**

---

# 4. Cadena canónica que lleva a v13

No usar una versión histórica por nombre sin comprobar SHA.

```text
v7 input-only
25cb559a183156bbb8de312688da86bec300bd78e66f0d6c6f7d776a3cc88af7
  ↓
v9 clean persistent split
5dc7a7a413ce5916c2758be43b3107d5adf00beee93533b4acf5d83cec97c4be
  ↓
v10 Coop ItemBox
5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6
  ↓
v11 canonical pickup
2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69
  ↓
v12 pickup Pad 2
5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8
  ↓
v13 symmetric ammo relief
3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa
```

## Advertencia sobre v11

Durante la investigación hubo **más de un experimento llamado v11**.

La v11 que pertenece a la cadena canónica de v12/v13 es:

`2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69`

Es la versión que:

- conserva el candidato P1 stock;
- compara el Sub0 exacto por distancia al mismo `uItem`;
- guarda el más cercano en `uItem+0xF48`;
- amplía los gates pl-only únicamente para el Sub0 exacto con local coop activo.

No usar las otras iteraciones v11 como base de v12.

---

# 5. P2 / Sub0 — confirmado

Clases:

```text
uPcsPlayerMain
uPcsPlayerSub0
uPcsPlayerSub1
```

Vtables:

```text
Main  0x04E1642C
Sub0  0x04E1649C
Sub1  0x04E1650C
```

Índices nativos:

```text
Main=0
Sub0=1
Sub1=2
```

Actor vivo de Sub0:

```text
uPcsPlayerSub0+0x44 -> exact uNpc*
```

Tracker:

```text
gSub0Npc = 0x057D9184
```

---

# 6. ThinkMode — confirmado

```text
Invalid=0
Pad=1
Cpu=2
Network=3
```

Solo Sub0 exacto en Cpu pasa por la transición canónica a Pad.

Ruta:

```text
0x01BB8B60
 -> 0x0278CC40
 -> 0x027F1290
```

Network(3) no se secuestra.

---

# 7. Dos pads físicos — confirmado estáticamente

Backend PC:

```text
DirectInput device
 -> socket 0/1
 -> Pad lógico 0/1
 -> sPad::Pad[0/1]
 -> PadData[0/1]
 -> sGamePad(selector)
```

PadData indexer:

```text
0x02DBE720
index < 2
stride 0xC0
```

v7 restaura el selector en los sitios donde la implementación PC forzaba `mStartPadNo`.

Resultado:

```text
P1   -> selector 0 -> PadData[0]
Sub0 -> selector 1 -> PadData[1]
```

---

# 8. Cámara — v9, heredada por v13

Managers:

```text
Self    = sGameCamera+0xCE0
Partner = sGameCamera+0xCE4
```

Flag:

```text
gLocalCoopActive = 0x057D9188
```

Target Partner:

```text
0x01C275EC -> 0x02066AB0
```

Split:

```text
Self    -> VIEW_0 -> TOP(2)    -> display 0
Partner -> VIEW_1 -> BOTTOM(3) -> display 0
```

Helper persistente:

```text
0x01C94FC0..0x01C950FA
ensure_split = 0x01C9504C
```

v9 se construyó sobre v7 input-only precisamente para que, con flag=0, la cámara stock permanezca intacta.

No usar v8 como base.

---

# 9. ItemBox — v10, heredado por v13

Slots:

```text
0x05562878 + index*4
slot 0 = sItemBox
slot 1 = sItemBoxCoop
```

`sItemBoxCoop` se crea automáticamente.

GameMode:

```text
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

Categorías:

```text
0x80010000 = pl
0x80020000 = np
```

El ItemBox Coop añade bag NPC/partner.

`cBagCoop+0xB8/+0xBC` son `cCoopSkill`, NO player IDs.

---

# 10. PcsSub / cBioItemPack — confirmado

Contexto Sub:

```text
cFsmActionPcsSub+0x1078
```

procede de tabla de objetos reales:

```text
sPcsManager+0xB44
```

Acciones que aceptan `pl` o `np`:

```text
AddWeapon
ClearWeapon
NpcSetWeaponSlot
NpcResetWeaponSlot
NpcChangeEquipSlot
```

Ruta:

```text
actor
 -> uNpc
 -> uNpc+0x1524
 -> cBioItemPack
```

Campos relevantes:

```text
+0xA0 mSubWeapon[5]
+0xB4 mSubBulletSlot[4]
+0xC8 mHerbNum
+0xCC mCoopKeyNum
```

El cambio rápido de arma de Sub0 ya usa PadData[1].

---

# 11. Pickup: investigación cerrada mucho más allá del WIP inicial

El antiguo WIP de `cPickupItemSyncData` ya fue resuelto en gran parte.

## cPickupItemSyncData

Tipo:

`sItem::cNetSyncData::cPickupItemSyncData`

DTI:

`0x05578DF0`

Size:

`0x1C`

Sender:

`0x0244CF30`

Receiver:

`0x01B9F142 -> 0x02459C20`

## 0x049A8023

El punto que se quedó cortado por timeout fue resuelto:

```text
0x049A8023 = inicialización DTI de uItem
string "uItem" @ 0x04D30838
global DTI 0x05579584
size 0x10F0
```

No volver a investigarlo como incógnita.

## cGetItemSyncData

Tipo:

`uItem::cNetSyncData::cGetItemSyncData`

Size:

`0x08`

Global DTI:

`0x05579564`

Dispatcher:

`0x0245FB90`

Callback:

`0x01C4FBC8 -> 0x02464AD0`

La ruta remota termina en el mismo handler común:

```text
0x01C639B6 -> 0x024646F0
```

Por tanto online y local comparten la entrega de pickup.

---

# 12. FsmPickupItem ya distingue Main/Partner de forma nativa

Parámetro:

`cFsmAction::cPickupItemParameter`

Metadata:

```text
+0x04 mIsDrawMessage
+0x05 mIsAddMainPlayer
+0x08 mListNo
+0x0C mDataNo
+0x10 mArrange
```

`mIsAddMainPlayer=1` selecciona Main/Self.

`mIsAddMainPlayer=0` selecciona partner/non-Self.

Ambas rutas convergen en:

```text
actor
 -> 0x01BCC327
 -> actor+0x1524
 -> cBioItemPack
 -> addItem
```

Esto confirma que el sistema de pickup ya está diseñado para entregar al pack del partner.

---

# 13. Bloqueo local stock de uItem y v11 canónico

Stock local:

- finder de candidato busca solo categoría `pl`;
- candidato se guarda en `uItem+0xF48`;
- la finalización vuelve a exigir `pl`.

Sub0 sigue siendo categoría `np` aunque ThinkMode sea Pad.

v11 canónico corrige solo esta ruta local:

- conserva P1 stock;
- añade Sub0 exacto como segundo candidato;
- compara distancia cuadrática al item;
- guarda el más cercano en el único slot stock `uItem+0xF48`;
- gates posteriores aceptan stock pl **o exact Sub0 con local active**;
- NPC genéricos siguen rechazados;
- Network global no se modifica.

Canonical v11 SHA:

`2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69`

Limitación aún abierta:

si el actor más cercano es no elegible para un objeto pero el otro sí lo es, la selección previa por distancia puede necesitar refinamiento.

---

# 14. v12 — ActionCommand usa Pad 2 para pickup de Sub0

El `uItem` contiene un `cActionCommand`.

Stock callback selector:

```text
0x01BA99B7 -> 0x026D7AD0
```

stock devuelve siempre 0.

v12:

```text
uItem target P1   -> member 0 -> PadData[0]
uItem target Sub0 -> member 1 -> PadData[1]
```

solo con local coop activo.

Además corrige:

```text
0x02DB2B50
```

que recibía selector pero en PC volvía a cargar `mStartPadNo`.

SHA v12:

`5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8`

---

# 15. Pickup de Sub0 llega a su propio pack — confirmado

Después de los gates v11/v12, el código stock hace:

```text
actor admitido
 -> 0x01BCC327
 -> uNpc+0x1524
 -> cBioItemPack
 -> add/item handling
```

Por tanto:

**Sub0 recibe el pickup en su propio cBioItemPack.**

No se entrega al pack de P1 por estar en Campaign.

---

# 16. v13 — regla Coop de reparto de munición

Para el grupo normalizado:

`0x80040200`

existe una regla adicional stock:

```text
0x027ABFE0
```

En stock solo corre con `GameMode::Coop`.

Esa regla:

- busca al otro actor;
- obtiene su pack;
- añade munición;
- usa multiplicador Coop de cantidad adquirida.

Multiplicador:

```text
[0x05479970] = 1.5
```

Metadata interna japonesa:

```text
入手弾数倍率
```

= multiplicador de cantidad de munición obtenida.

v13 replica **solo esta regla** para local Campaign:

- gate = stock Coop OR `gLocalCoopActive`;
- si P1 recogió -> otro actor = Sub0 exacto;
- si Sub0 recogió -> otro actor = P1 stock;
- acepta P1 como “other” bajo local coop;
- usa el multiplicador Coop 1.5;
- no cambia `mGameMode` global.

Helper:

```text
0x01C95220...
```

SHA v13:

`3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa`

Limitación actual de pickup:

la arbitraje de candidato todavía elige por cercanía antes de todas las comprobaciones de elegibilidad stock.

---

# 17. PUNTO ACTUAL REAL: puertas/interacciones 2P

Después de cerrar pickups v11/v12 y ammo relief v13, la investigación pasó a puertas.

Documento:

`docs/13-door-2p.md`

## Infraestructura nativa

Clases/strings:

```text
uDoor2pBase
cDoor2pBaseClosedState
Door2pBasePlayer_NetParam
cDoor2pBaseWaitState_PL
cDoor2pBaseCancelState_PL
uTwoOpenDoor
cTwoOpenDoorClosedState
cTwoOpenDoorOpenState
cTwoOpenDoorOpenState_PL
cTwoOpenDoorStopedState
```

Layout duplicado 0/1:

```text
+0x1010 mReadyFlag[0]
+0x1011 mReadyFlag[1]

+0x1018 mGuestStatusFlag[0]
+0x1019 mGuestStatusFlag[1]

+0x1024 mLocalFlag[0]
+0x1025 mLocalFlag[1]
```

## Setter actor-específico

```text
0x01C9217B -> 0x02588D20
```

Callsites directos:

```text
0x0257185D
0x0257198A
```

Recibe actor y obtiene índice 0/1 mediante:

```text
0x01BEDBB7 -> 0x01CB7610
```

No filtra `pl/np`.

Escribe el slot del actor.

## Ready setter

```text
0x02589040
```

recibe índice explícito 0/1 y actualiza `mReadyFlag[index]` y estado paralelo.

La lógica de puerta consulta ambos slots.

## Riesgo pendiente

Otras rutas llaman:

```text
0x01C85962 -> 0x02DA3050
```

y usan el índice local global del proceso.

Todavía no se sabe si esas lecturas pertenecen a gameplay crítico o solo a red/feedback local.

---

# 18. CONTINUAR EXACTAMENTE DESDE AQUÍ

No volver a pickups como tarea principal.

Siguiente trabajo exacto en puertas:

1. identificar el owner/state que contiene los callsites:
   `0x0257185D`, `0x0257198A`;
2. reconstruir qué actor se pasa a `0x02588D20`;
3. comprobar si la acción/wrapper PcsSub llega con Sub0;
4. separar gameplay de sincronización/feedback que usa el índice local global;
5. demostrar un bloqueo concreto antes de parchear;
6. **no modificar globalmente el índice de jugador local**;
7. solo crear v14 si se demuestra un cambio de código necesario.

Después de puertas:

- otras interacciones;
- QTE;
- revive/death;
- checkpoints;
- pausa/menu ownership;
- HUD visual;
- cutscenes/forced cameras;
- runtime test.

---

# 19. Archivos que debe leer un chat nuevo

Orden:

1. **este archivo**;
2. `docs/02-investigation-log.md`;
3. `docs/03-hypotheses-and-discarded-paths.md`;
4. `docs/13-version-lineage.md`;
5. `docs/12-pickup-coop.md`;
6. `docs/12-v11-pickup-actioncommand.md`;
7. `docs/13-door-2p.md`;
8. `docs/11-sub-inventory-fsm-ui.md`;
9. `docs/10-physical-pad-binding.md`;
10. `research/current_state.json`.

Builders/manifests canónicos:

```text
patches/build_v9_clean_persistent_split.py
patches/build_v10_itembox_from_v9.py
patches/build_v11_sub0_pickup.py
patches/build_v12_sub0_pickup_pad2.py
patches/build_v13_ammo_relief.py

research/manifests/p2-fullpad-clean-persistent-split-v9.json
research/manifests/local-coop-v10-itembox.json
research/manifests/local-coop-v11-sub0-pickup.json
research/manifests/local-coop-v12-sub0-pickup-pad2.json
research/manifests/local-coop-v13-ammo-relief.json
```

---

# 20. Históricos que no deben confundirse con la base

- v1/v2: selector antiguo insuficiente;
- v3: selector PC inicial;
- v4: split nativo inicial;
- v5: exact Sub0 pero seguía Cpu;
- v6: Cpu→Pad canónico;
- v7: FULLPAD;
- v8: persistent split construido sobre combinado incondicional — superseded;
- v9: clean persistent split;
- v10: Coop ItemBox;
- múltiples experimentos v11: usar solo el SHA canónico `2c69...` para la cadena v12/v13;
- v12: pickup Pad2;
- v13: ammo relief simétrico.

---

# 21. Estado local de archivos

Los EXE derivados pueden no estar montados después de reinicios del entorno.

Los builders/manifests son la fuente reproducible.

No asumir que v13 existe en `/mnt/data`; reconstruir la cadena cuando sea necesario.

---

# 22. Regla de trabajo

Cada hallazgo nuevo:

1. verificar contra el EXE de enero;
2. marcar CONFIRMADO / HIPÓTESIS / DESCARTADO;
3. persistir en GitHub antes de avanzar demasiado;
4. conservar rectificaciones históricas;
5. no crear nueva versión sin cambio real de código;
6. no secuestrar Network(3);
7. mantener comportamiento stock/online cuando local flag=0.

**Base canónica actual: v13. Próximo trabajo: uDoor2pBase / callsites 0x0257185D y 0x0257198A.**
