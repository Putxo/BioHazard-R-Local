# 12 — cDebugPlayer: pruebas 1P/2P y equipamiento separado

Build: **BioRevHD 30-Enero-2013.exe**.

## Clase y campos 2P

`cDebugPlayer` está registrado con tamaño `0x34`, vtable `0x04CC620C` y constructor `0x01D7CAB0`.

El constructor inicializa, entre otros campos:

```text
+0x10 = 0
+0x11 = 1
+0x14 = 7
+0x18 = 10
+0x1C = -5
+0x20 = 5
+0x24 = 0x80011010
+0x28 = 0x80011020
+0x2C = 0
+0x30 = 0
```

La property registration asocia:

```text
+0x10  キャラ設定を使用する  = "usar configuración de personaje"
+0x11  ２Ｐ必要              = "2P necesario"
```

El valor `+0x11=1` por defecto es una pista fuerte del entorno de pruebas 2P, pero **todavía no se etiqueta como switch global de spawn** hasta encontrar su consumidor.

## Terrain action test

El bloque `地形アクションテスト` ("Terrain action test") contiene propiedades ejecutables separadas:

```text
Weapon0 / Weapon0(2P)
Weapon1 / Weapon1(2P)
Weapon2 / Weapon2(2P)
SubWeapon / SubWeapon(2P)
Genesis / Genesis(2P)
1P / 2P
character settings
```

No son strings huérfanos: cada propiedad de equipo registra callbacks reales.

## Slots de equipamiento confirmados

Los getters 1P y 2P usan la misma interfaz virtual y los mismos números de slot:

| Propiedad | Slot |
|---|---:|
| Weapon0 | 0 |
| Weapon1 | 1 |
| Weapon2 | 2 |
| SubWeapon | 4 |
| Genesis | 0xE |

Getters 1P:

```text
Weapon0  0x01D7E2E0
Weapon1  0x01D7E550
Weapon2  0x01D7E7C0
Sub      0x01D7EA30
Genesis  0x01D7ECA0
```

Setters 1P:

```text
0x01D7E400
0x01D7E670
0x01D7E8E0
0x01D7EB50
0x01D7EDC0
```

Getters 2P:

```text
Weapon0  0x01D7EF10
Weapon1  0x01D7F160
Weapon2  0x01D7F3B0
Sub      0x01D7F600
Genesis  0x01D7F850
```

Setters 2P:

```text
0x01D7F020
0x01D7F270
0x01D7F4C0
0x01D7F710
0x01D7F960
```

## Diferencia 1P / 2P

Las parejas son estructuralmente paralelas. La diferencia importante aparece al resolver qué actor/jugador se usa antes de acceder al inventario.

Las rutas diferenciadas terminan en:

```text
0x01C2C7F4 -> 0x01CB7400
0x01BCA0C2 -> 0x01D115B0
```

Los callbacks 1P fuerzan una rama y los 2P la otra; después ambos usan la misma interfaz de slots.

**CONFIRMADO:** la build conserva APIs de debug capaces de leer y escribir equipamiento de 1P y 2P por separado.

**Pendiente:** nombrar con certeza las dos rutas de resolución del actor y conectarlas formalmente con `uPcsPlayerMain/Sub0`.

## Pista descartada: for2P

`for2P @ 0x04DB5308` se siguió hasta una tabla/configuración cuyo puntero está en `0x054B1774`.

El parser alrededor de `0x028F5DB0` lo procesa junto a `SetFlag` y estructuras de configuración de objetos/ride.

No hay evidencia de que sea un flag global de HUD, input o cooperativo.

**Estado:** descartado como hook genérico. Puede seguir siendo una propiedad 2P específica de un ride/evento.


---

## Resolución formal del actor: Self vs no-Self

`cDebugPlayer` no diferencia 1P/2P cambiando los números de slot; diferencia **qué actor resuelve antes de entrar en la misma API de equipamiento**.

### Ruta 1P

`0x01C2C7F4 -> 0x01CB7400` itera candidatos y usa el predicado `0x01BB3192 -> 0x01CB7560`.

El predicado obtiene el ID Self mediante `0x01B9EB39 -> 0x01CB7650` y `0x01C473C4 -> 0x02D99DB0`, obtiene el ID del candidato mediante `0x01BEDBB7 -> 0x01CB7610` (`return [candidate+0xE3C]`) y exige:

```text
candidateID == SelfID
AND eligibility(candidate) == true
```

El filtro común de elegibilidad usa `0x01C8D53B -> 0x01CA5F20`.

### Ruta 2P

`0x01BCA0C2 -> 0x01D115B0` itera candidatos y usa `0x01C8731B -> 0x01D116C0`.

Este predicado obtiene los mismos IDs y exige:

```text
candidateID != SelfID
AND 0x01BE01D8 -> 0x01D11780(candidate) == true
AND eligibility(candidate) == true
```

`0x01D11780` consulta el subobjeto `candidate+0xF5C` con argumento `7` y devuelve la negación del resultado. Su nombre semántico exacto todavía no está recuperado.

**CONFIRMADO:** las rutas de equipamiento 1P/2P distinguen explícitamente **Self** frente a un **actor jugador elegible no-Self**. En una sesión de dos jugadores ese no-Self corresponde al compañero; la equivalencia formal con `uPcsPlayerSub0` se mantiene como fuerte y debe terminar de demostrarse desde el binder/colección cuando existan más de dos candidatos.

## Objeto contenedor

El constructor de `app::debug::sDebug` (`0x01D81120`) instala vtable `0x04CC65A0`. El RTTI asociado es `.?AVsDebug@debug@app@@`.

Durante ese constructor se crea `cDebugPlayer` en `0x01D812FE` y se asigna al miembro `sDebug+0x30`. La property registration de `sDebug` (`0x01D818D0`) registra ese miembro con el nombre literal `player @ 0x04CC63EC`.

Por tanto la cadena estructural queda confirmada:

```text
app::debug::sDebug
  +0x30  player -> cDebugPlayer
             +0x10 useCharacterSettings
             +0x11 2P necessary
```

El siguiente objetivo es localizar un consumidor ejecutable de `player->+0x11`; si no aparece, deberá tratarse como un dato consumido únicamente por el sistema genérico de reflexión/debug y no como un switch directo de spawn.
