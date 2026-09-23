# 11 — Persistent Partner camera / v8

Build principal: **BioRevHD 30-Enero-2013.exe**.

Esta iteración parte de **v7 FULLPAD + Native Split** y corrige el principal riesgo pendiente de cámara: el juego vuelve a ejecutar lógica stock que puede desactivar Partner o volver a usar una única vista después de la inicialización.

## Hallazgo previo

Los callbacks internos `Self View` y `Partner View` convergen en `0x0203E8A0`.

Stock:

- Self activa Self, desactiva Partner y enlaza Self a VIEW_0.
- Partner desactiva Self, activa Partner y enlaza Partner a VIEW_0.

Además, durante el setup posterior de actores/cámaras, la rama Partner asigna su target y después llama a dos setters con cero, apagando dos bits de estado.

El juego también contiene `0x027A2FA0`, que devuelve Self CameraManage cuando el uNpc es el jugador local y Partner CameraManage cuando es el otro actor. Esto confirma que la asociación actor→manager está prevista nativamente.

## Diseño v8

Se añade un DWORD BSS:

```text
0x057D9188 = gLocalCoopActive
```

Solo se pone a 1 después de que el **Sub0 exacto** haga la transición canónica:

```text
ThinkMode::Cpu(2) -> ThinkMode::Pad(1)
```

`ThinkMode::Network(3)` no activa el flag, de modo que el comportamiento online original permanece stock.

Cuando el flag está activo, `ensure_split`:

1. recupera el uNpc exacto de Sub0 desde `0x057D9184`;
2. obtiene sGameCamera;
3. obtiene Self `+0xCE0` y Partner `+0xCE4`;
4. asigna el Sub0 exacto como target de Partner mediante `0x01C275EC -> 0x02066AB0`;
5. inicializa/activa ambos managers con las mismas rutas nativas del juego;
6. enlaza Self→VIEW_0 y Partner→VIEW_1 mediante `0x01C34D5A`;
7. configura VIEW_0 TOP(2) y VIEW_1 BOTTOM(3), visibles en display 0;
8. refresca el sistema de cámara.

## Persistencia

También se interceptan condicionalmente:

```text
0x0203E92D  Self View: intento de desactivar Partner
0x0203E98B  Partner View: intento de desactivar Self
0x0203E9AD  Partner View: índice de viewport stock
0x01F38054  Partner camera state bit 0
0x01F38061  Partner camera state bit 1
```

Con `gLocalCoopActive=0` todos conservan el comportamiento stock.

Con `gLocalCoopActive=1` ambos managers permanecen activos, Partner usa VIEW_1 y los dos bits de Partner se mantienen a 1.

## Code cave correcto y descarte documentado

El primer intento de esta iteración propuso `0x027A1500` como code cave. La validación de bytes lo rechazó porque allí empieza **código real**, no padding.

No se generó ningún EXE con ese solapamiento.

Se buscó después un run real de `0xCC` y se eligió:

```text
0x01C94FC0...
```

que pertenece a un bloque continuo de padding de 64 KiB. El helper ocupa solo 314 bytes.

El builder exige que cada byte de la zona destino siga siendo `0xCC`; si no, aborta.

## Resultado

```text
BioRevHD 30-Enero-2013 LOCAL COOP v8 FULLPAD PERSISTENT SPLIT.exe

SHA-256:
c9a4c1e9887eb3504905734cb0279b2c8fe61ec55bb69768c55724b3b7ec8356

mismo tamaño que el original: sí
PE32 válido: sí
diferencias vs original: 1026 bytes / 90 rangos
diferencias añadidas vs v7: 351 bytes / 11 rangos
```

## Estado

**STATICALLY VERIFIED ONLY.**

v8 supersede a v7 como candidato estático principal para la combinación input+cámara.

Todavía deben probarse dentro del juego: asignación física del segundo mando, render simultáneo real, culling, cinemáticas, HUD, inventario/pausa, interacciones y QTE.
