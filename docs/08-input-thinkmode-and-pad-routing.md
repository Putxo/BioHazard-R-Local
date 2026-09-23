# 08 — ThinkMode, ruta real de input y selección de pad

Build de referencia: **BioRevHD 30-Enero-2013.exe**.

Este documento continúa exclusivamente la investigación de esta conversación.

## 1. Corrección: mStartPadNo no es la solución para P1/P2 simultáneos

Campo confirmado:

```text
sGamePad + 0x970 = mStartPadNo
```

Setter real:

```text
0x02AF4200
thunk: 0x01BDE1A8
```

Núcleo:

```asm
mov eax,[this]
mov ecx,[arg]
mov [eax+0x970],ecx
ret 4
```

Uno de los llamadores importantes está dentro de `0x02CA09A0`.

Esa función recorre:

```text
i = 0..1
```

consulta actividad/estado de cada mando y, cuando corresponde, ejecuta:

```text
sGamePad::setStartPadNo(i)
```

Call site observado:

```text
0x02CA0B95 push i
0x02CA0B99 get sGamePad
0x02CA0BA0 call 0x01BDE1A8
```

**Conclusión:** `mStartPadNo` selecciona el mando principal/inicial de la aplicación. No es el mecanismo adecuado para hacer que dos jugadores consuman simultáneamente PadData 0 y PadData 1.

Estado de la hipótesis anterior “usar mStartPadNo para P2”: **DESCARTADA**.

---

## 2. Ruta real de input de uPlayer

Rutina principal estudiada:

```text
0x027A0CA0
```

Actualiza directamente estado de gameplay de `uPlayer`, entre otros:

```text
uPlayer + 0x1670  mMoveAnalog
uPlayer + 0x1678  mRotateAnalog
uPlayer + 0x1680  mAimAnalog
uPlayer + 0x1688  flag de acción
uPlayer + 0x1689  flag de acción
```

En la ruta de entrada local, antes de cada consulta a `sGamePad`, hace:

```asm
mov  ecx,this
call 0x01C6C746
push eax
call <get sGamePad>
call <getter concreto>
```

Thunk:

```text
0x01C6C746 -> 0x027A27B0
```

Implementación actual de `0x027A27B0`:

```asm
...
xor eax,eax
ret
```

**CONFIRMADO:** el selector lógico de pad de `uPlayer` devuelve siempre **0** en esta build.

Esto explica por qué, aunque haya dos PadData, el gameplay local normal del jugador acaba consultando el pad lógico 0.

---

## 3. La función 0x027A0CA0 ya separa tres fuentes de control

Antes de leer los analógicos, `0x027A0CA0` obtiene el modo de control actual.

Flujo observado:

```text
modo == 1 -> ruta local sGamePad
modo == 2 -> datos/buffer en uPlayer+0x1BE0
modo == 3 -> ruta separada CPU/AI
```

La ruta 2 no debe reinterpretarse como “segundo jugador local”: consume un buffer distinto del PadData local.

Por tanto el primer experimento que redirigía indistintamente modos 2 y 3 hacia entrada local queda **SUPERADO/DEPRECADO** como diseño.

---

## 4. ThinkMode: nombres conservados en sCharaBase

Strings encontradas junto a:

```text
e:\bhr\source\biorevhd\prog\game\chara\scharabase.cpp
sCharaBase
```

son:

```text
ThinkMode::Network
ThinkMode::Cpu
ThinkMode::Pad
ThinkMode::Invalid
uCallThink
```

El comportamiento de `0x027A0CA0` permite mapear:

```text
1 = ThinkMode::Pad
2 = ThinkMode::Network
3 = ThinkMode::Cpu
0 = Invalid / sin control válido (consistente con checks posteriores)
```

La evidencia no es solo el orden de strings: la rama 1 usa `sGamePad`, la 2 usa el buffer de red y la 3 entra en lógica de CPU.

---

## 5. ThinkMode almacenado en uPlayer

Getter:

```text
thunk 0x01C84490 -> 0x01EFD5B0
```

devuelve:

```text
[uPlayer + 0xE40]
```

El constructor relacionado alrededor de:

```text
0x027E9A00
```

inicializa:

```text
uPlayer+0xE3C = -1   // SerialID observado en otras rutas
uPlayer+0xE40 = 2
```

La inicialización posterior propaga `+0xE40` al objeto `uCallThink`.

---

## 6. Setter nativo de ThinkMode

Implementación:

```text
0x027F1290
thunk 0x01BE3257
```

Está presente además en la vtable de `uPlayer` mediante un wrapper virtual:

```text
vtable uPlayer: slot 68
thunk 0x01BB8B60 -> 0x0278CC40
```

`0x0278CC40` llama al setter de `uPlayer` y sincroniza otro controlador interno relacionado.

### Comportamiento importante al abandonar CPU

`0x027F1290` comprueba:

```text
si modo actual == 3 (Cpu)
y nuevo modo != 3
```

y ejecuta limpieza sobre una estructura en:

```text
uPlayer + 0xEDC
```

antes de aplicar el nuevo modo.

Después:

1. aplica el nuevo modo al `uCallThink`;
2. guarda el nuevo valor en `uPlayer+0xE40`.

**Consecuencia:** para convertir Partner de CPU a control local conviene usar esta ruta nativa, porque ya contiene el cleanup requerido al salir de CPU.

---

## 7. Inicialización mediante el slot virtual 68

En la inicialización de `uPlayer`, alrededor de:

```text
0x027EDD05
```

el juego carga:

```text
[uPlayer+0xE40]
```

y llama virtualmente:

```text
[vtable+0x110]
```

que corresponde al slot 68 descrito arriba.

Si una condición previa se cumple, también existe una ruta que llama al mismo slot con 0.

Esto confirma que ThinkMode no es un valor leído únicamente por el input: forma parte del ciclo de inicialización/control del jugador.

---

## 8. Predicado nativo Self / Partner

En código debug del jugador se observa literalmente la selección de strings:

```text
"Self"
"Partner"
```

alrededor de `0x027B35A1`.

La decisión usa:

```text
thunk 0x01BB3192 -> 0x01CB7560
```

La función:

1. obtiene el SerialID del jugador local/self mediante una cadena de singletons;
2. obtiene el SerialID del `uPlayer` candidato mediante:
   `0x01BEDBB7 -> 0x01CB7610`
3. compara ambos;
4. además valida un estado/tipo mediante:
   `0x01C8D53B -> 0x01CA5F20`
5. devuelve true para Self y false para Partner.

El propio llamador usa:

```text
true  -> "Self"
false -> "Partner"
```

**CONFIRMADO:** ya existe un predicado de identidad local Self/Partner reutilizable.

---

## 9. Diseño de input preferido a partir de esta evidencia

El diseño anterior:

```text
modo 2/3 -> forzar ruta local
si modo 2/3 -> pad 1
```

queda reemplazado por:

```text
Partner/Sub0
   |
   +--> cambiar ThinkMode::Cpu (3)
   |    a ThinkMode::Pad (1)
   |    usando la ruta nativa
   |
   +--> selector de pad 0x027A27B0:
          Self    -> 0
          Partner -> 1
```

Ventajas:

- no secuestra `ThinkMode::Network`;
- deja intacta la ruta de red;
- usa el cleanup nativo al salir de CPU;
- usa la identidad Self/Partner que ya usa el propio juego;
- permite que ambos uPlayer entren en la misma lógica normal de gameplay, cada uno con PadData diferente.

---

## 10. Pista investigada y descartada: PcsSubNpcSetThinkType

Durante esta fase aparecieron strings y acciones FSM:

```text
PcsSubNpcSetThinkType
PcsMainNpcSetThinkType
cSubNpcSetThinkTypeParameter
cMainNpcSetThinkTypeParameter
mThinkMode
SetThinkMode
```

Inicialmente parecían una vía directa para cambiar el `ThinkMode::Cpu/Pad/Network` de `sCharaBase`.

Se siguieron:

```text
PcsMainNpcSetThinkType registration -> thunk 0x01BD9446 -> 0x02A6BFB0
PcsSubNpcSetThinkType  registration -> thunk 0x01C90083 -> 0x02A933C0
```

Ambas convergen en lógica FSM/NPC común alrededor de:

```text
0x02A18740
```

y el campo `mThinkMode` pertenece al bloque de parámetros/gestión de pensamiento NPC.

La ruta seguida acaba manipulando estructuras de NPC/red y no ha demostrado llamar al setter de control-source de `uPlayer` en `0x027F1290`.

**Estado actual:** **DESCARTADA como prueba directa del ThinkMode Pad/Network/Cpu de uPlayer**. Se conserva como pista separada de la IA/FSM NPC.

---

## 11. Próximo paso

1. localizar el momento más estable en que el Partner queda en `ThinkMode::Cpu`;
2. cambiar solo ese Partner a `ThinkMode::Pad` usando el setter nativo;
3. modificar el getter `0x027A27B0` para que devuelva:
   - Self = 0
   - Partner = 1
4. crear un nuevo experimento **nativo de input**, desde el EXE original de enero;
5. validar estáticamente que:
   - Network sigue siendo Network;
   - CPU de otros usos no se redirige globalmente;
   - P1 sigue en pad 0;
   - P2/Partner recibe pad 1;
6. solo después reincorporar cámara/split-screen.
