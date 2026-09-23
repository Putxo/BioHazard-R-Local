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
