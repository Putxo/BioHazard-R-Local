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


---

# 8. Productor/sender nativo — 0x0274B200

También se localizó el lado emisor de `cPlayerPadSyncData`.

Rutina:

```text
0x0274B200
```

Construye un paquete temporal y lo rellena desde el estado del `uNpc` owner.

| Campo paquete | Getter | Origen |
|---|---|---|
| +0x08 moveAnalog | `0x01BF5416 -> 0x0208B7E0` | `uNpc+0x1670` |
| +0x10 rotateAnalog | `0x01C7EB03 -> 0x02703130` | `uNpc+0x1678` |
| +0x18 aimAnalog | `0x01C6253E -> 0x02728570` | `uNpc+0x1680` |
| +0x20 waistRotateX | `0x01C3B1AA -> 0x02089BC0` | primer float de `uNpc+0x16C0` |
| +0x24 isRun | `0x01C3DD56 -> 0x0272D790` | `uNpc+0x1688` |
| +0x25 isAim | `0x01BC4D43 -> 0x0208E9D0` | estado de aim calculado |

El paquete se compara contra una cache dentro de `cNetSyncData` a partir de `+0x70`. Si cambia —o se alcanza el refresco periódico— se envía por la ruta de red.

El único caller relevante localizado dentro de la actualización de control NPC es:

```text
0x027A1027
  -> thunk 0x01BED879
  -> 0x0274B200
```

Por tanto el pipeline remoto original queda reconstruido de extremo a extremo.

---

# 9. Relación con los modos de control del NPC

La rutina `0x027A0CA0` distingue varios modos.

Por comportamiento:

- modo 1 lee físicamente `sGamePad` y después puede enviar `cPlayerPadSyncData`;
- modo 3 no vuelve a generar los vectores principales, lo que encaja con que los reciba por el handler remoto `0x027B1D70`.

Existe metadata/debug con el nombre `ThinkModes`, pero las etiquetas numéricas exactas todavía no se han recuperado.

Por eso se registra como **hipótesis fuerte**, no como enum nombrado confirmado.

---

# 10. Limitación PC: el selector de pad se ignora

Las APIs de `sGamePad` llamadas por el modo local reciben un selector de pad. Sin embargo, las implementaciones PC inspeccionadas ignoran ese argumento y usan siempre:

```text
sGamePad + 0x970 = mStartPadNo
```

Métodos:

```text
0x02DB1570 move analog
0x02DB1740 aim analog
0x02DB1910 rotate analog
0x02DB1C90 alternate rotate
0x02DAFFB0 run/action bool
0x02DAF3C0 aim bool
```

Esto explica por qué el primer experimento que cambió el selector 0→1 no podía, por sí solo, escoger físicamente el segundo PadData.

## Nueva ruta preferida

Restaurar el uso del argumento selector en estos getters PC y después hacer que únicamente el partner convertido a local solicite índice 1.

Esta modificación aprovecha una interfaz que ya existe en el motor en lugar de introducir un global nuevo.
