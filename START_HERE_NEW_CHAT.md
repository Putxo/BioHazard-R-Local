# START HERE — handoff completo para un chat nuevo

> **Ámbito:** exclusivamente la investigación de este chat sobre cooperativo local para Resident Evil Revelations 1 / BioHazard Revelations PC.
>
> **Regla principal:** no empezar de cero, no rehacer trabajo ya cerrado y no tratar versiones históricas como candidatas actuales. La base canónica actual es **v10**, todavía sin validación runtime.

## 1. Objetivo final

Conseguir en una sola instancia del juego:

- P1 y P2 locales reales;
- dos mandos físicos independientes;
- P2 controlando el partner/Sub0;
- dos cámaras independientes;
- pantalla partida persistente;
- inventario/equipamiento correcto para ambos;
- pickups/munición/hierbas correctos;
- HUD/pausa/interacciones/QTE/transiciones/cutscenes compatibles;
- preservar el comportamiento stock/online cuando el cooperativo local no está activo.

No se suben EXE/DLL/PDB/assets propietarios al repositorio.

---

## 2. Ejecutables fuente exactos de este chat

| Build | SHA-256 |
|---|---|
| `BioRevHD 30-Enero-2013.exe` | `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69` |
| `BioRevHD 23-Feb-2013 prototipo(2).exe` | `f67742e6f8d0f6ade1defc2dcdc34ba06e9e95d2abdc630a605a7da9b437a041` |
| `rerev May 17, 2013 prototipo.exe` | `a015f15a92dd4d242404d83d012a5319d677c15195a0fe862091ebb827dafe34` |
| `rerev Feb 7, 2024 retail.exe` | `1573b79eb921b1e7f571e6deb71c39961c7f1940fad6d19283fc3aea7fca48aa` |

La base principal es **30-Enero-2013 FullDebugWin32**.

Razón: conserva RTTI, menús debug y lógica que febrero ya eliminó. Febrero sigue siendo útil como comparación.

Ejemplo decisivo:

- enero conserva lógica real detrás de `CreateRaidPlayer`;
- febrero conserva menú/wrapper, pero el destino investigado termina en `0x0079C2D0: ret 4`.

---

## 3. Base canónica actual: v10

### v9 — base de input+cámara limpia

`BioRevHD 30-Enero-2013 LOCAL COOP v9 FULLPAD CLEAN PERSISTENT SPLIT.exe`

SHA-256:

`5dc7a7a413ce5916c2758be43b3107d5adf00beee93533b4acf5d83cec97c4be`

Builder:

`patches/build_v9_clean_persistent_split.py`

Manifest:

`research/manifests/p2-fullpad-clean-persistent-split-v9.json`

### v10 — candidato canónico actual

`BioRevHD 30-Enero-2013 LOCAL COOP v10 CLEAN SPLIT COOP ITEMBOX.exe`

SHA-256:

`5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6`

Builder mínimo sobre v9:

`patches/build_v10_itembox_from_v9.py`

Manifest:

`research/manifests/local-coop-v10-itembox.json`

Verificación de linaje:

- v10 difiere de v9 en **23 bytes efectivos / 2 rangos**;
- si se elimina únicamente el wrapper nuevo de ItemBox y se restaura la llamada original, se reproduce **exactamente** el SHA de v9;
- mismo tamaño PE.

**Estado:** verificado estáticamente, no probado todavía dentro del juego.

---

## 4. Entidad P2 / Sub0 — confirmado

RTTI/clases reales:

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

18/21 slots virtuales son iguales; los slots 0,4,5 difieren.

El slot 5 selecciona nativamente:

```text
Main -> 0
Sub0 -> 1
Sub1 -> 2
```

La función común `0x02DD0080` consume ese índice dentro de `sPcsManager`.

Campos debug confirmados:

```text
sPcsManager+0x1308 mIsDebug
+0x130C MainID
+0x1310 Sub0ID
+0x1314 Sub1ID
```

La identidad del actor exacto de Sub0 se rastrea por:

- vtable `uPcsPlayerSub0 = 0x04E1649C`;
- binder común alrededor de `0x02DF5015`;
- actor vivo guardado en `uPcsPlayerSub0+0x44`;
- tracker local usado por parches: `gSub0Npc = 0x057D9184`.

---

## 5. ThinkMode — confirmado

Etiquetas internas:

```text
ThinkMode::Invalid
ThinkMode::Pad
ThinkMode::Cpu
ThinkMode::Network
```

Mapeo confirmado por etiquetas + flujo ejecutable:

```text
0 = Invalid
1 = Pad
2 = Cpu
3 = Network
```

La línea actual **no secuestra Network**.

v6 introdujo el cambio correcto:

- solo el actor exacto Sub0;
- solo si está en `Cpu(2)`;
- usa la ruta canónica para pasarlo a `Pad(1)`;
- Network(3) permanece intacto.

Ruta canónica:

```text
virtual thunk 0x01BB8B60
 -> wrapper 0x0278CC40
 -> setter thunk 0x01BE3257
 -> setter 0x027F1290
```

---

## 6. Dos mandos físicos — confirmado estáticamente

`sGamePad` mantiene dos entradas reales.

### Dos PadData

Helper:

```text
0x01C4F385 -> 0x02DBE720
```

Valida:

```text
index < 2
element = base + index*0xC0
```

### Dos objetos sPad::Pad

Punteros:

```text
sGamePad + 0x968 + index*4
index 0..1
```

RTTI:

```text
.?AVPad@sPad@@
```

Constructor low-level:

```text
0x01C01EAF -> 0x03358F60
```

### DirectInput físico

Documentado en `docs/10-physical-pad-binding.md`.

Cadena confirmada:

```text
dispositivo DirectInput
 -> socket 0/1
 -> Pad lógico 0/1
 -> sPad::Pad[index]
 -> PadData[index]
 -> sGamePad(selector)
```

Strings internos incluyen:

```text
Work[%d] Pad[%d] Socket[%d]
New JoyPad controller[p%d] is found.
Already controller[p%d] attached.
Device is created.
ERROR: Pad[v%d] can't use.
```

---

## 7. Bug/limitación PC de sGamePad y solución v7

La API recibe un selector de pad, pero la implementación PC original colapsa muchas rutas a:

```text
sGamePad::mStartPadNo @ +0x970
```

Por eso los primeros experimentos que solo devolvían selector 1 eran insuficientes.

v7 restaura el selector en todos los sitios relevantes de la ruta uNpc localizados.

Manifest:

`research/manifests/p2-fullpad-v7.json`

Datos:

- 129 llamadas directas al selector encontradas;
- 67 cargas estándar de `mStartPadNo` relevantes;
- 8 cargas analógicas alineadas ya corregidas antes;
- **75 sitios de carga** cubiertos;
- 0 cargas `mStartPadNo` restantes en esos 75 sitios conocidos.

Resultado:

```text
Sub0 exacto -> selector 1 -> PadData[1]
resto       -> selector 0 -> PadData[0]
```

---

## 8. Cámara / split-screen — estado canónico v9

Dos `uCameraManage` reales:

```text
Self    = sGameCamera+0xCE0
Partner = sGameCamera+0xCE4
```

Los botones debug `Self View` / `Partner View` alternan cuál ocupa VIEW_0.

Funciones:

```text
Self View    0x0203E570
Partner View 0x0203E630
 -> 0x0203E8A0
```

Rutinas relevantes:

```text
activate   0x01BF353F -> 0x02069AB0
deactivate 0x01C684D4 -> 0x020699B0
bind viewport 0x01C34D5A -> 0x01EBD610
set target 0x01C275EC -> 0x02066AB0
```

### Viewports

`sCamera` construye 8 Viewport reales:

```text
base   = sCamera+0x30
stride = 0x190
count  = 8
```

Campos:

```text
+0x04 mpCamera
+0x10 mVisible
+0x13 mMode
+0x14 mDisplay
+0x18 mRegion (estructura/rect, NO enum)
```

Enum exacto de `mMode`:

```text
FULLSCREEN=0
FREE=1
TOP=2
BOTTOM=3
LEFT=4
RIGHT=5
TOPLEFT=6
BOTTOMLEFT=7
TOPRIGHT=8
BOTTOMRIGHT=9
VIRTUAL=10
```

### v9

Flag:

```text
gLocalCoopActive = 0x057D9188
```

Solo se activa tras el cambio canónico del Sub0 exacto `Cpu->Pad`.

Cuando está activo:

```text
Self    -> VIEW_0 -> TOP(2)    -> display 0
Partner -> VIEW_1 -> BOTTOM(3) -> display 0
Partner target = exact tracked Sub0 uNpc
```

El helper persistente está alrededor de:

```text
0x01C94FC0..0x01C950FA
ensure_split = 0x01C9504C
```

### Por qué v8 está superseded

v8 se construyó sobre el **v7 combinado**, que ya heredaba un split v4 incondicional.

Por tanto no preservaba completamente la cámara stock cuando el flag local estaba apagado.

v9 se reconstruyó sobre **v7 input-only**, eliminando ese problema.

**No volver a usar v8 como base.**

---

## 9. ItemBox cooperativo — confirmado

Clases reales:

```text
sItemBox
sItemBoxCoop : sItemBox

sItemBox::cBag
sItemBoxCoop::cBagCoop : cBag
```

Tamaños:

```text
sItemBox / sItemBoxCoop 0x188
cBag 0xB8
cBagCoop 0xC0
```

### Corrección importante

`cBagCoop+0xB8/+0xBC` **NO son IDs de jugador**.

El owner fue identificado como:

```text
sCoopManager::cCoopSkill
```

No reutilizar esos campos como player IDs.

### Slots de sistema

Array global de dos instancias:

```text
0x05562878 + index*4
```

Registro:

```text
0x01D06C10(index) -> &slot[index]
cGameSystem escribe this en 0x02D0A094
0x01D06B20(index) lee slot[index]
```

`cGameSystemDouble<sItemBox,0>` crea automáticamente la segunda instancia mediante `sItemBoxCoop::MyDTI`.

Resultado incluso partiendo de Campaign:

```text
slot 0 = sItemBox
slot 1 = sItemBoxCoop
```

### Selector nativo

`cSystemData<Game>+0x20` está registrado como:

```text
mGameMode
```

Valores:

```text
GameMode::Campaign = 0
GameMode::Coop     = 1
```

Stock:

```text
Campaign -> slot 0
Coop     -> slot 1
```

---

## 10. v10: ItemBox Coop sin cambiar GameMode global

No se fuerza `mGameMode=Coop` globalmente.

Wrapper nuevo:

```text
VA 0x01C95100
```

Callsite:

```text
0x01D067AF
```

Thunk stock “is Coop”:

```text
0x01C076CF
```

Semántica:

```cpp
return stock_isCoop() || gLocalCoopActive;
```

Por tanto:

```text
Campaign + local OFF -> ItemBox slot 0
stock Coop           -> ItemBox slot 1
Campaign + local ON  -> ItemBox slot 1
```

No cambia misiones, red, saves u otros consumidores globales de GameMode.

---

## 11. Bags player/NPC — confirmado

Normalizador:

```text
0x01C1C8B8 -> 0x01D4B120
id & 0xF00F0000
```

Etiquetas internas:

```text
0x80010000 -> "pl"
0x80020000 -> "np"
0x80030000 -> "em"
0x80040000 -> "it"
0x80050000 -> "wp"
0x80060000 -> "om"
```

En `sItemBoxCoop`:

```text
pSharedBag -> 0x80010000
pMyBag     -> 0x80011000
```

Dos slots virtuales que en `sItemBox` devuelven null pasan en Coop a resolver:

```text
0x80020000 -> categoría NPC/partner
```

Interpretación funcional fuerte: el ItemBox cooperativo tiene ruta de bag específica para el partner/NPC.

---

## 12. PcsSub e inventario/equipamiento — confirmado

PcsMain tiene 84 acciones, PcsSub 77.

**77/77 acciones Sub tienen equivalente Main.**

Main añade 7 de lifecycle/gestión.

El contexto específico de PcsSub está en:

```text
cFsmActionPcsSub+0x1078
```

Procede de una tabla de punteros reales:

```text
sPcsManager+0xB44
```

Setter:

```text
0x01C171AB -> 0x02DD04E0
```

Guarda el objeto y en una tabla paralela `+0x704` guarda su ID.

### Acciones que aceptan explícitamente pl o np

```text
AddWeapon          0x02A16E80
ClearWeapon        0x02A17270
NpcSetWeaponSlot   0x02A23C90
NpcResetWeaponSlot 0x02A23F80
NpcChangeEquipSlot 0x02A29590
```

Todas continúan si el contexto es categoría `pl` o `np`.

Después convierten/validan por `uNpc` y alcanzan:

```text
0x01BCC327 -> 0x01D336A0
uNpc+0x1524 -> cBioItemPack
```

**No escriben el inventario de P1.**

`ChangeWeapon 0x02A11AB0` es distinto: filtra solo `pl`. No usarlo como prueba para P2; las rutas Npc* son las relevantes.

---

## 13. cBioItemPack — confirmado

Campos:

```text
+0xA0 mSubWeapon[5]
+0xB4 mSubBulletSlot[4]
+0xC8 mHerbNum
+0xCC mCoopKeyNum
```

La interfaz virtual de subweapon es real y se usa en gameplay.

El cambio rápido de slots del Sub0 ya llega a PadData[1] en v10.

Ruta:

```text
PadData[1]
 -> getters sGamePad corregidos por v7
 -> Sub0 uNpc
 -> 0x027ADB10(slot)
 -> equipo del partner
```

Los cuatro slots observados convergen en `0x027ADB10`.

Por tanto el cambio funcional de arma del partner ya está separado de P1.

---

## 14. HUD: pista que NO debe confundirse

`uGUI_MainEquipWin` / `uGUI_SubEquipWin` existen, pero el nombre **Sub** aquí se relaciona con **sub-weapon**, no demuestra por sí mismo una ventana “Player 2”.

No volver a usar el nombre como prueba de ownership P2.

Sí existe lógica diferenciada Main/SubWeapon y `notifyPadInput`, pero la acción funcional de cambio rápido ya ocurre en gameplay y usa PadData[1].

---

## 15. PlayerPad remoto — arquitectura útil confirmada

Tipo:

```text
cPlayerPadSyncData
```

Layout:

```text
+0x08 moveAnalog
+0x10 rotateAnalog
+0x18 aimAnalog
+0x20 waistRotateX
+0x24 isRun
+0x25 isAim
```

Sender:

```text
0x0274B200
```

Receiver:

```text
0x027B1D70
```

El receiver escribe en los mismos campos del `uNpc` que usa la ruta local/AI.

Esto fue clave para entender que el partner online ya tenía una ruta de control completa.

---

## 16. Caminos descartados/corregidos que NO deben repetirse

- **Febrero como base principal por XInput:** descartado; enero conserva lógica que febrero stubbea.
- **`mCameraList[0]/[1]` como P1/P2:** descartado; pertenecen a `uObjModel`.
- **`mPadViewportNo` como pad→viewport:** descartado; contexto de vibración.
- **`uCameraManage::mPadNo+0x88` como selector físico:** no demostrado; retirado de parches modernos.
- **primeros v1/v2 cambiando selector 0→1:** insuficientes porque PC `sGamePad` ignoraba el argumento y usaba `mStartPadNo`.
- **redirigir todos los NPC Cpu:** descartado; debe filtrarse el Sub0 exacto.
- **forzar ThinkMode::Network para local:** descartado; v6 usa Cpu→Pad.
- **v5 manteniendo Cpu y desviando solo una función:** superseded por v6, porque otros sistemas podían seguir tratando al partner como AI.
- **v8 como base:** superseded por v9 por split heredado incondicional.
- **`cBagCoop+0xB8/+0xBC` como player IDs:** descartado; son estado `cCoopSkill`.
- **confusión mMode/mRegion:** resuelta; REGION_* enum está en `mMode`; `mRegion` es estructura/rect de 16 bytes.
- **VIEW_4 como Partner stock:** incorrecto; VIEW_4 recibe otro objeto/free/debug. Partner se alterna stock sobre VIEW_0.

El registro completo de hipótesis y rectificaciones está en:

`docs/03-hypotheses-and-discarded-paths.md`

---

# 17. CONTINUAR EXACTAMENTE DESDE AQUÍ

La última investigación quedó interrumpida por el límite de envío mientras se estudiaba **la ruta de pickups/munición/hierbas del partner**.

No empezar por HUD ni por otra versión del parche.

## Objetivo inmediato

Demostrar si un pickup hecho por Sub0 local:

```text
1. identifica correctamente al partner;
2. termina en su cBioItemPack / bag np;
3. reutiliza la ruta Coop ya existente;
4. o necesita un hook local mínimo porque la ruta está condicionada por red.
```

## Tipo descubierto

String/RTTI:

```text
sItem::cNetSyncData::cPickupItemSyncData
```

También existe:

```text
uItem::cNetSyncData::cGetItemSyncData
```

## Packet cPickupItemSyncData — trabajo parcial

Se localizaron setters simples alrededor de:

```text
0x0244D0F0 -> [this+0x08]
0x0244D140 -> [this+0x0C]
0x0244D190 -> [this+0x10]
0x0244D1E0 -> [this+0x14]
```

y getters simétricos alrededor de:

```text
0x02459E30 -> +0x08
0x02459E70 -> +0x0C
0x02459EB0 -> +0x10
0x02459EF0 -> +0x14
```

**Los cuatro campos aún no están nombrados semánticamente.**

No inventar nombres hasta seguir productores/consumidores.

## Sender / construcción candidata

Bloque alrededor de:

```text
0x0244CF80
```

construye un objeto temporal, rellena varios campos mediante setters y lo pasa a una ruta de sync/send.

Se observan argumentos provenientes de `[ebp+0x10]`, `[ebp+0x14]`, `[ebp+0x18]`, etc.

Pendiente atribuir cada uno.

## Receiver importante

Thunk:

```text
0x01B9F142 -> 0x02459C20
```

Este es el receptor/callback que se estaba desmontando cuando se interrumpió el trabajo.

Dentro, alrededor de:

```text
0x02459D31
```

llama:

```text
0x01BCC327 -> 0x01D336A0
```

que ya está identificado en otras rutas como acceso:

```text
uNpc+0x1524 -> cBioItemPack
```

Después llama a:

```text
0x01B8695D
```

cuya semántica exacta todavía no se cerró.

**Ésta es la pista más prometedora:** el receiver de pickup alcanza la misma ruta de pack del actor.

## Estado de uItem observado en productores

Getters alrededor de `0x024372D0` leen campos:

```text
uItem+0xF28
uItem+0xF20
uItem+0xF24
uItem+0xF3C (byte)
uItem+0xF3D (byte)
uItem+0xF40
```

Todavía no están nombrados con certeza.

## Otra ruta candidata

Bloque:

```text
0x02460900
```

usa varias veces el getter real de playerID:

```text
0x01C53859 -> 0x027F1200
```

y escribe:

```text
uItem+0xF14 = 2
```

Es probable que forme parte del flujo de interacción/pickup, pero **no está atribuido definitivamente**.

## Punto exacto del timeout visible

La respuesta que se cortó estaba:

```text
"Desassemblant la inicialització global a 0x49A8023"
```

Por tanto un chat nuevo debe retomar también la inicialización global alrededor de:

```text
0x049A8023
```

para identificar la tabla/registro DTI asociado a la ruta de pickup.

No considerar ese punto resuelto.

---

## 18. Después de cerrar pickups

Orden recomendado, sin saltos:

1. confirmar pickup/item/ammo/herb de Sub0;
2. decidir si hace falta **v11**. Solo crear v11 si existe un cambio de código concreto y demostrado;
3. estudiar key items / `mCoopKeyNum`;
4. revisar interacción/puertas/puzzles/QTE de PcsSub;
5. revisar muerte/revive y checkpoints;
6. revisar pausa/menu ownership;
7. revisar HUD visual;
8. revisar cutscenes/forced cameras;
9. runtime test de v10 o v11;
10. corregir solo lo que falle realmente.

---

## 19. Archivos que debe leer un chat nuevo

Orden mínimo:

1. **este archivo**;
2. `docs/02-investigation-log.md` — cronología exhaustiva;
3. `docs/03-hypotheses-and-discarded-paths.md`;
4. `docs/10-physical-pad-binding.md`;
5. `docs/11-sub-inventory-fsm-ui.md`;
6. `docs/12-pickup-sync-wip.md`;
7. manifest de v9;
8. manifest de v10;
9. builders v7/v9/v10.

Para detalles históricos:

- `docs/06-experiments.md`;
- `docs/07-*.md`;
- `docs/08-*.md`;
- `docs/09-*.md`;
- manifests de v5/v6/v7/v8.

---

## 20. Estado de archivos locales tras reinicio del entorno

En el entorno actual sobrevivieron los **cuatro EXE fuente originales**, pero no los EXE experimentales derivados.

Esto no invalida nada: los builders/manifests están versionados en GitHub.

Un chat nuevo debe **reconstruir v7→v9→v10 desde los builders** si necesita bytes del candidato.

Nunca inventar un sandbox path ni asumir que un EXE experimental sigue montado.

---

## 21. Regla de trabajo para continuar

Cada hallazgo nuevo debe:

1. verificarse contra el EXE local de enero;
2. marcarse como CONFIRMADO / HIPÓTESIS / DESCARTADO;
3. subirse a GitHub antes de avanzar demasiado;
4. no borrar errores históricos: documentar la rectificación;
5. no crear otra versión del parche salvo cambio de código real;
6. no tocar Network(3) salvo evidencia inequívoca;
7. preservar sesiones stock/online cuando `gLocalCoopActive=0`.

**Base actual: v10. Próximo trabajo: cPickupItemSyncData / receiver 0x02459C20 / init global 0x049A8023.**
