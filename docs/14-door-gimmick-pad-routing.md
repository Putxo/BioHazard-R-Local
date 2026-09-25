# 14 — door_gimmick: MoveState y selector de pad del partner

Build principal: **BioRevHD 30-Enero-2013.exe**.

Base canónica a preservar: **v13 SYMMETRIC AMMO RELIEF**.

Este documento continúa inmediatamente después de cerrar la auditoría base de `uDoor2pBase` sin necesidad de v14. La siguiente interacción localizada pertenece a otra familia:

```text
door_gimmick@obj_model@chara@game@app
```

## 1. Clases nativas localizadas

RTTI/strings confirmados:

```text
MoveState
PlayerDoorGimmickStartState
PlayerDoorGimmickMoveState
PlayerDoorGimmickWaitState
PlayerDoorGimmickEndState
```

DTI conocidos:

```text
0x05581DF4 MoveState
0x05581DB4 PlayerDoorGimmickStartState
0x05581D74 PlayerDoorGimmickMoveState
0x05581E14 PlayerDoorGimmickWaitState
0x05581D94 PlayerDoorGimmickEndState
```

`MoveState` mide `0x24` bytes según su registro DTI.

## 2. MoveState real

Constructor:

```text
0x025907E0
```

Vtable:

```text
0x04D5D684
```

Slots relevantes:

```text
slot 5 -> 0x01C01AB8 -> 0x02590D00
slot 8 -> 0x01C4DF17 -> 0x02590F60
```

## 3. slot 5 guarda y resuelve el actor objetivo

`0x02590D00` recibe tres argumentos y guarda:

```asm
0x02590D23  mov eax,[this]
0x02590D26  mov ecx,[ebp+0x0C]
0x02590D29  mov [eax+0x10],ecx
```

Por tanto:

```text
MoveState+0x10 = identificador/selector del actor objetivo
```

Después:

- si `MoveState+0x10 >= 0`, resuelve el actor por ese valor mediante `sNetworkManage -> 0x01C07341`;
- si `MoveState+0x10 < 0`, llama explícitamente:

```text
0x01C4C9C3 -> resolver partner/non-Self (pPt)
```

y hace checked cast a `uPlayer`.

**CONFIRMADO:** `MoveState` está diseñado para operar también sobre el partner, no solo sobre Self/P1.

## 4. slot 8 vuelve a resolver ese mismo actor

`0x02590F60` lee:

```asm
mov ecx,[this+0x10]
push ecx
call get sNetworkManage
call actor lookup
```

y conserva el actor en:

```text
[ebp-0x20]
```

Después inspecciona si el actor está en alguno de:

```text
PlayerDoorGimmickStartState
PlayerDoorGimmickMoveState
PlayerDoorGimmickEndState
PlayerDoorGimmickWaitState
```

Por tanto el update del mismo `MoveState` sigue trabajando sobre el actor elegido por `+0x10`.

## 5. Lectura de input hardcodeada a Pad 0

Cuando el actor está en uno de los estados PlayerDoorGimmick esperados, el bloque:

```asm
0x02591118  push 0
0x0259111A  call 0x01C8B7F4   ; get sGamePad
0x02591121  call 0x01B94F53   ; -> 0x02DB0CF0
```

consulta explícitamente:

```text
selector = 0
```

`0x02DB0CF0` sí respeta su argumento en la línea v7/v13 porque v7 restauró:

```text
0x02DB0D25
0x02DB0D2F
```

para leer `[ebp+8]`.

El problema, por tanto, está en el **caller** de `MoveState`: fuerza 0 antes de llamar.

## 6. La lectura modifica una transición real del MoveState

Si el getter de pad retorna true:

```asm
0x0259112D  mov eax,[this]
0x02591130  mov byte ptr [eax+0x1D],1
```

Más adelante, durante el estado interno 2:

```asm
0x0259133C  test MoveState+0x1D
0x02591345  je ...
0x02591347  push 6
0x0259134C  call setState
```

El `switch` interno de `MoveState` maneja estados 1..7. La tabla verificada es:

```text
1 -> 0x02591211
2 -> 0x02591356
3 -> 0x02591481
4 -> 0x025914AD
5 -> epílogo
6 -> epílogo
7 -> 0x025911D2
```

El estado 6 es terminal/no-op dentro de este update.

Además, cuando el actor ya no está en ninguno de los estados `PlayerDoorGimmick*` esperados, la misma función puede fijar `+0x1D=1` y llevar el estado a 6.

Esto demuestra que la lectura del mando no es un mero efecto visual: participa en una transición interna real.

No se asigna todavía un nombre semántico más específico a estado 6 sin evidencia textual adicional.

## 7. Diagnóstico actual

La combinación de hechos es significativa:

1. `MoveState` puede seleccionar explícitamente al partner mediante `+0x10 < 0`;
2. su update vuelve a resolver ese actor;
3. el mismo update consulta una acción de `sGamePad`;
4. pero el caller fuerza selector 0;
5. la lectura puede cambiar el estado interno a 6.

**CANDIDATO DE BLOQUEO REAL DE PAD 2.**

Aún no se crea v14 hasta cerrar dos puntos:

- demostrar la ruta real que instancia/entra en `MoveState` con selector negativo/partner en gameplay;
- determinar si la transición accionada por el botón es necesaria/esperable para el partner y no una acción deliberadamente exclusiva de Self.

## 8. Próximo paso exacto

Seguir:

```text
0x01BDF9F4 -> 0x02590A70
```

que registra/crea `MoveState` mediante DTI `0x05581DF4`, y localizar quién le pasa los argumentos que terminan en `MoveState::slot5`.

También reconstruir el contexto de la transición a estado 6 sin inventar un nombre de enum.
