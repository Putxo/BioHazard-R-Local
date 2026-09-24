# BioHazard-R-Local

Investigación para añadir cooperativo local a **Resident Evil Revelations / BioHazard Revelations (PC, MT Framework)**.

> ## Nuevo chat: empieza aquí
>
> **[START_HERE_NEW_CHAT.md](START_HERE_NEW_CHAT.md)**
>
> Ese archivo contiene el estado canónico, la base actual, lo confirmado/descartado y el punto exacto desde el que continuar.

## Alcance

Este repositorio documenta **únicamente la investigación realizada en este chat** sobre las cuatro builds aportadas aquí.

No se incorporan resultados de otros chats.

## Objetivo

Conseguir, en una sola instancia:

- P1 y P2 locales reales;
- dos mandos físicos independientes;
- Sub0/partner controlado localmente;
- dos cámaras y pantalla partida;
- inventario/equipamiento/pickups correctos;
- HUD, pausa, interacciones, QTE, transiciones y cutscenes compatibles;
- preservar comportamiento stock/online cuando el modo local no está activo.

## Base de ingeniería inversa

Build principal:

`BioRevHD 30-Enero-2013.exe`

SHA-256:

`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`

Es `FullDebugWin32` y conserva RTTI, menús internos y lógica eliminada después.

La build 23-Feb-2013 se usa como comparación: conserva parte de la interfaz debug, pero algunas rutas como `CreateRaidPlayer` terminan en stubs.

## Base experimental canónica actual

**v10**

`BioRevHD 30-Enero-2013 LOCAL COOP v10 CLEAN SPLIT COOP ITEMBOX.exe`

SHA-256:

`5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6`

Estado:

**STATICALLY VERIFIED ONLY — todavía no validado dentro del juego.**

Builder:

`patches/build_v10_itembox_from_v9.py`

Manifest:

`research/manifests/local-coop-v10-itembox.json`

v10 = v9 exacto + 23 bytes efectivos de selección de `sItemBoxCoop`.

## Qué contiene v10

- exacto `uPcsPlayerSub0` rastreado hasta su `uNpc`;
- transición canónica `ThinkMode::Cpu(2) -> Pad(1)`;
- `ThinkMode::Network(3)` intacto;
- segundo pad real mediante `PadData[1]`;
- restauración de selector en la capa PC `sGamePad`;
- Partner target = exacto Sub0;
- Self -> VIEW_0 TOP;
- Partner -> VIEW_1 BOTTOM;
- split persistente solo cuando `gLocalCoopActive=1`;
- comportamiento cámara stock cuando el flag local está apagado;
- selección de `sItemBoxCoop` cuando el local co-op está activo sin cambiar globalmente `mGameMode`.

## Punto exacto actual

La investigación está ahora en la ruta de **pickup/item/ammo/herb del partner**.

Tipo clave:

`sItem::cNetSyncData::cPickupItemSyncData`

Receiver prioritario:

`0x02459C20`

Punto adicional interrumpido por timeout:

`0x049A8023`

Detalle completo:

**[docs/12-pickup-sync-wip.md](docs/12-pickup-sync-wip.md)**

## Documentación principal

1. [START_HERE_NEW_CHAT.md](START_HERE_NEW_CHAT.md) — handoff autocontenido.
2. [docs/02-investigation-log.md](docs/02-investigation-log.md) — cronología exhaustiva.
3. [docs/03-hypotheses-and-discarded-paths.md](docs/03-hypotheses-and-discarded-paths.md) — hipótesis, errores y descartes.
4. [docs/13-version-lineage.md](docs/13-version-lineage.md) — v1→v10 y qué está superseded.
5. [docs/10-physical-pad-binding.md](docs/10-physical-pad-binding.md) — dos pads físicos DirectInput.
6. [docs/11-sub-inventory-fsm-ui.md](docs/11-sub-inventory-fsm-ui.md) — PcsSub/inventario/equipamiento.
7. [docs/12-pickup-sync-wip.md](docs/12-pickup-sync-wip.md) — trabajo exacto pendiente.
8. [docs/09-inventory-coop.md](docs/09-inventory-coop.md) — `sItemBoxCoop`, bags pl/np y GameMode.
9. [docs/08-player-pad-sync.md](docs/08-player-pad-sync.md) — `cPlayerPadSyncData`.
10. [docs/05-sgamepad-spcsmanager.md](docs/05-sgamepad-spcsmanager.md) — `sGamePad` / `sPcsManager`.

## Historial técnico

También se conservan todos los experimentos y errores intermedios.

Entre otros:

- v1/v2: selector de pad insuficiente;
- v3: primera restauración de selector PC;
- v4: split nativo;
- v5: filtro exacto Sub0 manteniendo Cpu;
- v6: Sub0 Cpu→Pad nativo;
- v7: FULLPAD;
- v8: persistent split con herencia de cámara incondicional — **superseded**;
- v9: clean persistent split;
- v10: ItemBox Coop condicional.

No borrar las versiones históricas: forman parte de la trazabilidad solicitada.

## Política del repositorio

No se suben:

- ejecutables del juego;
- DLL/PDB propietarios;
- assets;
- dumps propietarios.

Sí se suben:

- hashes;
- offsets;
- desensamblado mínimo como evidencia;
- documentación;
- builders/patchers reproducibles;
- manifests;
- hipótesis descartadas;
- rectificaciones.

## Estado de archivos locales

Tras un reinicio del entorno, los EXE derivados v5–v10 pueden no seguir presentes localmente.

Los cuatro EXE fuente originales sí son la referencia.

Los derivados deben reconstruirse mediante los builders versionados; no asumir que siguen montados.

## Regla para continuar

No crear una nueva versión por cada hallazgo.

**v10 sigue siendo la base hasta que se demuestre un cambio de código necesario.**

Siguiente trabajo: cerrar `cPickupItemSyncData` y comprobar si los pickups de Sub0 ya llegan a su `cBioItemPack / bag np` o si necesitan un hook local mínimo.
