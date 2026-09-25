# Prompt listo para pegar en un chat nuevo

Continúa la investigación de cooperativo local de **Resident Evil Revelations 1** desde el repositorio:

`Putxo/BioHazard-R-Local`

Antes de hacer ningún cambio:

1. lee completo `START_HERE_NEW_CHAT.md`;
2. lee `research/current_state.json`;
3. lee `docs/14-door-gimmick-pad-routing.md`;
4. consulta `docs/13-door-2p.md`, `docs/02-investigation-log.md` y `docs/03-hypotheses-and-discarded-paths.md` para trazabilidad;
5. no rehagas trabajo ya cerrado.

La **base canónica actual es v14 DOOR GIMMICK PAD2**:

```text
BioRevHD 30-Enero-2013 LOCAL COOP v14 DOOR GIMMICK PAD2.exe
SHA-256:
73fe1255697c47a624025ba40b33bab8df5812e76021bdc2d9acdeec3daf56ea
```

Base v13:

```text
3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa
```

Está **verificada estáticamente, no runtime**.

No vuelvas a tratar como pendientes:

- `0x049A8023`: DTI de `uItem`;
- pickup básico de Sub0: v11/v12;
- Pad 2 del ActionCommand de pickup: v12;
- ammo relief Coop: v13;
- auditoría base de `uDoor2pBase`: compatible estáticamente, sin parche;
- `door_gimmick::MoveState` Pad 0 hardcodeado: corregido por v14 en `0x02591118`, helper `0x01C95300`.

v14 preserva stock/online:

```text
gLocalCoopActive == 0 -> selector 0 original
gLocalCoopActive != 0 -> selector del actor
P1 -> 0
exact Sub0 -> 1
```

Diff v14 vs v13:

```text
49 bytes
2 rangos
mismo tamaño PE
reversible byte por byte
```

La siguiente investigación exacta es **otras interacciones/QTE**.

Debes:

- buscar selectores de pad/member `0` hardcodeados;
- buscar APIs que reciban selector pero sigan usando `mStartPadNo`;
- separar usos de `localIndex` de red/presentación de los que afecten gameplay;
- buscar gates `pl` que excluyan al exact Sub0;
- no tocar globalmente el índice local;
- no secuestrar `ThinkMode::Network(3)`;
- preservar stock/online cuando `gLocalCoopActive=0`;
- crear v15 solo si se demuestra un bloqueo concreto;
- persistir cada hallazgo/corrección en GitHub antes de avanzar demasiado.

No confundas versiones históricas. La cadena canónica reciente es:

```text
v9 -> v10 -> v11 canonical -> v12 -> v13 -> v14
```

v11 canónica:

`2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69`

Toda la investigación procede únicamente del chat original documentado en el repositorio.
