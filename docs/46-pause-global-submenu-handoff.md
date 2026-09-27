# 46 — Pausa global y handoff del owner J1/J2

27 de septiembre de 2026. Consolida el análisis posterior al PR #19.

## sIDCockpit es global

El owner global contiene:
```text
+0x28 uCockpitManagerMain
+0x2C uMiniMapManager
+0x30 uGUI_GameOverHD
+0x34 uGUI_PauseHD
+0x38 uGUI_PauseBlurHD
+0x3C uSubMenuManager
```

La state machine interna es `0x02CDD180`; se confirmó:
- 2 -> Cockpit/MiniMap
- 3 -> GameOver
- 5 -> PauseHD
- 8 -> uSubMenuManager

Las reservas de inicio/final de pausa son globales (`+0x40/+0x41`). No duplicar PauseHD para P2.

## PauseHD

`uGUI_PauseHD::update 0x02C309C0` resuelve Self y refleja estado del actor. Su virtual 8 auditado consulta wrappers de `sGameFlags`; los índices 0/1/2 observados allí no son PadData.

Por tanto el siguiente parche no debe sustituir esos índices por 1. Hay que seguir la infraestructura real que alimenta navegación/opciones y usar el owner registrado por MenuOwnerRouter.

## SubMenu

`uSubMenuManager` posee:
```text
+0x70 uGUI_SubMenuBase
+0x74 uGUI_SubMenu
+0x78 uGUI_WorldMapIcon
+0x7C uWorldMapModel
```

uGUI_SubMenu stock no tiene owner/member reflejado y reconstruye inventario desde Self en:
```text
0x02B6DE91 -> Self
0x02B6DEA7 -> cBioItemPack

0x02B6EFDE -> Self
0x02B6EFF4 -> cBioItemPack
```

PR #19 ya sustituye solo esas dos resoluciones para owner=J2 y enruta los bitfields +0x198/+0x1A0 a Pad1.

## PR #19 sitios

```text
0x01F3EF43 Pause open
0x01F3EF92 SubMenu open
0x02B6A491 SubMenu +0x198
0x02B6A49E SubMenu +0x1A0
0x02B6DE91 actor #1
0x02B6EFDE actor #2
```

## Rectificaciones

- `uGUI_InGameFile+0x2B8` guarda/restaura estado previo; no es owner/opener canónico.
- PauseHD no es la capa correcta para inferir Pad1 desde los índices sGameFlags.
- MainEquipWin/SubEquipWin siguen significando main/sub-weapon, no P1/P2.

## Siguiente bloque

1. Identificar navegación/confirmar/cancelar/opciones de state 5.
2. Enrutarla a Pad1 solo si MenuOwnerRouter.owner==1 y Sub0 sigue válido.
3. Limpiar owner al cerrar state 5/8 o invalidarse la sesión.
4. Auditar nested states 6/7 y callers alternativos de state 5/8.
5. Solo después declarar pausa/menu P2 cerrado.

No se ejecutó gameplay.
