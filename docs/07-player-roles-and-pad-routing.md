# 07 — Roles Main/Sub, uPcsActor y ruta real del índice de pad

Build: **BioRevHD 30-Enero-2013.exe**.

Este documento continúa únicamente la investigación de esta conversación.

## 1. Vtables de uPcsPlayerMain / Sub0 / Sub1

RTTI confirmado:

```text
app::game::pcs::uPcsPlayerMain
app::game::pcs::uPcsPlayerSub0
app::game::pcs::uPcsPlayerSub1
```

Vtables:

```text
uPcsPlayerMain  0x04E1642C
uPcsPlayerSub0  0x04E1649C
uPcsPlayerSub1  0x04E1650C
```

La clase base identificada mediante RTTI es:

```text
app::game::pcs::uPcsChara
vtable 0x04E1631C
```

Los tres derivados tienen 21 slots virtuales y solo difieren en 3:

- slot 0: destructor específico;
- slot 4: devuelve descriptor/static data específico de clase;
- slot 5: implementación casi idéntica, pero fija el rol 0/1/2.

### Slot 5

Implementaciones reales:

```text
Main  0x02DF5D60 -> role 0
Sub0  0x02DF6380 -> role 1
Sub1  0x02DF69A0 -> role 2
```

El método:

1. ejecuta lógica base;
2. usa `this+0x30` como identificador;
3. llama a `sPcsManager` con el rol fijo 0/1/2;
4. recibe el ID correspondiente a ese rol;
5. lo aplica/almacena mediante una ruta que acaba escribiendo el valor en `this+0x40`.

Esto confirma que Main/Sub0/Sub1 son roles funcionales nativos, no etiquetas de debug.

## 2. Lookup de rol en sPcsManager

Implementación relevante:

```text
0x02DD0080
```

Firma inferida:

```text
lookup(group_or_context_id, role)
role:
  0 = Main
  1 = Sub0
  2 = Sub1
```

Comportamiento observado:

- rechaza roles >= 3;
- si `mIsDebug` está activo, lee directamente:
  - `+0x130C` Main
  - `+0x1310` Sub0
  - `+0x1314` Sub1
- si no está en debug:
  - busca hasta 20 entradas;
  - compara el ID de entrada;
  - usa una tabla de 3 IDs por entrada;
  - devuelve el ID asociado al rol pedido.

Por tanto existe una separación nativa y persistente de los tres roles PCS.

## 3. Nuevas especializaciones uPcsActor

RTTI encontrado:

```text
uPcsActor<SubPlayer0>
uPcsActor<SubPlayer1>
uPcsActor<Player0>
uPcsActor<Player1>
uPcsActor<Player2>
```

Vtables de las especializaciones Player:

```text
uPcsActor<Player0>  0x04E166CC
uPcsActor<Player1>  0x04E1673C
uPcsActor<Player2>  0x04E167AC
```

Como ocurre con uPcsPlayerMain/Sub0/Sub1, estas tres vtables tienen 21 slots y solo cambian 0, 4 y 5.

El slot 5 vuelve a usar constantes fijas:

```text
Player0 -> role 0
Player1 -> role 1
Player2 -> role 2
```

Esto refuerza que el sistema PCS transporta explícitamente la identidad Main/Sub a través de tipos C++ distintos.

## 4. Corrección: mMovePcs / mMoveSubPcs

La interpretación anterior de estos nombres como posibles rutas de "movimiento" era demasiado literal.

Strings japonesas adyacentes:

```text
mMovePcs     -> PCS番号
mMoveSubPcs  -> サブPCS番号
```

Traducción:

```text
PCS番号      = número de PCS
サブPCS番号  = número de sub-PCS
```

Por tanto estas propiedades se tratan ahora como selectores/debug de número de PCS, no como evidencia directa de input de movimiento.

**Estado anterior:** posible ruta de movimiento.

**Estado actual:** descartadas como hook principal de input.

## 5. uPlayer::getPadNo() / selector equivalente

La función llamada masivamente desde código de `uplayer.cpp` mediante thunk:

```text
thunk 0x01C6C746
 -> implementation 0x027A27B0
```

tiene esta semántica efectiva en la build de enero:

```cpp
int getPadNo_like() {
    return 0;
}
```

La implementación solo hace:

```asm
xor eax,eax
ret
```

Hay al menos 129 call sites al thunk dentro de código de gameplay.

Las referencias al source path:

```text
e:\bhr\source\biorevhd\prog\game\chara\player\uplayer.cpp
```

rodean esta zona de código, por lo que la atribución al sistema uPlayer tiene alta confianza.

Esto explica una parte del bloqueo: el gameplay siempre propone pad 0.

## 6. Hallazgo crítico en sGamePad: el argumento de pad existe pero se ignora

Patrón observado repetidamente en llamadas de uPlayer:

```asm
call getPadNo_like
push eax              ; pad number propuesto
push output
call get_sGamePad
mov ecx,eax
call sGamePad_method
```

Sin embargo, varias implementaciones `sGamePad_method` estudiadas **no usan el argumento de pad recibido**. En su lugar leen:

```text
sGamePad + 0x970 = mStartPadNo
```

Ejemplos:

### 0x02DB1570

Recibe dos argumentos y termina con `ret 8`, pero para seleccionar `PadData` usa:

```asm
mov edx,[ecx+0x970]
...
add eax,0x668
...
```

El índice pasado por el caller no se usa como selector del PadData.

### 0x02DB1910

Mismo patrón:

```text
lee +0x970
indexa PadData con mStartPadNo
ignora el índice de pad que el gameplay había pasado
```

### 0x02DB1C90

Mismo patrón.

### 0x02DAF000

También recibe un argumento desde `getPadNo_like`, pero internamente vuelve a leer:

```asm
mov ecx,[this+0x970]
```

y usa ese valor para consultar el estado.

## Interpretación

La interfaz de gameplay conserva un parámetro de número de pad, pero la implementación PC de `sGamePad` fuerza numerosas consultas al global `mStartPadNo`.

Esta es una explicación mucho más sólida para el comportamiento 1P que el experimento heurístico anterior.

## 7. sGamePad sí mantiene dos mandos simultáneamente

Constructor `sGamePad` aproximado:

```text
0x02DAC0F0
```

Se construyen dos conjuntos de datos con count=2.

Bloque expuesto como `ExPadData`:

```text
base +0x668
2 elementos
0xC0 bytes por elemento
```

El acceso indexado real:

```text
0x02DBE670
```

hace bounds check con índice < 2 y devuelve:

```text
base + index * 0xC0
```

Además:

```text
entry 0 +0xA0 = 0
entry 1 +0xA0 = 1
```

y la actualización de sGamePad procesa dos estados físicos separados.

Conclusión confirmada:

> La build de enero ya mantiene dos pads físicos/lógicos. No hace falta implementar desde cero soporte de segundo mando.

El problema a resolver es el puente:

```text
rol Main/Sub0
        ↓
uPlayer correspondiente
        ↓
getPadNo = 0/1
        ↓
sGamePad debe respetar ese argumento
        ↓
PadData[0]/PadData[1]
```

## 8. Consecuencia para el experimento anterior

El experimento `P2 INPUT EXPERIMENTAL` reemplazaba llamadas a `getPadNo_like` por un selector heurístico de 0/1 basado en modos internos.

Ahora sabemos que eso solo atacaba una mitad del problema:

1. `getPadNo_like` sí devuelve siempre 0;
2. pero además múltiples métodos de `sGamePad` ignoran el número recibido y vuelven a `mStartPadNo`.

Por tanto ese experimento queda aún más claramente clasificado como:

**prueba exploratoria incompleta, no solución de input P2**.

## 9. Próximo objetivo

Encontrar un identificador estable del rol del `uPlayer` real.

Dos rutas candidatas:

1. enlazar cada `uPlayer` con el ID de Main/Sub0 que ya mantiene `sPcsManager`;
2. localizar un campo/función existente que identifique Player0/Player1 en el actor.

Después:

- hacer que `getPadNo_like` devuelva 0 para Main y 1 para Sub0;
- restaurar en las APIs de `sGamePad` el uso del argumento de pad, en vez de `mStartPadNo`;
- validar input P1/P2 antes de volver a cámara/split-screen.
