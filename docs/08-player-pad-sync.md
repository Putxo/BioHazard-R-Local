# 08 — cPlayerPadSyncData: ruta nativa de control remoto del compañero

Build: **BioRevHD 30-Enero-2013.exe**.

Este hallazgo cambia de forma importante la estrategia de cooperativo local.

## Resumen

El juego contiene una estructura explícita:

```text
app::game::chara::cPlayerPadSyncData
```

que serializa el estado de control de un jugador remoto y, al recibirse, lo aplica directamente al `uNpc` que representa al compañero.

Los campos aplicados son exactamente los mismos que usa la ruta local/AI de control del NPC:

```text
moveAnalog
rotateAnalog
aimAnalog
waistRotateX
isRun
isAim
```

Por tanto existen dos rutas nativas que convergen en el mismo estado de control del partner:

```text
A) actualización local/AI de uNpc
   ~0x027A0CA0
       |
       v
uNpc + 0x1670 / 0x1678 / 0x1680 / 0x1688 / 0x1689

B) control remoto por red
   cPlayerPadSyncData
       |
       v
0x027B1D70
       |
       v
los mismos campos del uNpc
```

Esto convierte la ruta B en una candidata mucho más limpia para cooperativo local: alimentar el paquete/handler con el segundo mando local en vez de depender de transporte de red.

---

# 1. RTTI y tipo

String:

```text
cPlayerPadSyncData
VA 0x04D98830
```

RTTI:

```text
.?AUcPlayerPadSyncData@chara@game@app@@
```

Vtable:

```text
0x04D98588
```

El objeto ocupa aproximadamente:

```text
0x28 bytes
```

---

# 2. Layout confirmado

Constructor/init alrededor de:

```text
0x02747440
```

Inicializa:

```text
+0x08 moveAnalog     vec2
+0x10 rotateAnalog   vec2
+0x18 aimAnalog      vec2
+0x20 waistRotateX   float
+0x24 isRun          bool
+0x25 isAim          bool
```

Existe además estado/base antes de +0x08; un string cercano `status` sugiere un miembro heredado/base alrededor de +0x04, pero no se necesita para identificar el payload de pad.

## Property registration

La rutina alrededor de:

```text
0x02747630
```

registra explícitamente:

```text
moveAnalog    +0x08
rotateAnalog  +0x10
aimAnalog     +0x18
waistRotateX  +0x20
isRun         +0x24
isAim         +0x25
```

Por tanto estos offsets no son una inferencia por forma: están respaldados por el propio metadata/debug del ejecutable.

---

# 3. Serialización

Método:

```text
0x02747960
```

serializa los seis campos anteriores.

Deserialización:

```text
0x02747B40
```

los recupera del stream.

Esto confirma que `cPlayerPadSyncData` es realmente el estado de entrada que se transmite entre peers.

---

# 4. Canal PlayerPad

String:

```text
PlayerPad
VA 0x04E10840
```

La tabla de red lo asocia al identificador:

```text
0x332D
```

Durante la inicialización de `uNpc`, alrededor de:

```text
0x0278724A
```

se crea un `cNetSyncData<uNpc>` para ese canal.

El constructor usado es:

```text
thunk 0x01C1AC89
 -> 0x0274B0D0
```

El objeto de sync guarda el owner `uNpc` en:

```text
cNetSyncData + 0x68
```

y el wrapper/miembro de red queda asociado al `uNpc` alrededor de:

```text
uNpc + 0x1898
```

---

# 5. Dispatcher de entrada

Vtable del tipo específico:

```text
cNetSyncData<uNpc>
vtable 0x04D98800
```

Dispatcher entrante:

```text
slot 8
thunk 0x01BA63B1
 -> 0x0274A680
```

La primera rama identificada compara el DTI del objeto recibido con el de:

```text
cPlayerPadSyncData
```

y crea un delegate hacia:

```text
thunk 0x01C6F4F0
 -> 0x0274C710
```

Ese delegate obtiene el owner desde:

```text
cNetSyncData + 0x68
```

y llama al handler del `uNpc`:

```text
thunk 0x01C18BE1
 -> 0x027B1D70
```

---

# 6. Handler 0x027B1D70: aplicación del pad remoto

Entrada:

```text
this = uNpc
arg1 = cPlayerPadSyncData*
```

Copias observadas:

```text
data +0x08 moveAnalog
 -> uNpc +0x1670

data +0x10 rotateAnalog
 -> uNpc +0x1678

data +0x18 aimAnalog
 -> uNpc +0x1680

data +0x24 isRun
 -> uNpc +0x1688

data +0x25 isAim
 -> uNpc +0x1689
```

`data+0x20 waistRotateX` se usa además en la lógica de orientación/cintura y en estado relacionado alrededor de `uNpc+0x18A0`.

## Coincidencia con la ruta local/AI

La rutina anteriormente estudiada:

```text
0x027A0CA0
```

también alimenta:

```text
uNpc +0x1670
uNpc +0x1678
uNpc +0x1680
uNpc +0x1688
uNpc +0x1689
```

Por tanto la convergencia es directa.

---

# 7. Implicación para el cooperativo local

La estrategia preferida pasa a ser:

```text
PadData[0] -> jugador principal normal

PadData[1]
   |
   v
construir/rellenar cPlayerPadSyncData
   |
   v
reutilizar 0x027B1D70
   |
   v
uNpc/partner existente
```

Ventajas frente a reimplementar el control:

1. reutiliza la representación de input que Capcom ya diseñó para el segundo jugador remoto;
2. reutiliza el mismo handler que el cooperativo online;
3. evita duplicar la lógica que aplica movimiento/aim/run al partner;
4. reduce la necesidad de adivinar offsets;
5. separa claramente la lectura física del segundo mando de la aplicación al actor.

## Todavía pendiente

- localizar el productor/sender original de `cPlayerPadSyncData`;
- ver cómo lee los controles del jugador que transmite;
- sustituir esa fuente por `sGamePad::PadData[1]` o invocar localmente el handler;
- identificar de forma fiable el `uNpc` que corresponde al partner local;
- impedir que AI/otras rutas sobrescriban los campos después;
- decidir qué modo de control del NPC debe activarse;
- validar cámara/HUD por separado.

## Estado

**CONFIRMADO a nivel estático:** existe una ruta nativa de pad remoto que aplica directamente los datos de entrada al compañero NPC.

**ACTIVA como estrategia:** reutilizar esa ruta con el segundo mando local.
