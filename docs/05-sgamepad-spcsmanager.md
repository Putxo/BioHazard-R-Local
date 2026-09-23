# 05 — sGamePad, sPcsManager y clases Main/Sub

## sGamePad

Strings contiguas:

```text
sGamePad
mIsUsePadEx
mStartPadNo
ExPadData
sGamePad::PadData
e:\bhr\source\biorevhd\prog\game\pad\sgamepadpc.cpp
```

Direcciones:

```text
sGamePad             0x04E11D87
mIsUsePadEx          0x04E11DC0
mStartPadNo          0x04E11DD0
sGamePad::PadData    0x04E12210
```

Constructor aproximado:

```text
0x02DAC0F0
```

Campo confirmado:

```text
sGamePad::mStartPadNo = +0x970
```

Inicializado a 0.

Regiones candidatas de PadData:

```text
+0x668
+0x7E8
```

La separación es `0x180`, consistente con dos elementos de `0xC0` bytes observados en la construcción.

Una rutina alrededor de `0x02DAE020` lee `mStartPadNo` y lo usa en acceso real al estado del mando.

---

## sPcsManager

Strings:

```text
Main Player
Sub Player 0
Sub Player 1
uPCS
mMoveSubPcs
mMovePcs
Use Pcs Buffer
mDebugPlayerSub1ID
mDebugPlayerSub0ID
mDebugPlayerMainID
mIsDebug
sPcsManager::uPcsFsm
sPcsManager
```

Enum observado:

```text
Main Player  -> 0
Sub Player 0 -> 1
Sub Player 1 -> 2
```

Campos:

```text
+0x1308 mIsDebug
+0x130C mDebugPlayerMainID
+0x1310 mDebugPlayerSub0ID
+0x1314 mDebugPlayerSub1ID
```

Getters:

```text
MainID  0x02DCDF10
Sub0ID  0x02DCDF50
Sub1ID  0x02DCDF90
```

Setters aproximados:

```text
mIsDebug ~0x02DD1460
MainID   ~0x02DD14C0
Sub0ID   ~0x02DD1520
Sub1ID   ~0x02DD1580
```

Los setters de los tres IDs convergen en:

```text
0x01BC1F58 -> 0x02DD16F0
```

La función común procesa consecutivamente los tres IDs Main/Sub0/Sub1.

---

## Clases RTTI diferenciadas

Se identificaron:

```text
app::game::pcs::uPcsPlayerMain
app::game::pcs::uPcsPlayerSub0
app::game::pcs::uPcsPlayerSub1
```

Este hallazgo es especialmente importante porque demuestra que Main/Sub0/Sub1 tienen representación de clase diferenciada en el ejecutable.

### Interpretación actual

La ruta más prometedora ya no es “crear un segundo jugador desde cero”, sino:

1. localizar constructor/vtable de `uPcsPlayerSub0`;
2. encontrar qué métodos difieren respecto a `uPcsPlayerMain`;
3. seguir esos métodos hasta la selección de input;
4. enlazar Sub0 con el segundo PadData;
5. después resolver CameraManage y viewport.

---

## mMovePcs / mMoveSubPcs

Xrefs aproximados:

```text
mMovePcs
  ~0x02DCB123
  ~0x02DCB2A1

mMoveSubPcs
  ~0x02DCB185
  ~0x02DCB2ED
```

Callbacks/getters observados en la zona:

```text
~0x02DCB370
~0x02DCB820
~0x02DCB3B0
~0x02DCB8C0
```

Todavía no se etiqueta su semántica final.

---

## Próxima investigación

Prioridad inmediata:

```text
uPcsPlayerMain
uPcsPlayerSub0
uPcsPlayerSub1
   ↓
constructores / vtables / overrides
   ↓
ruta de movimiento e input
   ↓
sGamePad::PadData[index]
```

La siguiente confirmación importante debe ser una diferencia concreta de método entre Main y Sub0 que permita identificar cómo el juego decide qué input alimenta a cada clase.


---

## Corrección: los getters PC ignoran el selector recibido

La API de entrada llamada desde `uNpc` pasa un índice/selector a los getters de `sGamePad`.

Sin embargo, la implementación PC de enero ignora ese argumento y consulta `mStartPadNo @ +0x970`.

Métodos confirmados:

```text
0x02DB1570  move analog
0x02DB1740  aim analog
0x02DB1910  rotate analog
0x02DB1C90  alternate rotate analog
0x02DAFFB0  run/action boolean
0x02DAF3C0  aim boolean
```

Mapeo desde la ruta NPC:

```text
0x027A0Dxx -> 0x02DB1570 -> uNpc+0x1670 move
0x027A0Dxx -> 0x02DB1910/0x02DB1C90 -> uNpc+0x1678 rotate
0x027A0Dxx -> 0x02DB1740 -> uNpc+0x1680 aim
0x027A0E26 -> 0x02DAFFB0 -> uNpc+0x1688 run
0x027A0E87 -> 0x02DAF3C0 -> uNpc+0x1689 aim flag
```

### Implicación

El experimento anterior que modificaba el selector devuelto por `0x027A27B0` no podía separar físicamente Pad 1/Pad 2 mientras estos getters siguieran forzando el global.

El siguiente experimento debe hacer que las implementaciones respeten el selector que ya reciben.


---

## Confirmación: el selector indexa exactamente dos PadData

Se siguió el helper usado por los getters de `sGamePad`.

Thunk:

```text
0x01C4F385 -> 0x02DBE720
```

La implementación contiene:

```asm
cmp  dword ptr [ebp+8], 2
jb   valid_index
...
valid_index:
mov  eax,[ebp+8]
imul eax,eax,0xC0
mov  ecx,[ebp-8]
add  eax,[ecx]
ret  4
```

Por tanto:

- el índice válido está limitado explícitamente a `0..1`;
- cada elemento ocupa `0xC0` bytes;
- índice 0 e índice 1 seleccionan dos elementos distintos.

Esto encaja con la construcción observada anteriormente de dos estructuras de `0xC0`.

### Consecuencia

Una vez que los getters PC dejan de sustituir el argumento por `mStartPadNo`, el selector 1 **sí llega a una segunda entrada real de PadData**.

Todavía falta la prueba física en Windows de qué dispositivo conectado alimenta PadData[1], pero la separación interna de dos slots está confirmada.


---

## Dos objetos low-level sPad::Pad

La existencia de dos índices no se limita a `PadData`.

### Punteros

```text
sGamePad + 0x968 + index*4
index 0..1
```

El constructor de `sGamePad` inicializa exactamente dos punteros a null.

La ruta runtime `0x02DACA76..0x02DACADC` crea, si es necesario, dos objetos de `0x2F8` bytes.

Constructor:

```text
0x01C01EAF -> 0x03358F60
```

Vtable:

```text
0x04EE8E4C
```

RTTI:

```text
.?AVPad@sPad@@
```

### Actualización

`0x02DAC73E..0x02DAC77A` itera los dos objetos y copia estado desde:

```text
sPad::Pad + 0x15C
```

a:

```text
sGamePad + 0x198 + index*0x2F8
```

Después el flujo también procesa dos `PadData` de `0xC0` en:

```text
sGamePad + 0x668 + index*0xC0
```

Esto confirma que el backend PC está estructurado realmente para dos pads internos independientes.


---

## Restauración FULLPAD v7

La corrección de `mStartPadNo` no se limita a los seis getters inicialmente estudiados.

El selector nativo de `uNpc` tiene **129 call sites** y alcanza múltiples APIs de gameplay.

### Cobertura de cargas que colapsaban el selector

En los consumidores relevantes se localizaron:

```text
67 cargas estándar de sGamePad+0x970
8 cargas analógicas de frame alineado
TOTAL: 75
```

v6 ya corregía:

```text
4 estándar:
  aim bool x2
  run bool x2

8 analógicas:
  move x2
  aim analog x2
  rotate x2
  alternate rotate x2
```

v7 añade las **63 estándar restantes**.

Después de v7:

```text
0/67 cargas estándar conocidas siguen leyendo mStartPadNo
8/8 cargas analógicas usan el selector restaurado
```

En métodos estándar el selector está en:

```text
[ebp+8]
```

La transformación in-place mantiene el registro destino:

```text
mov reg,[...+0x970]  ; 6 bytes
->
mov reg,[ebp+8]      ; 3 bytes
nop
nop
nop
```

La lista completa de las 63 VAs nuevas y las comprobaciones de bytes están en:

`patches/build_v7_fullpad.py`

### Implicación

La separación PadData[0]/PadData[1] queda restaurada no solo para movimiento, sino también para las APIs de botones/acciones identificadas en la ruta de gameplay de `uNpc`.
