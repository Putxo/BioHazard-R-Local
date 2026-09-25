# Estado actual — script serial experimental

Fecha: 25 de septiembre de 2026. **Implementación parcial; no es una versión cooperativa completa ni validada jugando.**

## Candidato reproducido

```
Original enero: 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69
Base LOCAL ROUTING: 0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378
SCRIPT SERIAL: 71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4
Tamaño final: 60.755.456 bytes
```

La base fue reconstruida exactamente desde las fuentes publicadas de `5bcf15ad...`. Se conservan sus correcciones de pickup, puertas, ayuda y reenlace; las secciones `.lcfix/.lcdata` son idénticas a las de esa base.

La nueva salida coincide byte por byte con el candidato paralelo de `research/script-member-serial-validation` en `ec64e21a...`. **Son recetas alternativas para el mismo cambio, no dos parches para apilar.** La primera construcción `a7abf154...` de esta rama queda histórica: se adelantó la comparación de contexto para evitar leer el actor cuando no corresponde.

Constructor incremental: `patches/build_script_member.py`. Instalador acumulativo desde el original: `patches/portable_script_member/apply.py`, con payload reproducible mediante `make_payload.py`. Documentación: `docs/script-member-validation.md`.

## Cambio añadido

`cFsmAction::0x02976C30` y `cFsmActionPcs::0x029FF600` comparten una función nueva en `0x01C95340`. Devuelve Pad 2 solo para un owner exactamente `cFsmActionPcsSub`, una sesión local activa, contexto `+0x1078` igual al tracker, actor en Pad y serial `+0x1074` coincidente y no negativo. Se conserva cero para Main/globales, otras clases, Cpu, Network y contextos inválidos.

Son 92 bytes efectivos sobre LOCAL ROUTING y se revierte exactamente a esa base. No se cambia `uPcsInput::0x02DEE8D0` ni se fuerza globalmente Self, GameMode o el serial de red.

## Verificaciones realizadas

- 16 tests locales con original/base/candidato reales, incluida una matriz de 2.592 combinaciones y comprobaciones del PE, reversión y rechazos de CLI.
- CI del código `2affdb8d5c9bc4022c69fad87eb0d20c31cd27d6`: run `36127918860`, dos jobs correctos. El job nativo ejecuta la función ensamblada en un proceso Linux i386 de pruebas, sin enlazar el motor. El job portable omite tres tests que necesitan las imágenes privadas.
- 17 comprobaciones del instalador acumulativo, incluidas original→candidato→original mediante CLI y protección de los archivos existentes.
- 29 comprobaciones estáticas adicionales de RTTI y llamadas de interfaz/scheduler.

**Ninguna de esas pruebas ejecuta Resident Evil Revelations.** Los resultados de componentes de LOCAL ROUTING se conservan en el estado histórico y en `docs/20-local-routing-built-and-tested.md`.

## Continuación exacta: interfaz y comandos compartidos

`uPcsInput+0x30` remite a un **uScheduler**, DTI `0x0579963C`, no a un actor demostrado. Su callback cero permanece intacto; no sustituirlo a ciegas por Pad 2.

La interfaz sigue consultando Self en `0x02B404C6` (mira), `0x02B3B6C8` (armas), `0x02B61F3C` (mapa/hierbas) y `0x02C30A06` (estado de pausa). Se identificó `uCockpitManagerMain`, constructor `0x02B477A0`, creación de widgets `0x02B48510` y slot de mira `+0x90`.

Seguir `0x02B492D0 / 0x02B49A60 / 0x02B49DA0` y la asociación instancia→actor→viewport. No cambiar el finder Self global ni interpretar MainEquipWin/SubEquipWin como P1/P2. Detalle y auditor reproducible: `docs/gui-and-scheduler-ownership.md` y `scripts/audit_gui_ownership.py`.

Siguen abiertos HUD/menús por jugador, comandos compartidos de uPcsInput, flujo completo de muerte/checkpoint, cámaras forzadas/cutscenes y escenas sin partner. No se ha implementado aquí la creación de un P2 donde no existe compañero. Tampoco se han probado orden/lifetime reales del motor ni una partida con dos mandos.

No volver al DTI de uItem ni a rehacer las correcciones anteriores. Los estados previos completos se conservan en `docs/history/*before-71f5-script-serial*`. Los binarios del juego permanecen fuera de GitHub.
