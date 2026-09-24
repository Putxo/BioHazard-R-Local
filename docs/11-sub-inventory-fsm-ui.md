# 11 — PcsSub, inventario cooperativo y SubEquipWin

Build principal: **BioRevHD 30-Enero-2013.exe**.

Este documento recoge la investigación de la capa de gameplay/UI posterior a v9. No implica todavía que el inventario P2 sea controlable con Pad 2; demuestra qué infraestructura separada ya existe y qué falta conectar.

## 1. Simetría FSM PcsMain / PcsSub

Se compararon los registros de acciones:

- `cFsmActionPcsMain::cMain...Parameter`
- `cFsmActionPcsSub::cSub...Parameter`

Resultado:

```text
PcsMain actions: 84
PcsSub actions:  77
77/77 acciones Sub tienen equivalente Main con el mismo nombre funcional
```

Main añade únicamente acciones de lifecycle/gestión:

```text
Init
RemoveEnemy
RemoveNpc
RemoveObject
SetCharacter
StartSubFsm
StopSubFsm
```

Entre las acciones compartidas están:

- cambio, alta y borrado de armas;
- set/reset de weapon slot;
- cambio de equip slot;
- pickup;
- ataque;
- puertas;
- puzzles;
- ayuda;
- warp;
- HP;
- radio;
- target.

**Conclusión:** PcsSub ya tiene una API FSM de gameplay paralela muy amplia; no es únicamente un actor de IA que camina.

## 2. cFsmActionPcsSub y el contexto +0x1078

RTTI:

```text
app::game::fsm::cFsmActionPcsSub
vtable 0x04DCF41C
constructor 0x02A75BC0
```

El constructor inicializa:

```text
+0x1070 = 0
+0x1074 = -1
+0x1078 = 0
```

La rutina de configuración:

```text
0x01C3696B -> 0x02DCFBC0
```

asigna:

```text
+0x1070 = arg1
+0x1074 = arg2
+0x1078 = arg3
```

El caller directo observado alrededor de `0x02DCFA8B` toma `arg3` de una tabla de punteros del manager alrededor de `+0xB44`.

### Importante

`+0x1078` está demostrado como **contexto/objeto específico del Sub** usado por las acciones de gameplay.

Todavía **no** se le asigna un nombre de clase más concreto: no se ha demostrado que sea directamente un `uNpc*` ni otro tipo específico.

## 3. Las mismas funciones comunes de armas sirven a Main y Sub

Para cinco acciones importantes se localizaron exactamente dos callers directos de cada thunk: un wrapper Main y un wrapper Sub.

### ChangeWeapon

```text
common thunk 0x01C31600 -> 0x02A11AB0

Main wrapper 0x02A6B650
  third context = 0

Sub wrapper 0x02A928D0
  call around 0x02A92908
  third context = [this+0x1078]
```

### AddWeapon

```text
common thunk 0x01C92914 -> 0x02A16E80

Main wrapper 0x02A6BD70
  call 0x02A6BDA0
  third context = 0

Sub wrapper 0x02A93120
  call 0x02A93158
  third context = [this+0x1078]
```

### ClearWeapon

```text
common thunk 0x01C80A70 -> 0x02A17270

Main wrapper 0x02A6BDD0
  call 0x02A6BE00
  third context = 0

Sub wrapper 0x02A93190
  call 0x02A931C8
  third context = [this+0x1078]
```

### NpcSetWeaponSlot

```text
common thunk 0x01BA7D15 -> 0x02A23C90

Main wrapper 0x02A6CA90
  call 0x02A6CAC0
  third context = 0

Sub wrapper 0x02A94070
  call 0x02A940A8
  third context = [this+0x1078]
```

### NpcResetWeaponSlot

```text
common thunk 0x01BA014B -> 0x02A23F80

Main wrapper 0x02A6CAF0
  call 0x02A6CB20
  third context = 0

Sub wrapper 0x02A940E0
  call 0x02A94118
  third context = [this+0x1078]
```

### NpcChangeEquipSlot

```text
common thunk 0x01C4C437 -> 0x02A29590

Main wrapper 0x02A6D150
  call 0x02A6D180
  third context = 0

Sub wrapper 0x02A94850
  call 0x02A94888
  third context = [this+0x1078]
```

**Conclusión:** Main y Sub reutilizan literalmente las mismas implementaciones centrales de armas/equipamiento; la diferencia es el contexto específico que Sub suministra.

## 4. Estado cooperativo de jugador separado

Tipos:

```text
cPlayerSaveParam      size 0x90
cPlayerSaveParamCoop  size 0xA0
```

El metadata de `cPlayerSaveParamCoop` registra explícitamente:

```text
mSelf.mLifePoint
mSelf.mVitality
mPartner.mLifePoint
mPartner.mVitality
mPartnerThink
```

Esto confirma estado cooperativo Self/Partner separado para vida/vitalidad y modo del compañero.

## 5. cBioItemPack ya contiene inventario específico Sub

RTTI:

```text
app::game::chara::cBioItemPack
vtable 0x04D2D42C
constructor ~0x0242D3D0
```

Property metadata:

```text
+0x04  mSlot
+0x40  mBullet           [12]
+0xA0  mSubWeapon        [5]
+0xB4  mSubBulletSlot    [4]
+0xC8  mHerbNum
+0xCC  mCoopKeyNum
```

### Interfaz virtual real para mSubWeapon

```text
vslot 18 / +0x48
  thunk 0x01B84F4F -> 0x0242CD90
  wrapper setter orientado a charaID

vslot 19 / +0x4C
  thunk 0x01BCE050 -> 0x0242CD30
  mSubWeapon[index] = value, index < 5

vslot 20 / +0x50
  thunk 0x01C94E99 -> 0x0242CFB0
  wrapper getter orientado a charaID

vslot 21 / +0x54
  thunk 0x01BE4FDF -> 0x0242CF10
  getter directo mSubWeapon[index], index < 5
```

Los wrappers 18/20 convierten el `charaID` al slot cooperativo correspondiente antes de acceder a la colección.

Esto demuestra que `mSubWeapon` forma parte de la interfaz normal de inventario por personaje.

## 6. mSubWeapon / mSubBulletSlot se usan en gameplay real

No se consideran evidencia los xrefs que pertenecen al editor/reflection genérico.

La evidencia relevante está en código normal del item pack.

### Inicialización / port

Alrededor de:

```text
0x0242EF99..0x0242F35A
```

el código:

- inicializa/copia `mSubWeapon[5]`;
- inicializa `mSubBulletSlot[4]` a `-1`;
- elimina duplicados;
- rellena slots secundarios activos.

### addItem

La función normal de item-add, alrededor de `0x02433A90`, usa la interfaz virtual de subweapon.

Ejemplos:

```text
~0x02435B96  llamada por vslot +0x50
~0x02435C25  otra llamada por vslot +0x50
```

También mantiene directamente `mSubBulletSlot` alrededor de:

```text
0x02435BB0..0x02435C07
```

**Conclusión:** el inventario Sub no es metadata debug; participa en pickups/ammo normales de partida.

## 7. UI separada MainEquipWin / SubEquipWin

Clases distintas:

```text
uGUI_MainEquipWin
  vtable 0x04DE4C3C

uGUI_SubEquipWin
  vtable 0x04DE57B4
```

Comparando 50 entradas de vtable:

```text
42 iguales
8 diferentes: slots 0,1,3,4,5,8,11,47
```

Por tanto SubEquipWin tiene comportamiento específico y no es una mera etiqueta visual.

`uCockpitManagerMain` mantiene ventanas separadas:

```text
MainEquipWin
SubEquipWin
MainEquipWinBlur
SubEquipWinBlur
```

## 8. notifyPadInput existe por separado para Main y Sub

String común:

```text
notifyPadInput
VA 0x04DE504C
```

Registro Main:

```text
~0x02B3BC90
callback thunk 0x01C312D1 -> 0x02B3C790
```

Registro Sub:

```text
~0x02B43250
callback thunk 0x01BC6549 -> 0x02B43840
```

### Main handler

Comprueba:

```text
[this+0x2A8] = mMainWeaponNum
```

### Sub handler

Comprueba:

```text
[this+0x2A4] = mSubWeaponNum
```

Si el contador Sub es cero no entra en el flujo/animación correspondiente.

Esto confirma que la ruta de pad de la ventana Sub se vincula a estado de inventario Sub, no al contador Main.

## 9. SubEquipWin consume el inventario Sub

Rutina grande de construcción/actualización de la lista Sub:

```text
0x02B439B0
thunk 0x01C30737
caller ~0x02B4309E
```

Dentro usa dos veces un virtual en offset:

```text
vslot +0x54
```

Ejemplos:

```text
~0x02B43E70..0x02B43E78
~0x02B44118..0x02B44123
```

Ese offset coincide con el getter directo de `mSubWeapon[index]` en `cBioItemPack`.

Antes de la llamada convierte el candidato con:

```text
0x01B7F4F0 -> 0x01D33FA0
```

Los IDs de arma positivos retornados se guardan en:

```text
this + 0x2AC + mSubWeaponNum*4
```

y se incrementa `mSubWeaponNum`.

### Precaución de atribución

El objeto pasado a la rutina presenta exactamente la interfaz esperada de subweapon y el vslot coincide con `cBioItemPack`, pero todavía se está siguiendo el flujo RTTI completo del objeto en el caller.

Por ello se registra como evidencia muy fuerte de consumo de la interfaz Sub, evitando afirmar más de lo demostrado sobre su tipo dinámico en ese punto concreto.

## 10. Implicación para local co-op

La evidencia acumulada indica que no hace falta inventar desde cero:

- inventario P2;
- slots de arma P2;
- ammo P2;
- acciones FSM de equipamiento P2;
- ventana de equipo P2.

Ya existen rutas separadas nativas.

El problema pendiente se estrecha a:

1. quién despacha/llama `notifyPadInput`;
2. cómo decide MainEquipWin vs SubEquipWin;
3. qué selector de pad llega a esa capa;
4. cómo abrir/controlar SubEquipWin con Pad 2 sin afectar Main;
5. cómo arbitrar pausa/menús globales.

No se hará un parche UI especulativo hasta identificar ese dispatcher.


---

## 11. Cambio rápido de arma de Sub0 ya usa PadData[1]

Se siguió la ruta de selección directa de los cuatro slots de equipo dentro del `uNpc`.

Bloque principal:

```text
0x0272603F -> selector de pad 0x01C6C746
             -> sGamePad 0x01C8DD97 -> 0x02DB0E90
             -> si activo: 0x027ADB10(slot=1)

0x02726080 -> selector
             -> sGamePad 0x01C2B0D9 -> 0x02DB0F60
             -> 0x027ADB10(slot=2)

0x027260C1 -> selector
             -> sGamePad 0x01BA0637 -> 0x02DB1030
             -> 0x027ADB10(slot=3)

0x02726102 -> selector
             -> sGamePad 0x01C87F41 -> 0x02DB1100
             -> 0x027ADB10(slot=0)
```

Por tanto el orden completo de acciones observadas es:

```text
1, 2, 3, 0
```

y las cuatro convergen en la misma rutina de cambio de equipamiento:

```text
0x01BC4F41 -> 0x027ADB10
```

### 0x027ADB10

La rutina acepta exclusivamente índices `0..3`, usa una jump table y opera sobre el estado de equipo del `uNpc`.

Entre los campos manipulados:

```text
uNpc + 0x18DC   estructura/lista de equipamiento
uNpc + 0x1C78   selección/equipo activo
```

La rutina además contiene las comprobaciones/sincronización necesarias antes de aplicar el cambio.

### Verificación sobre v10

Los cuatro getters `sGamePad` anteriores son precisamente parte de los sitios corregidos por v7/fullpad.

En v10:

```text
0x02DB0EC5 / 0x02DB0ECF -> usan [ebp+0x08]
0x02DB0F95 / 0x02DB0F9F -> usan [ebp+0x08]
0x02DB1065 / 0x02DB106F -> usan [ebp+0x08]
0x02DB1135 / 0x02DB113F -> usan [ebp+0x08]
```

En el original esos sitios cargaban `sGamePad::mStartPadNo`.

Como v6/v7 establece:

```text
Sub0 exacto -> selector 1
resto       -> selector 0
```

el flujo queda:

```text
PadData[1]
 -> getter de botón de slot
 -> uNpc Sub0
 -> 0x027ADB10(slot)
 -> equipamiento del partner
```

### Consecuencia

**CONFIRMADO estáticamente:** el cambio rápido de arma/equipamiento de P2 ya está conectado al segundo pad en v10.

No hace falta parchear:

- `notifyPadInput`;
- `uGUI_SubEquipWin`;

para conseguir el cambio funcional de arma.

`notifyPadInput` se mantiene interpretado como notificación/actualización visual de la ventana; la acción real ocurre en gameplay, dentro de la ruta `uNpc`.

No se establece aquí ninguna prioridad funcional adicional. Este hallazgo solo confirma que el cambio de arma de Sub0 ya usa PadData[1]; cualquier investigación posterior se limitará a lo necesario para completar el cooperativo local solicitado.
