# 09 — SubPlayer0: identificación exacta del compañero y filtro local

Build de referencia: **BioRevHD 30-Enero-2013.exe**.

Este paso resuelve el principal problema de seguridad del experimento v3: no se debe convertir globalmente `ThinkMode::Cpu` en entrada local porque la misma ruta pertenece a muchos NPC humanos.

## 1. uPcsPlayer* enlaza un ID de PCS con un uNpc real

La rutina común:

```text
0x02DF4F30
```

parte de:

```text
uPcsPlayer* + 0x40 = ID PCS esperado
uPcsPlayer* + 0x44 = puntero de actor actual
```

Durante el scan:

```text
0x01C0355C -> 0x01D298E0
0x01BEE332 -> 0x01D296F0
0x01BEDBB7 -> 0x01CB7610
```

La última función devuelve:

```asm
mov eax,[this+0xE3C]
ret
```

Por tanto el scan compara:

```text
[uNpc + 0xE3C] == uPcsPlayer* + 0x40
```

Cuando encuentra coincidencia, vuelve a obtener el objeto y ejecuta:

```asm
mov [uPcsPlayer* + 0x44], eax
```

El valor almacenado es el objeto que inmediatamente se usa como actor/chara en múltiples métodos comunes.

**CONFIRMADO:** `uPcsPlayer*+0x44` es el puntero vivo al actor asociado al ID PCS; en este flujo el objeto es el `uNpc` cuyo ID está en `+0xE3C`.

## 2. SubPlayer0 se identifica por clase, no por personaje

Vtable:

```text
uPcsPlayerSub0 = 0x04E1649C
```

Por ello el binder común permite distinguir de forma exacta:

```text
uPcsPlayerMain
uPcsPlayerSub0
uPcsPlayerSub1
```

sin depender del personaje concreto del episodio.

Esto evita filtros frágiles por Jill/Parker/Chris/etc.

## 3. ThinkMode está justo al lado del ID

En `uNpc`:

```text
+0xE3C = ID usado por PCS
+0xE40 = ThinkMode
```

Constructor de `uNpc` alrededor de `0x027E9A00`:

```asm
mov [uNpc+0xE3C], -1
mov [uNpc+0xE40], 2
```

Por tanto el modo por defecto es:

```text
ThinkMode::Cpu = 2
```

Esto explica por qué el v3, que solo redirigía `Network=3`, no bastaba para un compañero normal de campaña.

## 4. Diseño v5: no modificar ThinkMode

Se descartó forzar el compañero a `ThinkMode::Network` porque el setter oficial también actualiza subsistemas asociados y podría interferir con red/sincronización.

v5 conserva los modos nativos.

Regla:

```text
ThinkMode::Pad (1)
    -> ruta local original

ThinkMode::Cpu (2)
    -> SI this == uNpc vivo de SubPlayer0:
           ruta local con PadData[1]
       SI NO:
           ruta CPU original

ThinkMode::Network (3)
    -> ruta Network original
```

Esto preserva:

- IA de todos los demás NPC;
- cooperativo online/network original;
- ruta local P1 original.

## 5. Seguimiento del puntero Sub0

El binder común de `uPcsPlayer` se instrumenta de forma mínima.

Cuando limpia `+0x44`:

- si la vtable es `uPcsPlayerSub0`, limpia también `gLocalCoopSub0Npc`.

Cuando encuentra actor:

- reproduce la asignación original `[this+0x44]=uNpc*`;
- si la vtable es `uPcsPlayerSub0`, copia ese mismo `uNpc*` a `gLocalCoopSub0Npc`.

## 6. Almacenamiento writable sin reutilizar un global del juego

No se usa padding de `.text` como variable.

La build original tiene:

```text
.data VirtualSize = 0x003B4184
.data final VA    = 0x057D9184
.idata comienza   = 0x057DA000
```

v5 incrementa únicamente:

```text
.data VirtualSize 0x003B4184 -> 0x003B4188
```

y usa el nuevo DWORD zero-fill:

```text
gLocalCoopSub0Npc = 0x057D9184
```

El tamaño de archivo no cambia.

## 7. Selector PadData

La función selector original `0x027A27B0` devolvía siempre 0.

v5 hace:

```text
this == gLocalCoopSub0Npc -> 1
otro actor                -> 0
```

Como ya se demostró que `sGamePad` tiene dos entradas reales y el helper valida `index < 2`, esto selecciona el segundo slot interno solo para SubPlayer0.

## 8. Outputs v5

### Input-only

```text
BioRevHD 30-Enero-2013 LOCAL COOP P2 SUB0 INPUT v5.exe
SHA-256:
d294508a5fe4919eb9cb46c446f23ba7a79eb1da8664dac3d7cf9ed3437068b7
```

Verificación vs original:

```text
same size: yes
different bytes: 210
contiguous ranges: 21
```

### Input + native split v4

```text
BioRevHD 30-Enero-2013 LOCAL COOP v5 SUB0 NATIVE SPLIT.exe
SHA-256:
e83207b2c64172468930eacbe0460fb7847061b56ac3d4e3a3b7f3ee009bfe9c
```

Verificación vs original:

```text
same size: yes
different bytes: 382
contiguous ranges: 23
```

## 9. Estado

**STATICALLY VERIFIED ONLY.**

v5 corrige dos problemas conceptuales de versiones anteriores:

1. no secuestra `Network=3`;
2. no transforma globalmente todos los NPC `Cpu=2`.

La validación siguiente debe ser runtime:

- confirmar que el compañero de una escena se enlaza como SubPlayer0;
- confirmar que el segundo mando físico alimenta PadData[1];
- comprobar movimiento, aim y botones simultáneos;
- validar cámara Partner y split-screen;
- después abordar HUD, inventario, QTE y scripts.
