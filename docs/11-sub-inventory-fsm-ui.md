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


---

## 12. Las acciones PcsSub aceptan explícitamente categoría NPC

Se siguieron las implementaciones comunes usadas por los wrappers Main/Sub.

Los predicados de categoría están confirmados:

```text
0x01C8C9A1 -> 0x01D77EB0
  normalize(id) == 0x80010000  ; "pl"

0x01C2B557 -> 0x01D77F10
  normalize(id) == 0x80020000  ; "np"
```

El normalizador es:

```text
0x01C1C8B8 -> 0x01D4B120
id & 0xF00F0000
```

### NpcChangeEquipSlot

Wrapper Sub:

```text
0x02A94850
  [this+0x1078]
  -> tercer argumento
  -> 0x01C4C437
  -> 0x02A29590
```

La implementación común hace:

```text
pl ? continuar
si no:
np ? continuar
si no: salir
```

Bloques:

```text
0x02A29619 -> is "pl"
0x02A29646 -> is "np"
0x02A29655 -> ruta común admitida
```

Por tanto NpcChangeEquipSlot acepta expresamente player o NPC.

### AddWeapon

Wrapper Sub:

```text
0x02A93120
  [this+0x1078]
  -> tercer argumento
  -> 0x01C92914
  -> 0x02A16E80
```

Filtro:

```text
0x02A16F0D -> pl
0x02A16F3A -> np
0x02A16F4D -> ruta común
```

Después:

```text
0x01B9D60D -> 0x0208FCB0
```

valida/convierte por el DTI de `uNpc`.

El DTI global usado en esa conversión (`0x055894DC`) se registra con la string literal:

```text
"uNpc"
```

A continuación:

```text
0x01BCC327 -> 0x01D336A0
```

accede al miembro en:

```text
uNpc + 0x1524
```

y devuelve el puntero contenido en ese wrapper/scoped_ptr. Las llamadas posteriores utilizan la interfaz del `cBioItemPack` ya reconstruida.

Ruta relevante:

```text
PcsSub context
 -> actor pl/np
 -> uNpc
 -> uNpc+0x1524
 -> cBioItemPack
 -> alta de arma/item
```

### ClearWeapon

Wrapper Sub:

```text
0x02A93190
  [this+0x1078]
  -> 0x01C80A70
  -> 0x02A17270
```

Filtro:

```text
0x02A172FD -> pl
0x02A1732A -> np
0x02A1733D -> ruta común
```

Después vuelve a usar:

```text
0x01B9D60D -> uNpc
0x01BCC327 -> uNpc+0x1524 -> cBioItemPack
```

y limpia las colecciones internas del pack, incluidos slots/subweapon/bullets.

### NpcSetWeaponSlot

Wrapper Sub:

```text
0x02A94070
  [this+0x1078]
  -> 0x01BA7D15
  -> 0x02A23C90
```

Filtro:

```text
0x02A23D19 -> pl
0x02A23D46 -> np
```

Después convierte por `uNpc` y aplica el slot.

### NpcResetWeaponSlot

Wrapper Sub:

```text
0x02A940E0
  [this+0x1078]
  -> 0x01BA014B
  -> 0x02A23F80
```

Filtro:

```text
0x02A24009 -> pl
0x02A24036 -> np
```

Después vuelve a la misma ruta `uNpc` y resetea el estado de slot.

## 13. Qué es +0x1078

El setter:

```text
0x01C3696B -> 0x02DCFBC0
```

guarda en `cFsmActionPcsSub+0x1078` el tercer contexto recibido desde `sPcsManager`.

La fuente es una tabla:

```text
sPcsManager + 0xB44
```

organizada en grupos de `0x44 = 17 * 4` bytes.

El setter real de esa tabla:

```text
0x01C171AB -> 0x02DD04E0
```

recibe un **puntero de objeto real** y lo almacena directamente en:

```text
[sPcsManager + group*0x44 + 0xB44 + index*4]
```

En paralelo obtiene un ID del mismo objeto y lo guarda en la tabla hermana:

```text
sPcsManager + group*0x44 + 0x704 + index*4
```

Por tanto `+0x1078` no es un ID ni una bandera: es un contexto de objeto vivo mantenido por `sPcsManager`.

Todavía no se fuerza un nombre C++ más específico al puntero de `+0x1078`, porque los seis productores de la tabla se están siguiendo por separado. Lo ya demostrado es suficiente para las acciones de inventario: las implementaciones comunes validan dinámicamente el objeto como categoría `pl` o `np` antes de operar.

## Consecuencia

La capa de gameplay ya soporta de forma nativa que las acciones Sub modifiquen el inventario/equipamiento de un actor NPC/partner.

Esto complementa la evidencia de `sItemBoxCoop`:

```text
sItemBoxCoop
  player bag  -> categoría pl
  partner bag -> categoría np

PcsSub actions
  aceptan pl o np
  -> uNpc
  -> cBioItemPack propio
```

No se necesita añadir un parche nuevo para estas operaciones antes de la validación runtime.


---

## 14. FsmPickupItem puede entregar al Main o al Partner

La recogida genérica no está implementada como una acción PcsSub llamada `PickupItem`. PcsSub sí registra `PcsSubItemSetEnablePickup`, pero la acción que añade realmente el objeto es la común:

```text
FsmPickupItem
callback thunk 0x01B7A487 -> 0x029971C0
```

### Parámetro confirmado por metadata

La rutina de metadata alrededor de `0x02997020` registra:

```text
+0x04 mIsDrawMessage
+0x05 mIsAddMainPlayer
+0x08 mListNo
+0x0C mDataNo
+0x10 mArrange
```

El nombre `mIsAddMainPlayer` elimina la ambigüedad sobre la bifurcación de destinatario.

### Validación del objeto recogido

El handler resuelve el objeto y lo valida contra el DTI global:

```text
0x05579584
```

La inicialización estática de ese DTI usa la string literal:

```text
"uItem"
```

Por tanto el objeto recogido se valida realmente como `uItem`.

### Selección del destinatario

El handler bifurca con:

```text
if (param.mIsAddMainPlayer)
    actor = ruta Main
else
    actor = ruta other/partner
```

Rutas:

```text
Main:
  0x01C2C7F4 -> 0x01CB7400

Partner/other:
  0x01BCA0C2 -> 0x01D115B0
```

Ambas recorren la colección de actores.

La rama Main usa un predicado que exige que el ID del candidato coincida con la identidad de referencia.

La rama Partner usa un predicado que exige que el ID sea distinto de la identidad de referencia, además de validaciones de estado/actividad.

No se asignan nombres más fuertes a esos predicados secundarios hasta recuperar sus metadatos.

### El objeto termina en el pack del actor seleccionado

Una vez elegido el actor:

```text
0x01BCC327 -> 0x01D336A0
```

obtiene:

```text
uNpc + 0x1524 -> cBioItemPack
```

Después se invoca el virtual del pack:

```text
vslot 6 / +0x18
thunk 0x01BD549A -> 0x02433B40
```

con el item recogido.

Esa implementación modifica el propio pack destinatario y contiene las rutas normales de munición/hierbas/items.

### Consecuencia

**CONFIRMADO estáticamente:** la acción común de pickup ya sabe añadir un objeto tanto al Main como al Partner.

No hay que redirigir manualmente un pickup de P2 hacia el inventario de P1 ni crear una acción PcsSub nueva.

El flujo nativo ya es:

```text
uItem
 -> mIsAddMainPlayer ?
      Main actor
      Partner actor
 -> actor uNpc
 -> uNpc+0x1524
 -> cBioItemPack propio
 -> add item
```

Pendiente: seguir `sItem::cNetSyncData::cPickupItemSyncData` para comprobar cómo la recogida interactiva/sincronizada transporta la identidad del destinatario y si esa capa necesita adaptación para local co-op.


---

## 15. cPickupItemSyncData sincroniza el ítem, no el destinatario

Se reconstruyó la ruta de red asociada a:

```text
sItem::cNetSyncData::cPickupItemSyncData
```

DTI global:

```text
0x05578DF0
```

Tamaño registrado:

```text
0x1C
```

### Layout observado

La implementación específica expone cinco campos de payload:

```text
+0x08 DWORD
+0x0C DWORD
+0x10 DWORD
+0x14 DWORD
+0x18 BYTE
```

Setters:

```text
0x0244D0F0 -> +0x08
0x0244D140 -> +0x0C
0x0244D190 -> +0x10
0x0244D1E0 -> +0x14
0x0244D230 -> +0x18
```

Getters del receptor:

```text
0x02459E20 -> +0x08
0x02459E60 -> +0x0C
0x02459EA0 -> +0x10
0x02459EE0 -> +0x14
0x02459F20 -> +0x18
```

### Productor

El productor común:

```text
0x0244CF30
```

construye el paquete y rellena esos cinco campos.

Callers confirmados:

```text
0x02434322
0x02460890
0x02460B3E
```

En los callers se observa que:

- un campo procede del ID/tipo del item;
- otro procede de la identidad/ID de la instancia `uItem`;
- los tres restantes proceden de estado de drop/param del objeto.

Strings de depuración adyacentes incluyen:

```text
dropVal
dropID
param
```

No se asigna todavía cada uno de esos tres nombres a un offset concreto sin evidencia adicional.

### Receptor

El dispatcher de sync reconoce el DTI de `cPickupItemSyncData` y crea un delegate hacia:

```text
0x01B9F142 -> 0x02459C20
```

La rutina receptora:

1. localiza/sincroniza la instancia del item mediante los IDs del paquete;
2. obtiene el actor/inventario local correspondiente por la ruta normal;
3. extrae:
   `uNpc+0x1524 -> cBioItemPack`;
4. usa el campo `+0x08` como valor de item en la operación de inventario.

### Corrección de interpretación

**No existe en este paquete un campo explícito "Main/Partner".**

La sincronización de red transporta el estado/identidad del **ítem**, mientras la selección del destinatario se resuelve en otra capa.

Esto concuerda con `FsmPickupItem`, donde el destinatario sí está nombrado explícitamente mediante:

```text
mIsAddMainPlayer
```

### Consecuencia para local co-op

No es necesario convertir el pickup local de P2 en un paquete de red ni introducir un playerID artificial en `cPickupItemSyncData`.

La ruta preferida es mantener el flujo local:

```text
interacción de Sub0
 -> FsmPickupItem
 -> mIsAddMainPlayer = false
 -> Partner actor
 -> cBioItemPack del partner
```

Pendiente: identificar quién construye/establece `cPickupItemParameter::mIsAddMainPlayer` en la interacción real para comprobar que el flujo iniciado por Sub0 ya selecciona la rama Partner o necesita un hook mínimo.


---

## 14. Pickup de Sub0 entrega al cBioItemPack propio

La finalización local del pickup converge en:

```text
0x024646F0
```

v11/v12 solo amplían el filtro inicial para admitir al **Sub0 exacto** cuando el cooperativo local está activo.

Una vez superado ese filtro, se reutiliza el código stock.

En las ramas normales de entrega:

```text
0x02464855 / 0x02464931
  actor = [ebp-0x14]
  -> 0x01BCC327
  -> uNpc+0x1524
  -> cBioItemPack
  -> 0x01C79B3F -> 0x02439270
```

Por tanto, cuando el actor admitido es el Sub0 local:

```text
uItem
 -> Sub0 uNpc
 -> Sub0 cBioItemPack
 -> add/item handling stock
```

**CONFIRMADO estáticamente:** el pickup no se entrega al pack de P1 por el hecho de estar en una Campaign. La ruta usa el actor pasado a `0x024646F0` y obtiene el `cBioItemPack` de ese actor.

## 15. Rama adicional Coop: relief/reparto de munición

Al final de `0x024646F0` existe una condición extra:

```text
0x01C78FA0 -> 0x01D754A0
normalized ID == 0x80040200
```

Si se cumple, llama:

```text
0x01B8A1AC -> 0x027ABFE0
```

`0x027ABFE0` sale inmediatamente si el juego no está en `GameMode::Coop`.

En Coop:

1. obtiene otro actor distinto del actor actual;
2. valida ese actor;
3. calcula un `dropID/dropVal`;
4. obtiene un multiplicador de munición;
5. multiplica/redondea la cantidad;
6. obtiene el `cBioItemPack` del otro actor;
7. llama al virtual `+0x34`.

El virtual:

```text
0x01BD8B63 -> 0x0242C590
```

mapea:

```text
0x80040200 -> mBullet[0]
0x80040201 -> mBullet[1]
0x80040202 -> mBullet[2]
0x80040203 -> mBullet[3]
0x80040204 -> mBullet[4]
```

e incrementa la munición respetando el máximo.

Esto identifica `0x027ABFE0` como lógica adicional de reparto/relief de munición al compañero, no como la entrega básica del pickup.

### Multiplicador

El getter:

```text
0x01B8D7F8 -> 0x02506FE0
```

devuelve:

```text
Campaign -> 1.0
Coop     -> [0x05479970] = 1.5
```

La propiedad `0x05479970` está registrada con la etiqueta japonesa:

```text
入手弾数倍率
```

que significa **multiplicador de cantidad de munición obtenida**.

### Consecuencia para v12

v12 ya entrega el pickup principal al pack correcto de Sub0.

Lo que todavía no replica en Campaign local es esta regla secundaria de Coop para repartir/bonificar munición al otro participante, porque `mGameMode` global permanece en Campaign por diseño.

No se fuerza `mGameMode`: el siguiente análisis debe encontrar el hook local mínimo para activar solo esta regla cuando `gLocalCoopActive=1`.


---

## 14. Bloqueo confirmado: el pickup local exige categoría pl

La ruta de recogida local alcanza:

```text
0x01C639B6 -> 0x024646F0
```

Entrada relevante:

```text
this = objeto/item
arg1 = actor que recoge
```

Al comienzo de `0x024646F0`:

```asm
test arg1,arg1
je   exit

mov  ecx,arg1
call 0x01C53859        ; obtiene ID/categoría
push eax
call 0x01C8C9A1        ; -> 0x01D77EB0, predicate "pl"
test al,al
je   exit
```

El predicado `0x01D77EB0` ya está identificado como:

```text
normalize(id) == 0x80010000
```

es decir, categoría interna:

```text
pl
```

La función **no prueba después `np`** como sí hacen `AddWeapon`, `ClearWeapon`, `NpcSetWeaponSlot`, `NpcResetWeaponSlot` y `NpcChangeEquipSlot`.

### Consecuencia para Sub0 local

Sub0 sigue siendo un actor `uNpc` / categoría:

```text
np = 0x80020000
```

aunque v6/v7/v9/v10 lo cambien de:

```text
ThinkMode::Cpu -> ThinkMode::Pad
```

Por tanto el pickup local stock no puede asumirse funcional para P2.

**CONFIRMADO:** existe un bloqueo explícito `pl-only` en la ruta de recogida.

### Siguiente línea de investigación

No se parcheará todavía el predicado a ciegas.

El juego ya contiene:

```text
sItem::cNetSyncData::cPickupItemSyncData
```

usado por el cooperativo online. Se está reconstruyendo su productor/receptor para saber cómo Capcom replica una recogida hecha por el partner remoto y decidir si:

1. el hook correcto es permitir exactamente al Sub0 local atravesar `0x024646F0`; o
2. conviene reutilizar la ruta de `cPickupItemSyncData`.

Hasta cerrar esa comparación no se crea una nueva versión del EXE.


---

## 15. cGetItemSyncData y PickUpState: la ruta coop usa el mismo pickup común

La investigación de sincronización de objetos distinguió dos mensajes diferentes.

### cPickupItemSyncData

Tipo:

```text
sItem::cNetSyncData::cPickupItemSyncData
```

Este mensaje sincroniza principalmente estado del objeto recogido en el mundo. No es por sí solo la operación que entrega el ítem al inventario.

### cGetItemSyncData

Tipo:

```text
uItem::cNetSyncData::cGetItemSyncData
```

Datos confirmados:

```text
string/RTTI: 0x04D30850
DTI global:  0x05579564
size:        0x08
vtable:      0x04D2FEB0
```

Su dispatcher recibe el mensaje alrededor de:

```text
0x0245FB90
```

Callback:

```text
0x01C4FBC8 -> 0x02464AD0
```

`0x02464AD0` obtiene el owner `uItem` desde el `cNetSyncData`, resuelve el actor remoto y termina llamando al mismo handler común:

```text
0x01C639B6 -> 0x024646F0
```

Por tanto el cooperativo online no tiene un segundo algoritmo de inventario para recoger objetos: reutiliza el mismo `uItem::pickup(actor)`.

## 16. Punto exacto del turno cortado: 0x049A8023 = DTI de uItem

El análisis que quedó cortado por el timeout de la interfaz estaba desmontando la inicialización global alrededor de:

```text
0x049A8023
```

La inicialización completa identifica el DTI:

```text
global 0x05579584
string "uItem" @ 0x04D30838
size 0x10F0
```

Por tanto `FsmPickupItem` valida realmente un objeto `uItem`; ese punto no era una clase de jugador pendiente de identificar.

## 17. PickUpState trabaja explícitamente con uNpc

RTTI/clases:

```text
player::PickUpStateBase
player::PickUpState
player::PickUpStateWaterSurface
player::PickUpStateUnderWater
```

Vtables:

```text
PickUpStateBase         0x04D95560
PickUpState             0x04D955A0
PickUpStateWaterSurface 0x04D955E0
PickUpStateUnderWater   0x04D95620
```

El constructor de `PickUpState` instala la vtable en:

```text
0x026FE340..0x026FE378
```

El método virtual relevante:

```text
0x0270AC70
```

recibe un actor y llama:

```text
0x01B9D60D -> 0x0208FCB0
```

para convertir/validar ese actor como `uNpc`.

Después pasa el actor resultante a:

```text
0x01BBE8F8 -> 0x0270AD00
```

y `0x0270AD00` termina invocando:

```text
uItem::pickup(actor)
0x01C639B6 -> 0x024646F0
```

con el actor `uNpc` explícito.

### Consecuencia

La propia arquitectura de `PickUpState` está preparada para trabajar con un `uNpc`. Por tanto el filtro inicial `pl-only` de `0x024646F0` es una regla de admisión, no una incompatibilidad estructural del resto del handler con `uNpc`.

## 18. v11: excepción mínima solo para el Sub0 local exacto

Se construyó v11 sobre v10.

Archivo:

```text
BioRevHD 30-Enero-2013 LOCAL COOP v11 SUB0 PICKUP.exe
```

SHA-256:

```text
66b8a6ac440a2b676ce687860073a500617ff86de71d588464eeedc38532b415
```

El test stock:

```text
0x02464733
if (!is_pl(actor)) exit;
```

se conserva para todos los casos normales.

Solo cuando `is_pl(actor)` falla, v11 comprueba:

```text
gLocalCoopActive != 0
AND
actor == gSub0Npc
```

Si ambas condiciones son ciertas, vuelve a la continuación stock:

```text
0x02464739
```

En cualquier otro caso conserva el exit original:

```text
0x024649AE
```

Helper:

```text
0x01C95120
```

### Verificación

v11 se reconstruyó desde el EXE original reproduciendo antes exactamente los SHA publicados de:

```text
v6  83554753d1d6a86bf256627830f509a313871a847c87338b61da0e3c0a2c0914
v7  25cb559a183156bbb8de312688da86bec300bd78e66f0d6c6f7d776a3cc88af7
v9  5dc7a7a413ce5916c2758be43b3107d5adf00beee93533b4acf5d83cec97c4be
v10 5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6
```

Después del cambio v11:

```text
same file size: yes
36 bytes diferentes vs v10
2 rangos
```

Revirtiendo únicamente el branch de 6 bytes y el helper de 32 bytes se reproduce exactamente el SHA de v10.

### Alcance de seguridad

v11 **no** permite pickup a todos los NPC.

```text
NPC genérico -> sigue rechazado
Sub0 sin local-coop -> sigue rechazado
Sub0 exacto + local-coop activo -> permitido
```

Builder y manifest:

- `patches/build_v11_sub0_pickup_from_v10.py`
- `research/manifests/local-coop-v11-sub0-pickup.json`
