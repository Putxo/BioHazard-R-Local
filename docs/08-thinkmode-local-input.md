# 08 — ThinkMode y corrección del enfoque de input local

Build: **BioRevHD 30-Enero-2013.exe**.

Este documento continúa únicamente la investigación realizada en esta conversación.

## 1. El selector 1/2/3 de la ruta de input ya tiene nombre

En la zona de `scharabase.cpp` aparecen explícitamente las cadenas:

```text
ThinkMode::Network
ThinkMode::Cpu
ThinkMode::Pad
ThinkMode::Invalid
uCallThink
stateManger
```

Direcciones/raw observados:

```text
raw 0x322AE98  "network"
raw 0x322AEA4  "e:\bhr\source\biorevhd\prog\game\chara\scharabase.cpp"
raw 0x322AF18  "sCharaBase"
raw 0x322AF38  "ThinkMode::Network"
raw 0x322AF50  "ThinkMode::Cpu"
raw 0x322AF64  "ThinkMode::Pad"
raw 0x322AF78  "ThinkMode::Invalid"
raw 0x322AFFC  "stateManger"
raw 0x322B00C  "uCallThink"
```

El flujo ejecutable permite mapear los valores:

```text
0 = ThinkMode::Invalid
1 = ThinkMode::Pad
2 = ThinkMode::Network
3 = ThinkMode::Cpu
```

## 2. La rutina de input de uPlayer confirma el significado

Rutina aproximada:

```text
0x027A0CA0
```

Obtiene el ThinkMode y bifurca por valor.

### Mode 1 — Pad

La rama desde aproximadamente `0x027A0D00`:

- llama repetidamente al selector/getPadNo-like;
- consulta `sGamePad`;
- escribe input de gameplay en campos de uPlayer:
  - movimiento alrededor de `+0x1670`;
  - rotación alrededor de `+0x1678`;
  - aim alrededor de `+0x1680`;
  - flags alrededor de `+0x1688/+0x1689`.

Esto identifica el modo 1 como lectura local de pad.

### Mode 2 — Network

La rama alrededor de `0x027A1080` usa un objeto desde `this+0x1BE0`.

Se localizaron RTTI/campos de:

```text
cPlayerPadSyncData
moveAnalog
rotateAnalog
aimAnalog
waistRotateX
isAim
isRun
status
```

La rama extrae precisamente este tipo de estado sincronizado.

Conclusión:

```text
ThinkMode 2 = Network
```

### Mode 3 — Cpu

La tercera rama se ejecuta cuando el modo es 3 y utiliza una ruta distinta del pad local y del paquete sincronizado.

Junto con la string explícita `ThinkMode::Cpu`, queda mapeado como CPU/AI.

## 3. uCallThink contiene mThinkMode explícitamente

Cadena de acceso:

```text
0x01C31524 -> 0x01D46310
0x01D46310:
  add ecx,0xE90
  call 0x01B94710 -> 0x01D46B60
```

El miembro `uPlayer+0xE90` conduce a un:

```text
app::game::chara::cCharaStateManager
```

Constructor:

```text
~0x0207E980
vtable 0x04CF4AD8
size observado ~0x598
```

Dentro de este manager, `+0x524` almacena un puntero a un objeto de tamaño `0x38`.

Constructor de ese objeto:

```text
thunk 0x01B8B561
 -> 0x027E8BD0
vtable 0x04DA3F94
```

RTTI:

```text
.?AVuCallThink@chara@game@app@@
```

El constructor inicializa:

```text
uCallThink +0x30 = 0
uCallThink +0x34 = 0
```

La función de registro de propiedades alrededor de `0x027E8EE0` asocia explícitamente:

```text
+0x30 -> "stateManger"
+0x34 -> "mThinkMode"
```

Por tanto:

> **uCallThink + 0x34 = mThinkMode** está confirmado directamente por metadata de la propia build.

## 4. Getter exacto de mThinkMode

Cadena:

```text
0x01B8660B -> 0x020806C0
```

Si `cCharaStateManager+0x524` es nulo devuelve 0.

Si existe:

```text
0x020806C0 -> 0x01BA3E8B -> 0x02080750
```

`0x02080750` hace esencialmente:

```asm
mov eax,[ecx+0x34]
ret
```

Es decir, devuelve `uCallThink::mThinkMode`.

## 5. uPlayer mantiene una copia del ThinkMode en +0xE40

En el constructor de uPlayer, alrededor de:

```text
0x027E9A8D
```

se observa:

```asm
mov dword ptr [this+0xE40],2
```

Por tanto el valor inicial es:

```text
ThinkMode::Network
```

Más tarde, el código de inicialización hace una llamada virtual en offset de vtable:

```text
+0x110
```

pasando el modo almacenado.

La entrada de la vtable conduce mediante:

```text
0x01BB8B60 -> 0x0278CC40
```

a una función que propaga el modo a los gestores de estado.

## 6. Setter de ThinkMode de uPlayer

Ruta principal:

```text
0x01BE3257 -> 0x027F1290
```

Comportamiento observado:

1. comprueba el valor actual en `uPlayer+0xE40`;
2. contiene tratamiento especial al salir de modo 3/CPU;
3. obtiene `cCharaStateManager`;
4. propaga el nuevo modo;
5. escribe el nuevo valor en `uPlayer+0xE40`.

Por tanto:

```text
uPlayer+0xE40 = copia/cache del ThinkMode
```

El wrapper virtual alrededor de `0x0278CC40` también actualiza gestores de estado adicionales del jugador.

## 7. Setter dentro de cCharaStateManager

Función relevante:

```text
~0x02080240
```

Compara el modo actual y termina propagando el nuevo modo al `uCallThink` instalado en `cCharaStateManager+0x524`.

El `uCallThink` se instala mediante una ruta alrededor de:

```text
0x0207FB60
thunk 0x01B978F2
```

Durante inicialización de uPlayer se crea un objeto de 0x38 bytes, se construye como `uCallThink` y se asigna al manager.

## 8. Corrección importante del primer experimento P2

El experimento:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT EXPERIMENTAL.exe`

había hecho una prueba exploratoria:

```text
mode 2/3 -> reentrar en la rama local de pad
mode 2/3 -> seleccionar pad 1
```

Ahora sabemos que conceptualmente eso no es la arquitectura correcta:

```text
mode 2 = Network
mode 3 = Cpu
```

Redirigir esas ramas a Pad destruye deliberadamente la semántica de control que el motor usa para jugadores remotos y CPU.

### Enfoque correcto

El objetivo pasa a ser:

```text
Main:
  ThinkMode::Pad (1)
  padNo = 0
  sGamePad -> PadData[0]

Sub0 / Player1:
  ThinkMode::Pad (1)
  padNo = 1
  sGamePad -> PadData[1]
```

No:

```text
Sub0 en Network/Cpu
  -> secuestrar rama 2/3
  -> fingir que es Pad
```

Por tanto el experimento anterior queda marcado como:

**exploratorio, incompleto y arquitectónicamente reemplazado por la nueva ruta ThinkMode::Pad + pad index nativo.**

## 9. Próximo objetivo inmediato

Encontrar quién cambia durante spawn/setup:

```text
Main:
  default Network(2) -> Pad(1)

Sub0:
  Network(2) o Cpu(3) -> [debe convertirse en Pad(1) para co-op local]
```

La siguiente búsqueda se centrará en callsites virtuales del setter de ThinkMode, especialmente la entrada de vtable `+0x110`, y en el código que crea/asigna Main/Sub0.

Una vez localizado ese punto, el parche correcto debería poder activar Pad para ambos jugadores sin secuestrar la lógica Network/Cpu.
