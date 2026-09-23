# 07 — RTTI/vtables Main/Sub y matriz de roles de sPcsManager

Build: **BioRevHD 30-Enero-2013.exe**.

Este documento continúa únicamente la investigación de esta conversación.

## 1. uPcsPlayerMain / Sub0 / Sub1: vtables casi idénticas

RTTI / vtables identificadas:

```text
uPcsPlayerMain  vtable 0x04E1642C
uPcsPlayerSub0  vtable 0x04E1649C
uPcsPlayerSub1  vtable 0x04E1650C
```

Cada tabla tiene 21 entradas examinadas. **18 de 21 son iguales**. Solo difieren los slots virtuales 0, 4 y 5.

### Slot 0

Son destructores escalares específicos de cada clase.

```text
Main  thunk 0x01C2AF1C -> 0x02DF5900
Sub0  thunk 0x01B9F340 -> 0x02DF5F20
Sub1  thunk 0x01BEA340 -> 0x02DF6540
```

### Slot 4

Devuelve metadato/puntero específico de clase. No se considera actualmente el hook de control.

### Slot 5 — diferencia funcional importante

Implementaciones:

```text
Main  thunk 0x01BFFECF -> 0x02DF5D60
Sub0  thunk 0x01C5541F -> 0x02DF6380
Sub1  thunk 0x01C8B2C2 -> 0x02DF69A0
```

Las tres ejecutan el mismo algoritmo y cambian una sola constante de rol:

```asm
Main: push 0
Sub0: push 1
Sub1: push 2

mov  ecx,[this+30h]
push ecx
push 0
call 0x01B99472
add  esp,4
mov  ecx,eax
call 0x01C5B775
push eax
mov  ecx,this
call 0x01C51B4E
```

El thunk `0x01C5B775` llega a `0x02DD0080`.

El resultado se guarda finalmente en `uPcsChara+0x40`.

**CONFIRMADO:** la diferencia Main/Sub0/Sub1 forma parte del comportamiento ejecutable y usa el mismo enum 0/1/2 observado en `sPcsManager`.

---

## 2. Base uPcsChara

La rutina base usada por el slot 5 llega aproximadamente a:

`0x02DF4D40`

Inicializa, entre otros:

```text
[this+0x40] = -1
[this+0x44] = 0
[this+0x4B] = 0
[this+0x4C] = -1
```

Después las clases derivadas resuelven su asociación de rol y sustituyen `+0x40`.

Setter observado:

```text
thunk 0x01C51B4E -> ~0x02DF4E40
[this+0x40] = argumento
```

La semántica final de `+0x30` y `+0x40` todavía no se etiqueta con un nombre inventado. Lo confirmado es el flujo.

---

## 3. sPcsManager::resolver(role, key) en 0x02DD0080

La función `0x02DD0080` recibe:

```text
this      = sPcsManager
arg1      = valor procedente de uPcsChara+0x30
arg2      = role (0 Main, 1 Sub0, 2 Sub1)
return    = asociación/ID resuelto, o -1
```

### Caso debug

Si `role >= 3`, devuelve -1.

Si:

`sPcsManager+0x1308 != 0`  (`mIsDebug`)

devuelve directamente:

```text
[this + 0x130C + role*4]

role 0 -> mDebugPlayerMainID
role 1 -> mDebugPlayerSub0ID
role 2 -> mDebugPlayerSub1ID
```

### Caso normal

Si no está en debug:

1. recorre 20 entradas;
2. compara `arg1` con:
   `[this + 0x2C + i*4]`
3. cuando encuentra una coincidencia calcula:
   `row = this + 0x7C + i*0x0C`
4. devuelve:
   `row[role]`

Pseudocódigo:

```cpp
int resolveRole(int key, int role) {
    int result = -1;

    if (role >= 3)
        return -1;

    if (mIsDebug)
        return debugPlayerId[role];

    for (unsigned i = 0; i < 20; ++i) {
        if (key == keys[i])
            return roleMatrix[i][role]; // 3 dwords por fila
    }

    return -1;
}
```

**CONFIRMADO:** existe una matriz normal de **20 × 3** asociaciones de rol:

```text
fila i:
  +0x00 Main
  +0x04 Sub0
  +0x08 Sub1

stride por fila = 0x0C
base = sPcsManager + 0x7C
```

Esto es más fuerte que la hipótesis anterior basada únicamente en nombres debug.

---

## 4. uPcsActor<MainPlayer/SubPlayer0/SubPlayer1>

RTTI diferenciado encontrado también para actores template:

```text
uPcsActor<MainPlayer>
uPcsActor<SubPlayer0>
uPcsActor<SubPlayer1>
```

Vtables:

```text
Actor Main  0x04E1657C
Actor Sub0  0x04E165EC
Actor Sub1  0x04E1665C
```

De nuevo, 18/21 entradas examinadas coinciden; slots 0/4/5 son específicos.

Slot 5 real:

```text
Main  0x02DFCA50 -> role 0
Sub0  0x02DFCB30 -> role 1
Sub1  0x02DFCC10 -> role 2
```

Los tres llaman al mismo resolver de `sPcsManager` y guardan el resultado mediante el mismo setter de `uPcsChara+0x40`.

**CONFIRMADO:** Main/Sub no son meras etiquetas de herramientas; la selección 0/1/2 está codificada también en los actores de gameplay.

---

## 5. Player0 / Player1 / Player2 son otra familia de roles

También se identificaron:

```text
uPcsActor<Player0>
uPcsActor<Player1>
uPcsActor<Player2>
```

con vtables diferentes:

```text
Player0 0x04E166CC
Player1 0x04E1673C
Player2 0x04E167AC
```

Sus slots específicos vuelven a usar constantes 0/1/2, pero al menos parte de la ruta llama a otro método de `sPcsManager`.

**Interpretación actual:** probablemente representan otra indexación de jugador (por ejemplo infraestructura multiplayer/Raid) distinta de la familia de campaña Main/Sub. No se mezclan todavía ambos conceptos.

---

## 6. Corrección de mMovePcs / mMoveSubPcs

Se volvió a seguir esta pista porque sus nombres parecían capaces de separar el movimiento de Main y Sub.

Xrefs alrededor de:

```text
mMovePcs     ~0x02DCB123
mMoveSubPcs  ~0x02DCB185
```

El código compara nombres de comando y construye/registrar objetos de estado/acción.

**CORRECCIÓN:** actualmente parecen **comandos de parser/FSM/script**, no campos que seleccionen directamente el mando físico.

No se eliminan de la investigación, pero dejan de ser el hook prioritario de input.

---

## 7. PadData: offsets confirmados por registro de propiedades

La rutina de registro alrededor de `0x02DB61D0` permite asignar nombres a campos del bloque `sGamePad::PadData`/estructura relacionada:

```text
+0x14 mControlTypeBak
+0x18 mAimSpeedType
+0x1C mAimStickType
+0x20 mSwimStickType

+0x28 mMoveAnalog
+0x30 mRotateAnalog
+0x38 mAimAnalog
+0x40 mSurveyAnalog
+0x48 mZoomAnalog
+0x4C mAccelTime
+0x50 mZoomRate
+0x54 mHandJiggleLevel
+0x60 mIsUseAimInertia

+0x78 mUseMoveAnalog
+0x80 mUseMoveLength
+0x88 mUseRotateAnalog
+0x90 mUseAimAnalog
+0x98 mUseSurveyAnalog

+0xB8 mSwimRapidCount
```

Esto confirma que los bloques candidatos de entrada contienen directamente los analógicos que necesitamos separar entre P1/P2.

---

## 8. Consecuencia para el plan

La ruta prioritaria cambia de:

```text
forzar modo secundario -> pad 1
```

a:

```text
uPcsPlayerSub0
  -> role 1
  -> sPcsManager role matrix
  -> asociación guardada en uPcsChara+0x40
  -> seguir consumidores de +0x40
  -> localizar selección nativa de PadData
```

El experimento anterior de pad 1 se conserva como prueba estática, pero ya no es el diseño preferido mientras exista una vía nativa por roles.

## 9. Próximo punto a resolver

Determinar exactamente:

1. qué representa `uPcsChara+0x30`;
2. qué representa el resultado guardado en `+0x40`;
3. qué funciones consumen `+0x40`;
4. si una de ellas selecciona un `PadData`/índice de pad.

No se asignará un nombre semántico a esos campos hasta encontrar evidencia ejecutable adicional.
