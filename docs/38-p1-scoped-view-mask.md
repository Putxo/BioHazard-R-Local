# 38 — Máscara P1 por vista durante el bucle stock

Continúa el PR #11 integrado en `3d624e39aec66063e67208107e587abffdd3062f`. Solo fuentes, tests y evidencia; no se genera ni modifica un EXE.

## Problema cerrado por este componente

Los clones P2 de Reticle, MainEquipWin y MapBaseAndHerb se dibujan manualmente antes de los bucles stock. Sin otra medida, el bucle stock todavía puede dibujar las instancias Self/P1 en la vista 1 si su `mDrawView` incluye ese bit. No debe ocultarse todo el cockpit: muchos widgets siguen siendo compartidos y no tienen clon P2.

Los tres originales con clon están localizados por ownership nativo:

- `uGUI_MainEquipWin = uCockpitManagerMain+0x5C`;
- `uGUI_Reticle = uCockpitManagerMain+0x90`;
- `uGUI_MapBaseAndHerb = uMiniMapManager+0x30`; `+0x40` es el alias del mismo objeto.

## Implementación

`P1ViewMask` abre un scope solo en draw de vista 1 y únicamente después de que `rev_hud_manager_event` haya devuelto éxito, es decir, después del despacho adicional de P2. Valida manager, alias y vtables exactas, guarda los diez bits `mDrawView` y elimina solo el bit de vista 1 de esos originales. No modifica Self, actor, coordenadas, recursos, Active/ForceSkip ni widgets compartidos.

Al salir del bucle stock restaura únicamente los diez bits `mDrawView` observados al entrar. Los demás bits de `cUnit+0x0C` que el motor pueda cambiar sobreviven. Una identidad de widget cambiada o una escritura fallida bloquea el scope; no se escribe sobre un puntero que haya dejado de pertenecer al slot.

Los gateways de fase11 existentes llaman al begin después del despacho P2. Se añaden dos salidas: `0x02B49E5C` para Cockpit y `0x02B687F0` para MiniMap. La salida de MiniMap también recibe la rama oculta, por eso `end` sin scope activo es un no-op válido. Un fallo de begin/end solicita `ManagerDriver::stop`; nunca destruye dentro del draw.

## Evidencia del original

La auditoría hash-pinned verifica 16 puntos: tres slots/alias de ownership, dos entradas pre-loop, dos salidas, ambos filtros `mDrawView`, tres vtables y el getter/setter de diez bits. Original: SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`, leído sin modificar.

## Límite

Esto impide duplicar en la vista P2 los tres HUD Self que ya tienen clon. No convierte en per-player los otros widgets, no implementa pausa/inventario y no decide todavía proyección/scissor. Se preserva la transformación nativa hasta terminar el análisis de `context+0xBC` y de la ruta común `0x035DD4E0`.

Los gateways siguen siendo fuentes no instaladas. Las pruebas de componente no equivalen a gameplay.
