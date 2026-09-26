# 45 — Pausa global, sIDCockpit y ownership del submenu

27 de septiembre de 2026. Continúa desde el estado consolidado `4afe150c82c669960539f98c7efe7c2efb813d87`. Solo fuentes/evidencia; no se genera ni modifica un EXE.

## sIDCockpit es el árbitro global

El constructor de `sIDCockpit` crea y conserva seis hijos distintos:

- `uCockpitManagerMain` en `this+0x28`;
- `uMiniMapManager` en `this+0x2C`;
- `uGUI_GameOverHD` en `this+0x30`;
- `uGUI_PauseHD` en `this+0x34`;
- `uGUI_PauseBlurHD` en `this+0x38`;
- `uSubMenuManager` en `this+0x3C`.

Por tanto PauseHD/PauseBlur/SubMenu no son instancias por jugador: pertenecen a una única state machine global de cockpit.

## Reservas de pausa global

`sIDCockpit::move` mantiene dos flags independientes en `+0x40/+0x41`, con assert literal:

```
[sIDCockpit::move] pause flag error
```

El código impide que ambas estén activas simultáneamente.

La rutina alrededor de `0x02CDE840` reserva inicio/final de pausa. Con el modo forzado puede escribir directamente START o END; con el modo normal valida estados antes de reservar.

Conclusión: la pausa del motor debe seguir siendo global y única. No se duplicará `uGUI_PauseHD` para P2 ni se crearán dos estados de pausa global.

## setState de sIDCockpit

La state machine de `sIDCockpit` está centralizada en `0x02CDD180`.

El switch despacha estados sobre hijos globales. Queda demostrado:

- estado 2 reactiva Cockpit/MiniMap;
- estado 3 entra en GameOver;
- estado 5 activa `uGUI_PauseHD`;
- estado 8 activa `uSubMenuManager`.

Los estados 6/7 no deben etiquetarse como Pause/SubMenu únicamente por número.

### Rectificación de uGUI_InGameFile

`uGUI_InGameFile` (vtable `0x04DE6254`) guarda en `+0x2B8` el estado previo de `sIDCockpit`, fuerza temporalmente otro estado y luego lo restaura.

Por tanto `uGUI_InGameFile+0x2B8 == 8` significa “estado anterior = SubMenu”; **no es el setter/opener real del SubMenu**.

El helper `0x02B519A0` restaura ese estado anterior y no debe usarse para registrar el owner del inventario P2.

## PauseHD no lee el pad físico en el update auditado

`uGUI_PauseHD::update 0x02C309C0`:

1. ejecuta la actualización base;
2. resuelve explícitamente Self;
3. consulta estado del actor;
4. puede poner `PauseHD+0x2AC = 1`.

No contiene la decisión principal de qué mando solicita pausa.

El virtual 8 de PauseHD consulta wrappers que terminan en `sGameFlags`; los índices observados 0/1/2 son consultas de flags globales, **no selectores de PadData**.

Esto descarta parchear PauseHD para “Pad 2” directamente.

## uSubMenuManager y uGUI_SubMenu son globales

`uSubMenuManager` posee:

- `uGUI_SubMenuBase` en `+0x70`;
- `uGUI_SubMenu` en `+0x74`;
- `uGUI_WorldMapIcon` en `+0x78`;
- `uWorldMapModel` en `+0x7C`.

No son cuatro inventarios ni dos pares P1/P2.

La metadata de `uGUI_SubMenu` expone campos de navegación/lista como `mItemId`, `mTab`, `mTabMax`, `mListIndex` y `mSelect`, pero no un owner/player/member.

## El submenu stock está hardcodeado a Self para datos de inventario

Se localizaron dos rutas independientes dentro de `uGUI_SubMenu`:

```
0x02B6DE91 -> 0x01C2C7F4 -> Self
0x02B6DEA7 -> 0x01BCC327 -> cBioItemPack
```

y:

```
0x02B6EFDE -> 0x01C2C7F4 -> Self
0x02B6EFF4 -> 0x01BCC327 -> cBioItemPack
```

Ambas reconstruyen contenido del submenu a partir del pack de Self. Una segunda instancia stock sin ownership nuevo volvería a mostrar P1.

Una de estas rutas construye listas en el propio `uGUI_SubMenu` según un modo/tab; la otra repite la selección Self->pack para una ruta relacionada. No se debe redirigir el finder Self global.

## Input del submenu

Ni `uGUI_SubMenu::slot8` ni `uSubMenuManager::slot8` llaman directamente a las APIs físicas de `sGamePad` de la familia ya corregida por v7. Sus consultas globales identificadas pasan por `sGameFlags`.

Por tanto el input de navegación llega desde una capa superior de infraestructura GUI/eventos. Todavía no está demostrado un selector de miembro dentro de uGUI_SubMenu.

## Consecuencia de diseño

La dirección segura queda separada en dos piezas:

1. **pausa global única**: sIDCockpit conserva ownership del estado de pausa;
2. **owner de inventario/submenu**: cuando un participante abre el submenu, las lecturas de datos deben resolverse contra P1 o Sub0 sin modificar globalmente Self.

Falta localizar el opener real del estado 8 a través del wrapper público de sIDCockpit y la infraestructura de input que origina esa transición. Ese es el punto correcto para registrar el owner P1/P2 del submenu.

## Siguiente trabajo exacto

- seguir todos los callers del wrapper público de `sIDCockpit::setState` que puedan solicitar estado 8;
- identificar la fuente del evento/input que dispara ese caller;
- comprobar si esa fuente transporta selector de miembro/pad o si necesita una asociación local explícita;
- después implementar un owner externo de uGUI_SubMenu y sustituir únicamente sus dos resoluciones Self->pack por “owner seleccionado”, sin alterar Self global ni duplicar PauseHD;
- mantener la navegación y la pausa global separadas hasta demostrar su dispatcher.

No se ha ejecutado gameplay y no se han instalado hooks.
