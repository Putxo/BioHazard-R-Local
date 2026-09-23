# 06 — Experimentos estáticos producidos en esta conversación

Los ejecutables experimentales **no se suben a GitHub**. Aquí se registra todo lo necesario para identificarlos y entender qué se modificó.

Ninguno de estos experimentos se considera cooperativo local funcional hasta probarlo dentro del juego.

---

# Experimento 1 — P2 INPUT EXPERIMENTAL

Archivo local generado:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT EXPERIMENTAL.exe`

Base:

`BioRevHD 30-Enero-2013.exe`

SHA-256 base:

`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`

SHA-256 experimental:

`6c55444a820d05d0b0a15cafad0e8e7d0ec5bc67c1a548631a47d9ef2e6f0729`

Estado:

**STATICALLY VERIFIED ONLY — no se ejecutó gameplay de Windows en el entorno de análisis.**

## Objetivo

Probar de forma mínima la hipótesis de que una ruta secundaria de jugador pueda volver a entrar en la lectura local de input y obtener el segundo índice lógico de pad.

## Parches

```text
mode_redirect_jne:
  0x27A0CE7 -> 0x27A1318

mode_redirect_stub:
  0x27A1318
  modos inferidos 2/3 -> ruta local de input
  otros modos        -> ruta non-local original

pad_selector_stub:
  0x27A1340
  modos inferidos 2/3 -> pad index 1
  modo 1/otros        -> pad index 0

pad_selector_calls:
  0x27A0D03
  0x27A0D49
  0x27A0D6D
  0x27A0DBF
  0x27A0E29
  0x27A0E8A
```

## Diff exacto contra el original

Tamaño del archivo sin cambios.

79 bytes diferentes, repartidos en 10 rangos:

```text
0x00C280E9-0x00C280EA   2 bytes
0x00C28104-0x00C28107   4 bytes
0x00C2814A-0x00C2814D   4 bytes
0x00C2816E-0x00C28171   4 bytes
0x00C281C0-0x00C281C3   4 bytes
0x00C2822A-0x00C2822D   4 bytes
0x00C2828B-0x00C2828E   4 bytes
0x00C28718-0x00C2871C   5 bytes
0x00C2871E-0x00C2872E  17 bytes
0x00C28740-0x00C2875E  31 bytes
```

Cambios de bytes observados en el análisis:

```text
0x00C280E9-0x00C280EA
  9303 -> 2b06

0x00C28104-0x00C28107
  3eba4cff -> 38060000

0x00C2814A-0x00C2814D
  f8b94cff -> f2050000

0x00C2816E-0x00C28171
  d4b94cff -> ce050000

0x00C281C0-0x00C281C3
  82b94cff -> 7c050000

0x00C2822A-0x00C2822D
  18b94cff -> 12050000

0x00C2828B-0x00C2828E
  b7b84cff -> b1040000

0x00C28718-0x00C2871C
  cccccccccc -> 83f8020f84

0x00C2871E-0x00C2872E
  CC padding -> f9ffff83f8030f84c3f9ffffe951fdffff

0x00C28740-0x00C2875E
  CC padding -> e8df0149ff8bc8e8bf523eff83f802740883f803740333c0c3b801000000c3
```

## Por qué no está validado

La semántica exacta de los modos internos 1/2/3 fue inferida por flujo de control, no recuperada de símbolos. El parche demuestra solo que puede reencaminarse la selección; no demuestra que el actor Sub/P2 esté vivo ni que el motor le entregue ese modo durante una partida real.

---

# Experimento 2 — SPLITSCREEN EXPERIMENTAL v2

Archivo local generado:

`BioRevHD 30-Enero-2013 LOCAL COOP SPLITSCREEN EXPERIMENTAL v2.exe`

SHA-256:

`9952e14daedfa05afceea6e1456f7d18aa1ed071ea900c7aef2cc7c8eeeec089`

Estado:

**STATICALLY VERIFIED ONLY — no se ejecutó gameplay de Windows en el entorno de análisis.**

## Por qué se construyó

A estas alturas del chat ya se había comprobado que enero conserva:

- gestores Self/Partner;
- infraestructura de ocho Viewports;
- rutas debug de varios jugadores/cooperación;
- campos `uCameraManage::mPadNo`;
- una ruta de input susceptible de seleccionar otro pad.

El objetivo era probar estáticamente una configuración de dos cámaras/dos viewports sin afirmar todavía que P2 funcionase.

## Incluye el experimento de input anterior

Conserva los 79 bytes del experimento 1.

## Parches añadidos de cámara/viewport

```text
hook:
  0x203E3A9 -> 0x203E3BD

code cave:
  0x203E3BD..0x203E44B
  manifest original: 143 bytes de cave

camera managers:
  Self    = sGameCamera + 0xCE0
  Partner = sGameCamera + 0xCE4

mPadNo:
  Self    = 0
  Partner = 1

Viewport 0:
  camera  = Self
  visible = 1
  mode    = TOP (2)
  display = 0

Viewport 1:
  camera  = Partner
  visible = 1
  mode    = BOTTOM (3)
  display = 0

Viewport 4:
  no se toca
  motivo: parecía ligado a la ruta stock uFreeCamera/debug

activación:
  llamada a implementación ~0x2069AB0 para ambos uCameraManage
```

## Diff contra el original de enero

242 bytes diferentes, en 11 rangos.

El bloque nuevo de split-screen ocupa:

```text
0x004C57A9-0x004C584B   163 bytes diferentes
```

más los 79 bytes del experimento de input.

## Limitaciones conocidas en el momento de crearlo

- No demuestra que todas las escenas de campaña creen o mantengan un Player2/Sub0 controlable.
- Los modos de input 1/2/3 siguen siendo semántica inferida.
- Partner Camera puede no tener una cámara activa en una escena y producir viewport negro.
- El motor puede sobrescribir posteriormente la configuración del viewport.
- No se ha validado HUD independiente.
- No se ha validado inventario/pausa por jugador.
- No se han validado QTE, puertas, escaleras, interacciones, muerte/revive, checkpoints ni transiciones.
- No se ha demostrado todavía que la IA deje de escribir sobre el actor Sub/P2.
- No hubo ejecución del juego en Windows dentro del entorno de análisis.

## Manifests originales

Los JSON exactos generados durante esta conversación se conservan en:

- `research/manifests/p2-input-experimental.json`
- `research/manifests/splitscreen-v2-experimental.json`


---

# Experimento 3 — NATIVE INPUT EXPERIMENTAL v3

Archivo local generado:

`BioRevHD 30-Enero-2013 LOCAL COOP NATIVE INPUT EXPERIMENTAL v3.exe`

Base directa:

`BioRevHD 30-Enero-2013.exe`

SHA-256 base:

`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`

SHA-256 v3:

`9de444e104f2846aa166e1460f33a110ffac5b880f374edfd31b94aef3e5c649`

Estado:

**STATICALLY VERIFIED ONLY — todavía no se ha ejecutado gameplay de Windows en este entorno.**

## Por qué existe v3

Los experimentos 1/2 trataban los modos internos 2/3 como posibles rutas de segundo jugador. Después se recuperaron los nombres y comportamiento reales:

```text
1 = ThinkMode::Pad
2 = ThinkMode::Network
3 = ThinkMode::Cpu
```

Por tanto v3 se rehízo **desde el EXE original**, sin heredar los parches heurísticos anteriores.

## Diseño

### ThinkMode

En `0x027A0CE4` se desvía el dispatch a una cave en `0x027A1318`.

La cave conserva:

```text
Pad (1)     -> ruta local stock
Network (2) -> ruta Network stock
Cpu (3):
    Self    -> ruta CPU stock
    Partner -> setter nativo ThinkMode::Pad -> ruta local stock
otros       -> ruta stock no-local
```

Para Partner usa:

```text
Self predicate:
0x01BB3192 -> 0x01CB7560

ThinkMode wrapper:
0x01BB8B60 -> 0x0278CC40
```

El wrapper de ThinkMode es preferible a escribir `uPlayer+0xE40` directamente porque sincroniza los controladores internos y la ruta concreta hace el cleanup al abandonar CPU.

### Índice de pad

La función stock:

```text
0x01C6C746 -> 0x027A27B0
```

devolvía siempre 0.

v3 sustituye `0x027A27B0` por:

```text
Self    -> pad 0
Partner -> pad 1
```

usando el mismo predicado Self/Partner que utiliza el propio juego.

## Diff exacto

Mismo tamaño que el original.

```text
3 rangos modificados
73 bytes diferentes

0x00C280E4-0x00C280EC   9 bytes
0x00C28718-0x00C2874A  51 bytes
0x00C29BB0-0x00C29BBC  13 bytes
```

### Rango 1

```text
83f8010f8593030000
->
e92f06000090909090
```

### Rango 2

51 bytes de `CC` usados como cave:

```text
83f801742483f8030f85200000008b4df851e8631e41ff84c00f850f0000006a018b4df8e81f7841ffe9a7f9ffffe935fdffff
```

### Rango 3

```text
558bec81eccc00000053565751
->
51e8dc0941ff0fb6c083f001c3
```

## Verificación de desensamblado

Se comprobó con `objdump -D -Mintel` que:

- el jump de `0x027A0CE4` aterriza en `0x027A1318`;
- Pad vuelve a `0x027A0CED`;
- Network/otros vuelven a `0x027A1080`;
- Partner+Cpu llama a `0x01BB8B60` con argumento 1;
- el selector `0x027A27B0` llama a `0x01BB3192` y transforma el booleano en 0/1.

## Limitación importante

Si una escena/script fuerza deliberadamente al Partner a `ThinkMode::Cpu`, v3 intentará devolverlo a `Pad` cuando esta ruta de actualización de input se ejecute.

Eso puede ser correcto para gameplay cooperativo libre, pero probablemente necesitará excepciones para:

- cutscenes;
- secuencias guiadas;
- QTE/scripted movement;
- momentos en los que el partner debe quedar temporalmente bajo control de CPU.

Por eso v3 es una prueba de **propiedad de input nativa**, no todavía el parche final.

Manifest:

`research/manifests/native-input-v3-experimental.json`

Patcher reproducible:

`scripts/apply_native_input_v3.py`


---

# Experimento 3 — P2 INPUT v3

Archivo local:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT v3.exe`

SHA-256:

`ee05fc7d6c965aed0661165eb0ead4c6dffa1f67bc32fd758297edb86f3213e1`

Base:

`BioRevHD 30-Enero-2013.exe`

Estado:

**STATICALLY VERIFIED ONLY — no se ejecutó gameplay de Windows en el entorno de análisis.**

## Por qué existe v3

Los experimentos anteriores cambiaban el selector devuelto por la ruta NPC, pero se descubrió después que los getters PC de `sGamePad` ignoraban ese argumento y usaban siempre `mStartPadNo`.

v3 corrige ese problema en la fuente.

## Lógica

```text
ThinkMode 1
  -> ruta local original
  -> selector 0
  -> PadData 0

ThinkMode 2
  -> ruta original sin tocar

ThinkMode 3
  -> redirigido a la ruta local
  -> selector 1
  -> PadData 1
```

## Parches de flujo NPC

```text
0x027A0CE7
  JNE -> cave 0x027A1318

0x027A1318
  mode==3 -> 0x027A0CED (ruta local)
  otro non-1 -> 0x027A1080 (ruta original)

0x027A27D3
  hook -> 0x027A1340

0x027A1340
  obtiene ThinkMode
  devuelve 1 si mode==3
  devuelve 0 en los demás casos
```

## Parches sGamePad

Se sustituyen 12 cargas de `mStartPadNo` por el argumento selector ya recibido por la función:

```text
0x02DAF404 / 0x02DAF40E  aim bool
0x02DAFFE5 / 0x02DAFFEF  run bool

0x02DB15B7 / 0x02DB15C8  move analog
0x02DB1787 / 0x02DB1798  aim analog
0x02DB1957 / 0x02DB1968  rotate analog
0x02DB1CD7 / 0x02DB1CE8  alternate rotate analog
```

En getters analógicos:

```text
mov edx,[sGamePad+0x970]
```

pasa a:

```text
mov edx,[ebx+0x0C]
nop
nop
nop
```

En getters booleanos se usa el argumento `[ebp+0x08]` conservando el registro destino.

## Verificación binaria

```text
tamaño final = tamaño original
111 bytes diferentes
17 rangos contiguos
```

El desensamblado del resultado confirma:

- modo 1 mantiene fall-through original;
- modo 3 salta a la ruta local;
- modo 2 conserva ruta original;
- selector devuelve 1 solo para modo 3;
- los seis getters relevantes ya no cargan `mStartPadNo` en los puntos parcheados.

## Diferencia conceptual respecto a v1/v2

v1/v2 demostraban que podía cambiarse el flujo, pero no separaban de forma fiable el dispositivo físico porque la capa PC ignoraba el selector.

v3 restaura el selector en la propia implementación PC.

Manifest:

`research/manifests/p2-input-v3.json`


---

# Experimento 4 — LOCAL COOP v3 INPUT+SPLITSCREEN

Archivo local:

`BioRevHD 30-Enero-2013 LOCAL COOP v3 INPUT+SPLITSCREEN.exe`

SHA-256:

`6978dc6d63781718c3728160c0312737424536734c0850b9337e990fb2da677d`

Estado:

**STATICALLY VERIFIED ONLY.**

## Construcción

Se partió otra vez de la build original de enero.

Componentes:

1. **P2 INPUT v3**
   - modo 1 conserva ruta local original y selector 0;
   - modo 2 queda completamente intacto;
   - modo 3 se redirige a la ruta local y usa selector 1;
   - los seis getters PC de `sGamePad` respetan el selector.

2. **SPLITSCREEN**
   - se aplicó únicamente el bloque de cámara/viewport ya documentado del experimento split-screen v2;
   - no se arrastraron los parches de input antiguos de v2.

## Cámara/viewport aplicado

Ventana de diferencias copiada del experimento split-screen anterior:

```text
file 0x004C57A9..0x004C584B
163 bytes diferentes
VA hook  0x0203E3A9
VA cave  0x0203E3BD
```

Configura de forma experimental:

```text
Self CameraManage    mPadNo=0 -> Viewport 0 -> TOP
Partner CameraManage mPadNo=1 -> Viewport 1 -> BOTTOM
```

## Diff total contra el original

```text
same size: yes
different bytes: 274
contiguous ranges: 18
```

## Qué mejora respecto al split-screen v2 anterior

El split-screen v2 anterior contenía el input experimental viejo, que después se demostró insuficiente porque `sGamePad` ignoraba el selector.

Esta versión sustituye completamente aquella parte por el input v3 corregido.

## Pendiente de prueba

- que el partner esté creado como `uNpc` controlable en la escena;
- que ThinkMode 3 sea el estado que realmente usa ese partner en la sesión objetivo;
- que PadData índice 1 corresponda al segundo mando conectado;
- que Partner Camera tenga una cámara válida;
- que el juego no sobrescriba los Viewports después;
- HUD/UI;
- inventario y pausa;
- interacciones y QTE;
- muerte/revive;
- checkpoints y transiciones.

Manifest:

`research/manifests/local-coop-v3-input-splitscreen.json`


---

## CORRECCIÓN POSTERIOR — geometría de viewport en experimentos 2 y 4

El metadata de `sCamera::Viewport` demuestra que:

```text
mMode   = +0x13
mRegion = +0x18
```

Los experimentos anteriores escribieron 2/3 en `mMode`, no en `mRegion`.

Por ello las descripciones antiguas “TOP/BOTTOM” asociadas a esas escrituras son incorrectas.

Los EXE históricos y sus manifests **no se borran** porque forman parte del proceso solicitado por el usuario, pero quedan marcados como superseded para la geometría de split-screen.

La siguiente versión debe escribir el enum de región en:

```text
Viewport0 base sCamera+0x30  -> mRegion sCamera+0x48
Viewport1 base sCamera+0x1C0 -> mRegion sCamera+0x1D8
```
