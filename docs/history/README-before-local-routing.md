# BioHazard-R-Local

Investigación para añadir cooperativo local a **Resident Evil Revelations / BioHazard Revelations (PC, MT Framework)**.

> ## Nuevo chat: empieza aquí
>
> **[START_HERE_NEW_CHAT.md](START_HERE_NEW_CHAT.md)**
>
> Si quieres un texto listo para pegar como primer mensaje de otro chat: **[CONTINUE_IN_NEW_CHAT.md](CONTINUE_IN_NEW_CHAT.md)**
>
> Ese archivo contiene el estado canónico completo, la cadena de versiones correcta y el punto exacto desde el que continuar.

## Alcance

Este repositorio documenta **únicamente la investigación realizada en este chat** sobre las builds aportadas aquí.

No se incorporan resultados de otros chats.

## Build principal

`BioRevHD 30-Enero-2013.exe`

SHA-256:

`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`

Build `FullDebugWin32`, usada como base de ingeniería inversa y parche.

## Base experimental canónica actual

**v14 — DOOR GIMMICK PAD2**

`BioRevHD 30-Enero-2013 LOCAL COOP v14 DOOR GIMMICK PAD2.exe`

SHA-256:

`73fe1255697c47a624025ba40b33bab8df5812e76021bdc2d9acdeec3daf56ea`

Estado:

**STATICALLY VERIFIED ONLY — runtime pendiente.**

Builder:

`patches/build_v14_door_gimmick_pad2.py`

Assembly:

`research/patches/door_gimmick_pad_v14.S`

Manifest:

`research/manifests/local-coop-v14-door-gimmick-pad2.json`

## Cadena canónica reciente

```text
v7 input-only
 -> v9 clean persistent split
 -> v10 Coop ItemBox
 -> v11 canonical pickup
 -> v12 pickup Pad 2
 -> v13 symmetric ammo relief
 -> v14 door-gimmick Pad 2
```

SHA canónicos:

```text
v9  5dc7a7a413ce5916c2758be43b3107d5adf00beee93533b4acf5d83cec97c4be
v10 5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6
v11 2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69
v12 5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8
v13 3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa
v14 73fe1255697c47a624025ba40b33bab8df5812e76021bdc2d9acdeec3daf56ea
```

**Importante:** hubo varias iteraciones históricas llamadas v11. La v11 que pertenece a la cadena v12/v13 es la de SHA `2c69c5...`.

## Qué cubre ya la línea actual

- exacto `uPcsPlayerSub0` rastreado hasta su `uNpc`;
- transición canónica `ThinkMode::Cpu(2) -> Pad(1)`;
- `ThinkMode::Network(3)` intacto;
- segundo pad físico/DirectInput y `PadData[1]`;
- restauración completa del selector en la ruta NPC localizada;
- Self/Partner CameraManage simultáneos;
- Self -> VIEW_0 TOP;
- Partner -> VIEW_1 BOTTOM;
- split persistente solo con local coop activo;
- ItemBox Coop sin cambiar globalmente GameMode;
- pickup local del Sub0 exacto;
- ActionCommand del pickup usando Pad 2;
- entrega del pickup al `cBioItemPack` propio de Sub0;
- réplica local de la regla Coop de reparto/relief de munición al otro jugador.

## Punto actual real

La auditoría de **uDoor2pBase** quedó cerrada sin parche: la infraestructura nativa procesa Self y partner por separado y aplica slots 0/1 offline.

La siguiente familia, **door_gimmick::MoveState**, sí tenía una lectura real de Pad 0 hardcodeada aunque el estado puede dirigirse al partner. v14 corrige únicamente ese callsite, protegido por `gLocalCoopActive`.

Detalle:

- **[docs/13-door-2p.md](docs/13-door-2p.md)**
- **[docs/14-door-gimmick-pad-routing.md](docs/14-door-gimmick-pad-routing.md)**

Siguiente trabajo: auditar **otras interacciones/QTE** buscando selectores de pad/member 0 hardcodeados, usos de `mStartPadNo`, `localIndex` en gameplay o gates `pl` que excluyan al Sub0 exacto.

## Documentación principal

1. [START_HERE_NEW_CHAT.md](START_HERE_NEW_CHAT.md)
2. [docs/02-investigation-log.md](docs/02-investigation-log.md)
3. [docs/03-hypotheses-and-discarded-paths.md](docs/03-hypotheses-and-discarded-paths.md)
4. [docs/13-version-lineage.md](docs/13-version-lineage.md)
5. [docs/12-pickup-coop.md](docs/12-pickup-coop.md)
6. [docs/12-v11-pickup-actioncommand.md](docs/12-v11-pickup-actioncommand.md)
7. [docs/13-door-2p.md](docs/13-door-2p.md)
8. [docs/11-sub-inventory-fsm-ui.md](docs/11-sub-inventory-fsm-ui.md)
9. [docs/10-physical-pad-binding.md](docs/10-physical-pad-binding.md)
10. [docs/09-inventory-coop.md](docs/09-inventory-coop.md)
11. [docs/08-player-pad-sync.md](docs/08-player-pad-sync.md)
12. [research/current_state.json](research/current_state.json)

## Historial y trazabilidad

Se conservan:

- hipótesis correctas;
- errores;
- caminos descartados;
- versiones superseded;
- builders;
- manifests;
- evidencia de offsets;
- correcciones posteriores.

No borrar trabajo antiguo: forma parte de la auditoría pedida.

## Política del repositorio

No se suben EXE/DLL/PDB/assets propietarios.

Sí se suben:

- hashes;
- offsets;
- desensamblado mínimo;
- documentación;
- builders reproducibles;
- manifests;
- assembly propio;
- rectificaciones;
- estado actual.

## Regla para continuar

Base canónica actual: **v14 DOOR GIMMICK PAD2**.

No volver a auditar `uDoor2pBase` como pendiente: quedó compatible estáticamente y no necesitó parche.

Siguiente trabajo exacto:

- auditar otras interacciones/QTE;
- localizar selectores `0` hardcodeados o getters que ignoren su selector;
- comprobar `mStartPadNo` y `localIndex` sólo cuando afecten gameplay;
- comprobar gates `pl` que puedan excluir al Sub0 exacto;
- preservar stock/online cuando `gLocalCoopActive=0`.
