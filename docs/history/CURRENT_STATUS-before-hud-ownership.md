# Estado actual — continuación del 25 de septiembre de 2026

**Implementación parcial. El cooperativo local completo todavía no está terminado ni validado jugando.** No confundir ejecución nativa de un helper aislado con ejecución del juego.

## Último candidato construido

```
SCRIPT SERIAL GUARD EXPERIMENTAL
SHA-256 71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4
Tamaño 60.755.456 bytes
Base inmediata 0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378
Original enero 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69
```

Esta continuación conserva el módulo LOCAL ROUTING EXPERIMENTAL, reproducido byte por byte desde sus fuentes. Añade una selección de miembro protegida a dos callbacks de guion. Solo el owner cFsmActionPcsSub exacto, con Sub0 local en Pad y contexto/serial concordantes, obtiene el miembro 1. Los demás casos conservan 0.

Fuentes y evidencia: [docs/22-script-serial-validation.md](docs/22-script-serial-validation.md), `patches/build_script_serial_guard.py` y `research/patches/script_member_serial_guard.S`.

## Qué se conserva y qué cambia

Se conservan los cambios anteriores de re-enlace de Sub0, cadena de sensor/actor/serial en puertas gimmick, callback real de pickup, selección recíproca de ayuda y cuatro montajes de ActionCommand. Esta continuación no los implementa otra vez.

Se añaden los callbacks `0x02976C30` y `0x029FF600` para la subclase comprobada. El cambio es de 92 bytes efectivos, incluido el checksum; se preservan `.lcfix/.lcdata` y todo el resto del archivo. La reversión es exacta.

El constructor paralelo anterior `build_pcs_script_pad.py` se conserva como histórico: no es el candidato con validación de serial descrito aquí.

## Pruebas y distribución

Nueve tests locales sin omisiones, 1.038 escenarios del intérprete limitado y 36 comprobaciones con imágenes reales. Windows x86 ejecutó los 72 bytes exactos del helper en 596 escenarios, comprobando registros y pila, sin funciones del motor. Linux y Windows pasaron la CI de run `36127817884`, commit `79e86fc19e8494ff0865665ed1e7fb4d34e0afcd`.

`tools/portable_script_patch.py` permite generar y aplicar el manifiesto distribuido desde el EXE original, sin compiladores. La aplicación completa reprodujo el candidato exacto y rechazó ocho casos negativos. Los binarios permanecen fuera de GitHub.

## Punto exacto pendiente

La auditoría de [HUD de hierbas](docs/23-herb-hud-owner-audit.md) identifica `uGUI_MapBaseAndHerb::update 0x02B61D60`: busca Self y actualiza su contador de instancia `+0x294` desde el pack del actor. Son 25 comprobaciones estáticas; NO es un HUD 2P implementado. La clase Blur no representa al jugador 2.

Continuar por creación/lifetime y enlace actor/viewport de las instancias HUD; no cambiar Self a P2 globalmente ni duplicar el mismo contador. `uPcsInput::0x02DEE8D0` sigue pendiente de ownership demostrado. Tampoco están completos pausa/inventario por jugador, muerte/checkpoints/cutscenes, escenas sin partner ni la validación del orden real de callbacks y render/controles en campaña.

## Rectificaciones preservadas

v12 había modificado el callback de uObjModel en vez del de uItem; documentos 17/18 lo corrigen. v14 solo corregía una consulta final de puerta; el módulo posterior añade selección/serial. MainEquipWin/SubEquipWin significan equipo principal/secundario, no P1/P2. No alterar Network ni el serial/Self/GameMode global.

El estado anterior se conserva íntegro en `docs/history/CURRENT_STATUS_before_script_serial.md`.

## Consolidación de las dos continuaciones, sin aplicar dos veces el parche

Se han reunido los cambios de `fc8211a20ac570dadff57ed304a3dec9b6a88ce5` y la continuación independiente `0e4be9db4c2fa4362dfa627918f029adbaaa5dab`. Ambos constructores actuales generan exactamente el mismo candidato `71f5e70d...`. **`build_script_member.py` y `build_script_serial_guard.py` son alternativas, no pasos consecutivos.** La primera salida independiente `a7abf154...` queda histórica y no se distribuye como actual.

La continuación independiente añade 16 tests locales con imágenes, 2.592 combinaciones de guards, pruebas de ABI ejecutadas en Linux i386 y un instalador acumulativo reversible. CI de su código: commit `2affdb8d5c9bc4022c69fad87eb0d20c31cd27d6`, run `36127918860`, dos jobs correctos. Esas cifras no se suman a las de la otra suite para fingir cobertura de gameplay. Evidencia en [docs/script-member-validation.md](docs/script-member-validation.md).

También añade 29 comprobaciones de RTTI/llamadas en [docs/gui-and-scheduler-ownership.md](docs/gui-and-scheduler-ownership.md): uPcsInput tiene padre `uScheduler` (DTI `0x0579963C`), no un jugador demostrado. Mira, equipo, mapa/hierbas y pausa contienen búsquedas de Self. Se identificó `uCockpitManagerMain::0x02B48510`, creación y slot de mira `+0x90`; continuar por `0x02B492D0 / 0x02B49A60 / 0x02B49DA0` y asociación de cada widget a actor y viewport.

`research/current_state.json` está actualizado con esta continuidad, conservando los datos de la implementación previa. Las versiones íntegras de los dos estados antes de consolidarlos quedan en `docs/history/CURRENT_STATUS-independent-before-consolidation.md` y `CURRENT_STATUS-parallel-before-consolidation.md`.

El paquete local de `patches/portable_script_member/apply.py` contiene el payload de SHA `deef32213898d607995c41b2986e5de4b864da4401c8763a6361fd68513d528d`; su generador está en el repositorio. Es una segunda receta reversible desde el original y no debe mezclarse con el formato de `tools/portable_script_patch.py`.

**No se ha ejecutado Resident Evil Revelations. Siguen abiertas implementaciones de interfaz, transiciones completas y escenas sin compañero; no falta únicamente probar el juego.**
