# 12 — v11/v12: pickup local de Sub0 y cActionCommand en Pad 2

Build base de la línea actual: **30-Enero-2013 FullDebug**.

## v11 — permitir que Sub0 sea actor de pickup

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP v11 SUB0 PICKUP.exe`

SHA-256:

`2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69`

v11 parte de v10 y modifica únicamente la ruta de `uItem` necesaria para que el actor local seleccionado pueda ser el exacto `uPcsPlayerSub0/uNpc`.

Diseño:

- conserva la búsqueda stock de P1;
- cuando el cooperativo local está activo, compara también el Sub0 exacto contra el mismo transform del `uItem`;
- guarda en el slot stock `uItem+0xF48` al actor más cercano;
- los dos gates stock que exigían categoría `pl` admiten como excepción **solo** el puntero exacto `gSub0Npc`;
- no habilita NPC genéricos;
- no modifica la ruta global/network.

Helper:

```text
0x01C95120 choose
0x01C951D0 gate1
0x01C951F7 gate2
```

Diff frente a v10:

```text
262 bytes efectivos
5 rangos
mismo tamaño PE
```

## Confirmación RTTI de uItem

El punto donde quedó interrumpido un turno anterior era la inicialización global alrededor de `0x049A8023`.

Se terminó de seguir:

- `0x049A8023` registra el DTI de `uItem`;
- string `"uItem"`;
- global DTI `0x05579584`;
- constructor real instala vtable `0x04D30844`;
- la inicialización siguiente registra `uItem::cNetSyncData::cGetItemSyncData`.

Esto confirma que los hooks de v11 están dentro de la clase correcta.

## Problema descubierto después de v11: cActionCommand seguía anclado a miembro 0

`uItem` crea un `cActionCommand` de tamaño `0x110` y lo guarda alrededor de:

```text
uItem + 0xF5C
```

RTTI:

```text
app::game::action_command::cActionCommand
vtable 0x04CDA850
```

El metadata de `cActionCommand` registra `mpOwner` en `+0x30`.

### Delegate selector del uItem

Durante setup, `uItem` construye un delegate y lo instala en el subobjeto `cActionCommand+0x34`.

El método que resuelve el miembro/pad es:

```text
0x01BA99B7 -> 0x026D7AD0
```

El puntero crudo `0x01BA99B7` aparece **una sola vez en todo el EXE**, en esa configuración de `uItem`.

Implementación stock:

```asm
xor eax,eax
ret 4
```

Por tanto los ActionCommands de `uItem` resuelven siempre:

```text
member index = 0
```

aunque v11 haya seleccionado a Sub0 como actor de pickup.

## cActionCommand pasa ese índice realmente a sGamePad

La rutina principal:

```text
0x01E8CB20
```

llama al selector de member y pasa el valor a varias APIs de `sGamePad`.

Entre ellas:

```text
0x01C309FD -> 0x02DB2B50
0x01B7ACA2 -> 0x02DB0080
0x01BCC3A9 -> 0x02DAEB40
```

v7 ya había restaurado el selector en:

- `0x02DB0080`;
- `0x02DAEB40`.

Pero no en `0x02DB2B50`.

### 0x02DB2B50

La función termina con `ret 4`, por lo que recibe un argumento selector, pero stock PC ejecuta:

```asm
mov ecx,[sGamePad+0x970] ; mStartPadNo
```

en `0x02DB2B7D`.

Los presets que `uItem` instala usan códigos internos:

```text
0x80008200
0x90008200
0xA000A000
0xB000A000
```

y las ramas correspondientes de `0x01E8CB20` pasan por `0x02DB2B50`.

## v12 — pickup confirmado por Pad 2

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP v12 SUB0 PICKUP PAD2.exe`

SHA-256:

`5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8`

Cambios adicionales sobre v11:

1. `0x026D7AD0`:
   - devuelve 1 solo si `gLocalCoopActive != 0`;
   - y `uItem+0xF48 == gSub0Npc`;
   - en cualquier otro caso devuelve 0.

2. `0x02DB2B7D`:
   - sustituye la carga de `mStartPadNo`;
   - usa el selector recibido en `[ebp+8]`.

Verificación estática:

```text
same PE size: yes
different bytes vs v11: 50
ranges: 3
PE32 válido
```

### Flujo resultante

```text
uItem selecciona target:
  P1 -> member 0 -> PadData[0]
  Sub0 local -> member 1 -> PadData[1]

cActionCommand
  -> sGamePad(selector)
  -> botón/interacción del mando correspondiente
```

Fuera de local co-op, el callback sigue devolviendo 0, por lo que el comportamiento permanece equivalente al stock.

## Pendiente inmediato

La arquitectura stock conserva un solo target de pickup por `uItem` (`+0xF48`). v11/v12 eligen P1 o Sub0 por distancia.

Falta comprobar el caso:

- ambos están dentro del rango;
- el más cercano no es elegible para ese objeto;
- el otro sí es elegible.

La siguiente investigación debe determinar si el selector debe incorporar elegibilidad antes de elegir actor o si las validaciones posteriores ya resuelven correctamente ese caso.
