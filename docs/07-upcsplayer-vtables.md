# 07 — uPcsPlayerMain / Sub0 / Sub1: vtables e identidad nativa

Build: **BioRevHD 30-Enero-2013.exe**.

Este documento registra el análisis realizado en esta conversación después de localizar RTTI diferenciado para los tres roles de PCS.

## RTTI y vtables

Type descriptors encontrados:

```text
.?AVuPcsPlayerMain@pcs@game@app@@
.?AVuPcsPlayerSub0@pcs@game@app@@
.?AVuPcsPlayerSub1@pcs@game@app@@
```

Vtables:

```text
uPcsPlayerMain  0x04E1642C
uPcsPlayerSub0  0x04E1649C
uPcsPlayerSub1  0x04E1650C
```

Cada una tiene 21 entradas analizadas.

Resultado de la comparación:

- 18 de 21 slots son idénticos.
- solo cambian los slots virtuales **0, 4 y 5**.

Esto reduce la lógica específica Main/Sub a un conjunto muy pequeño de métodos.

## Slot 0

```text
Main thunk  0x01C2AF1C -> 0x02DF5900
Sub0 thunk  0x01B9F340 -> 0x02DF5F20
Sub1 thunk  0x01BEA340 -> 0x02DF6540
```

Por forma y comportamiento parecen destructores/deleting destructors específicos de clase.

**Estado:** no relacionado con selección de input.

## Slot 4

```text
Main  0x01B83591 -> 0x02DF58C0
Sub0  0x01B9D8CE -> 0x02DF5EE0
Sub1  0x01B9F6B5 -> 0x02DF6500
```

Son funciones mínimas que devuelven valores constantes distintos por clase.

Ejemplos observados:

```text
Main -> 0x0559A34C
Sub0 -> 0x0559A2CC
```

El patrón es consistente con devolver metadatos/DTI/type info propios de cada clase.

**Estado:** no parece selector de pad.

## Slot 5 — hallazgo importante

```text
Main  0x01BFFECF -> 0x02DF5D60
Sub0  0x01C5541F -> 0x02DF6380
Sub1  0x01C8B2C2 -> 0x02DF69A0
```

Los tres métodos son casi idénticos. La diferencia decisiva es una constante:

```text
Main -> push 0
Sub0 -> push 1
Sub1 -> push 2
```

Ese 0/1/2 coincide exactamente con el enum ya encontrado:

```text
0 = Main Player
1 = Sub Player 0
2 = Sub Player 1
```

### Flujo reconstruido

De forma simplificada:

```asm
call 0x02DF4D40        ; init común

push SLOT_INDEX        ; 0 / 1 / 2
mov  ecx,[this+0x30]
push ecx
push 0
call 0x01F014F0        ; obtiene singleton/sistema
add  esp,4

mov  ecx,eax
call 0x02DD0080        ; lookup sPcsManager(key, slot)

push eax
mov  ecx,this
call 0x02DF4E40        ; guarda resultado
```

Importante: el `push 0` pertenece a la obtención del singleton. Los dos argumentos que llegan a `0x02DD0080` son:

1. `[this+0x30]`
2. el índice Main/Sub `0/1/2`

### 0x02DD0080

La rutina recibe un índice de slot menor que 3.

Si `sPcsManager::mIsDebug` (`+0x1308`) está activo:

```text
return [sPcsManager + 0x130C + slot*4]
```

Es decir:

```text
slot 0 -> mDebugPlayerMainID
slot 1 -> mDebugPlayerSub0ID
slot 2 -> mDebugPlayerSub1ID
```

En modo normal recorre la tabla interna de PCS, busca el registro que corresponde al primer argumento y devuelve el campo asociado al slot Main/Sub solicitado.

### Resultado dentro del jugador

`0x02DF4E40` guarda el ID devuelto en:

```text
uPcsPlayer* + 0x40
```

Por tanto:

**CONFIRMADO:** cada una de las tres clases tiene una identidad nativa Main/Sub distinta, seleccionada por código real mediante los índices 0/1/2.

**NO CONFIRMADO todavía:** que ese índice se use directamente como índice de pad.

## Campo +0x40

El constructor común `0x02DF4D40` inicializa:

```text
+0x40 = -1
+0x44 = 0
+0x4B = 0
+0x4C = -1
```

Métodos compartidos posteriores leen `+0x40` para localizar/comparar el objeto/actor correspondiente.

Interpretación actual:

`+0x40` es un identificador de actor/PCS asociado al rol Main/Sub, no un número de mando.

## Consecuencia para el cooperativo local

Antes de este hallazgo el experimento de input trataba los modos secundarios de forma heurística.

Ahora existe una ancla mucho más sólida:

```text
uPcsPlayerMain -> slot 0
uPcsPlayerSub0 -> slot 1
uPcsPlayerSub1 -> slot 2
```

La siguiente tarea es seguir los métodos compartidos desde esa identidad hasta la capa de entrada y comprobar si existe una traducción nativa:

```text
slot PCS -> PadData index
```

Si existe, debe preferirse frente al parche heurístico anterior.
