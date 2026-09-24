# 12 — Pickup de Sub0 y cPickupItemSyncData

Build principal: **BioRevHD 30-Enero-2013.exe**.

Base experimental vigente: **v10 CLEAN SPLIT COOP ITEMBOX**.

Este documento continúa la investigación después de cerrar que PcsSub ya puede modificar su propio cBioItemPack y que sItemBoxCoop aporta bags `pl` y `np`.

## 1. El pickup no es una acción PcsSub específica

No se encontró una acción FSM PcsSub llamada `PickupItem`.

Sí existe:

```text
PcsSubItemSetEnablePickup
```

que controla la habilitación del pickup.

La recogida real pertenece a una capa común de `uItem` / `FsmPickupItem` y además dispone de sincronización de red propia.

## 2. Tipo de sincronización de pickup

RTTI/string:

```text
sItem::cNetSyncData::cPickupItemSyncData
string VA 0x04D2FC5C
global DTI object 0x05578DF0
```

Inicialización global:

```text
~0x049A5300
size = 0x1C
```

La factoría alrededor de:

```text
0x02443DA0
```

reserva `0x1C` bytes.

## 3. Layout parcial confirmado

Setters observados:

```text
0x0244D0F0 -> [this+0x08]
0x0244D140 -> [this+0x0C]
0x0244D190 -> [this+0x10]
0x0244D1E0 -> [this+0x14]
```

El paquete tiene además un último byte alrededor de `+0x18`, coherente con el tamaño total 0x1C y con el builder/sender que recibe cinco argumentos.

Todavía no se renombran semánticamente esos campos hasta completar los getters/callers.

## 4. Sender nativo

Rutina:

```text
0x0244CF30
```

- obtiene el objeto de red desde `[this+0x2C0]`;
- construye un `cPickupItemSyncData` temporal en stack;
- rellena los campos desde cinco argumentos;
- envía por la infraestructura de sync.

Thunk del sender:

```text
0x01BD0B93 -> 0x0244CF30
```

Callers directos localizados en rutas `uItem`:

```text
~0x02434322
~0x02460890
~0x02460B3E
```

## 5. Receiver nativo

El dispatcher alrededor de:

```text
0x024498C0..0x02449AA8
```

compara el DTI recibido con:

```text
cPickupItemSyncData @ 0x05578DF0
```

y crea un delegate hacia:

```text
0x01B9F142 -> 0x02459C20
```

Por tanto `0x02459C20` es el receptor confirmado del pickup sincronizado.

## 6. uItem y el actor candidato de pickup

Metadata de `uItem`:

```text
mEnablePickup          @ +0xF39
mIsInvisibleItem       @ +0xF3A
mIsFoundInvisibleItem  @ +0xF3B
mbPickupOnce           @ +0xF35
mIsParentOM            @ +0xF3D
```

El constructor inicializa además:

```text
+0xF40 = 0x80000000
+0xF44 = 0
+0xF48 = 0
```

Una rutina alrededor de:

```text
0x02461FF0..0x024626D2
```

busca un actor próximo, evalúa distancia/altura/radio y termina almacenando el actor elegido en:

```text
uItem + 0xF48
```

Escritura observada:

```text
~0x024626B3
```

Por tanto `+0xF48` es, con evidencia fuerte, el actor/candidato actual de pickup.

## 7. Bloqueo stock para Sub0: categoría pl

La rutina de finalización alrededor de:

```text
0x02460610
```

empieza comprobando que:

```text
uItem+0xF48 != 0
```

Obtiene el charaID del actor mediante:

```text
0x01C53859 -> 0x027F1200
```

y después llama al predicado ya identificado:

```text
0x01C8C9A1 -> 0x01D77EB0
```

que comprueba:

```text
normalize(id) == 0x80010000
```

es decir, categoría:

```text
pl
```

Si no es `pl`, la función abandona la ruta de pickup.

### Implicación

Sub0 sigue siendo categoría:

```text
np = 0x80020000
```

aunque su ThinkMode pase de Cpu a Pad.

Por tanto esta comprobación es actualmente el candidato más fuerte a impedir que el P2 local complete un pickup por sí mismo.

## 8. Hipótesis activa para el siguiente parche

No se debe permitir `np` genéricamente, porque eso habilitaría pickup para cualquier NPC.

La modificación candidata, todavía NO aplicada, es:

```text
permitir si:
  stock_is_pl(actor)
  OR
  (gLocalCoopActive && actor == gSub0Npc)
```

Globals ya existentes en v10:

```text
gSub0Npc          = 0x057D9184
gLocalCoopActive  = 0x057D9188
```

Antes de construir una nueva versión hay que confirmar que el resto de `0x02460610`, una vez superado ese gate, trabaja correctamente con el actor `np` y termina en el cBioItemPack/ItemBox cooperativo correspondiente.

## 9. Estado

**CONFIRMADO:**

- existe un paquete de sincronización de pickup de 0x1C bytes;
- existe sender y receiver nativos;
- `uItem+0xF48` contiene el actor candidato de pickup;
- la ruta stock de finalización exige categoría `pl`;
- Sub0 local permanece categoría `np`.

**PENDIENTE INMEDIATO:**

1. desmontar por completo `0x02460610` después del gate;
2. demostrar qué actor/contexto se usa para añadir el objeto;
3. comprobar que la ruta acepta el cBioItemPack/ItemBox del Sub0;
4. solo entonces crear un parche exact-Sub0 para el gate.


---

## 10. Tercer bloqueo: el selector solo obtiene el jugador local pl

La rutina que calcula el actor cercano:

```text
0x02461FF0
```

obtiene un identificador del jugador local y llama a:

```text
0x01C07341 -> 0x02DA3840
```

`0x02DA3840` recorre actores pero filtra expresamente por:

```text
0x01C8C9A1 -> categoría pl
```

antes de comparar el identificador. Por tanto el actor que llega a la lógica de distancia de `0x02461FF0` es deliberadamente el jugador local `pl`.

Si supera distancia/altura/estado, se guarda en:

```text
uItem+0xF48
```

**Consecuencia:** parchear solo la finalización no basta; Sub0 nunca llegaría a ser el candidato de pickup.

## 11. Segundo gate pl en la entrega real

La función:

```text
0x01C639B6 -> 0x024646F0
```

recibe el actor que recoge y vuelve a exigir categoría `pl`:

```text
0x02464720 get charaID
0x02464726 call is-pl
0x02464733 JE exit
```

Después de ese gate usa:

```text
0x01BCC327 -> actor/uNpc+0x1524 -> cBioItemPack
```

en varias ramas. Esto es compatible estructuralmente con Sub0, que ya está demostrado como `uNpc` con su propio `cBioItemPack`.

## 12. Diseño candidato exact-Sub0

No se modificará `sNetworkManage` globalmente.

En la ruta de `uItem` se conservará un único candidato local, pero podrá ser:

```text
P1 stock
o
gSub0Npc
```

seleccionando el más cercano al objeto.

Los dos gates posteriores conservarán el comportamiento stock y añadirán únicamente:

```text
stock_is_pl(actor)
OR
(
  gLocalCoopActive == 1
  AND actor == gSub0Npc
)
```

Globals existentes:

```text
gSub0Npc         0x057D9184
gLocalCoopActive 0x057D9188
```

No se habilitará pickup para NPCs genéricos.


---

## 14. El botón de pickup lo gestiona cActionCommand

La investigación posterior a v11 cerró una duda importante: ni `FsmPickupItem` ni el callback final `0x02460610` leen directamente el mando.

### uItem DTI confirmado en 0x049A8023

La inicialización global alrededor de:

```text
0x049A7FF0..0x049A8023
```

registra un tipo de tamaño aproximado `0x10F0`.

La string usada por el registro, en:

```text
0x04D30838
```

es literalmente:

```text
uItem
```

El objeto DTI/global asociado es:

```text
0x05579584
```

Esto confirma que las comprobaciones de DTI encontradas en la ruta de pickup son comprobaciones reales contra `uItem`.

### FsmPickupItem

String:

```text
FsmPickupItem
VA 0x04DD2660
```

Registro:

```text
~0x02A9F0C7
callback thunk 0x01B7A487
 -> 0x029971C0
```

`0x029971C0`:

- resuelve parámetros/objetos;
- valida el target como `uItem` usando DTI `0x05579584`;
- obtiene el actor/contexto;
- obtiene su `cBioItemPack`;
- aplica la operación de inventario correspondiente.

**No contiene lectura directa de sGamePad.**

Por tanto la decisión de “se ha pulsado recoger” sucede antes de esta acción FSM.

### PcsSubItemSetEnablePickup

String/action:

```text
PcsSubItemSetEnablePickup
```

Callback:

```text
0x01C7BF39 -> 0x02A93350
```

`0x02A93350` pasa el contexto Sub `[this+0x1078]` a:

```text
0x01BAB4F1 -> 0x02A18150
```

La función resuelve un objeto, valida DTI `uItem` y habilita/deshabilita su estado de pickup.

**Tampoco es el lector del botón.**

## 15. uItem contiene un cActionCommand

El tipo:

```text
cActionCommand
```

tiene:

```text
size = 0x110
DTI/global = 0x05564ED8
```

y se registra globalmente alrededor de:

```text
0x0493DB40
```

El layout de `uItem` contiene exactamente un bloque de ese tamaño en:

```text
uItem + 0xFD0
```

La inicialización de `uItem` alrededor de:

```text
0x0245F3D4..
```

configura ese `cActionCommand`.

Entre los delegates creados aparece:

```text
0x01BDFA9E -> 0x02460610
```

es decir, el callback de pickup que v11 ya amplió para aceptar al Sub0 exacto.

Configuración observada sobre `uItem+0xFD0`:

```text
~0x0245F4E3
  action ID 0x201
  call 0x01C2427F

~0x0245F4F6
  config 0x10
  call 0x01B960D3

~0x0245F50C
  delegate/callback
  call 0x01C7F26A

~0x0245F51E
  push 2
  call 0x01C20779

~0x0245F53C / 0x0245F54E
  config type 0x0C / 0x0D
  call 0x01C10FD1
```

### Conclusión

La ruta relevante para que Pad 2 dispare el pickup es ahora:

```text
uItem+0xFD0 cActionCommand
  -> detecta acción/botón
  -> delegate
  -> 0x02460610
  -> entrega del item
```

v11 ya resuelve:

- selección P1/Sub0 del candidato;
- gates exact-Sub0;
- entrega a cBioItemPack.

Lo pendiente es **hacer que cActionCommand evalúe el pad correspondiente al actor candidato**, en vez de asumir el pad local principal.

## 16. Strings/tipos útiles del subsistema ActionCommand

Se localizaron:

```text
cActionCommand
cActionCommand::cParamTriggerButton
cParamPressButton
cParamRepeatButton
cParamStick
sActionCommand
Pad1
Pad2
Pad3
Pad4
mPadNo
e:\bhr\source\biorevhd\prog\game\action_command\cactioncommand.cpp
```

Importante: la string compartida `mPadNo` no se toma todavía como prueba de que cada `cActionCommand` posea un selector físico de pad. Algunos xrefs pertenecen a metadata/tester y otros a `uCameraManage`.

La siguiente tarea es identificar la rutina concreta de `cParamTriggerButton` / `sActionCommand` que acaba consultando `sGamePad`.
