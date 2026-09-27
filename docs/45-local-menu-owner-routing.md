# 45 — Owner local de pausa/submenu e inventario de Sub0

27 de septiembre de 2026. Continúa el estado consolidado `4afe150c82c669960539f98c7efe7c2efb813d87`. Solo fuentes, tests y evidencia; no se genera/modifica un EXE ni se instalan hooks.

## Dos bloqueos stock separados

### Apertura física

En `cEpisodeMainState`, cuando `sIDCockpit` está en estado 2:

- `0x01F3EF43` consulta el bit `0x8` del estado de `sPad[mStartPadNo]` y entra en estado 5 (PauseHD);
- `0x01F3EF92` consulta el bit `0x1` del mismo pad y entra en estado 8 (uSubMenuManager).

El wrapper stock `0x01CB30E0` carga `sPad+0x970 = mStartPadNo`; el getter directo `0x01CB3140` lee `sPad + member*0x2F8 + 0x1A0`.

Por tanto J2 no puede originar ninguno de esos dos estados por la ruta stock.

### Datos del submenu

`uGUI_SubMenu` reconstruye inventario en dos funciones distintas. Ambas hacen:

```
Self finder 0x01C2C7F4
 -> actor
 -> 0x01BCC327
 -> actor+0x1524
 -> cBioItemPack propio
```

Callsites Self exactos: `0x02B6DE91` y `0x02B6EFDE`.

Una segunda instancia stock sin ownership seguiría mostrando los datos de J1.

## Input dentro de uGUI_SubMenu

El virtual 8 real obtiene `sPad` en `0x02B6A484`.

Dos valores sí dependen del miembro:

- `0x02B6A491` -> wrapper por `mStartPadNo` -> `Pad[index]+0x198`;
- `0x02B6A49E` -> wrapper por `mStartPadNo` -> `Pad[index]+0x1A0`.

Cuatro helpers cercanos también cargan `mStartPadNo`, pero sus implementaciones directas ignoran el argumento y devuelven campos globales `sPad+0x650/+0x654/+0x658/+0x65C`. No se parchean solo por contener una carga redundante del índice.

## MenuOwnerRouter

El componente guarda exclusivamente:

```
owner: 0 Self / 1 Sub0
surface: None / Pause(5) / SubMenu(8)
```

Reglas:

- local coop apagado o Sub0 inválido: comportamiento stock;
- Sub0 debe seguir existiendo, tener serial 0..127 y ThinkMode::Pad;
- P1 usa Pad 0 y P2 Pad 1, igual que el routing local ya integrado;
- si ambos solicitan la misma apertura en el mismo tick, P1 tiene prioridad;
- un opener P2 registra owner=1;
- los dos bitfields del submenu se leen de Pad 1 solo mientras owner=1 y la superficie es SubMenu;
- las dos lecturas Self del submenu conservan primero el finder stock y solo reemplazan su resultado por el Sub0 exacto cuando ese mismo owner sigue siendo válido;
- no se modifica globalmente Self;
- no se escribe persistentemente mStartPadNo;
- Pause owner se registra para el bloque posterior de navegación/opciones, pero este cambio no redirige todavía datos de PauseHD.

Los estados 5/8 siguen siendo globales de sIDCockpit. No se crean dos PauseHD ni dos uSubMenuManager simultáneos.

## Callsite set

`patches/menu_routing/menu_sites.json` fija seis sitios: dos openers, dos bitfields de navegación y dos Self finders de datos. Los gateways son source-only y preservan el contrato de limpieza `ret 4` de los wrappers de sPad.

## Continuación

Con este bloque, J2 puede ser identificado como solicitante del submenu y el componente sabe qué actor/pad debe usar el inventario. Sigue pendiente:

1. conectar este owner al flujo real de cierre/restauración del submenu;
2. resolver la navegación completa de PauseHD/opciones cuando owner=1;
3. comprobar cualquier submenu secundario/nested state 6/7;
4. instalar los sitios junto con el resto del runtime y validar gameplay.

Después: scheduler/QTE restantes, muerte/reanimación/checkpoints, cutscenes/cámaras forzadas y escenas sin partner.
