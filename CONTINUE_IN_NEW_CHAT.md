# Prompt listo para pegar en un chat nuevo

Continúa la investigación de cooperativo local de **Resident Evil Revelations 1** desde el repositorio:

`Putxo/BioHazard-R-Local`

Antes de hacer ningún cambio:

1. lee completo `START_HERE_NEW_CHAT.md`;
2. lee `research/current_state.json`;
3. lee `docs/13-door-2p.md`;
4. consulta `docs/02-investigation-log.md` y `docs/03-hypotheses-and-discarded-paths.md` cuando necesites trazabilidad;
5. no rehagas trabajo ya cerrado.

La **base canónica actual es v13 SYMMETRIC AMMO RELIEF**:

```text
BioRevHD 30-Enero-2013 LOCAL COOP v13 SYMMETRIC AMMO RELIEF.exe
SHA-256:
3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa
```

Está **verificada estáticamente, no runtime**.

La siguiente investigación exacta es **puertas/interacciones 2P**:

```text
uDoor2pBase
setter actor-específico:
0x01C9217B -> 0x02588D20

callsites prioritarios:
0x0257185D
0x0257198A
```

Debes:

- identificar el owner/state que contiene esos callsites;
- reconstruir el actor exacto pasado a `0x02588D20`;
- demostrar si PcsSub/Sub0 llega nativamente al slot 1;
- separar lógica de gameplay de lógica de red/feedback que use el índice local global `0x01C85962 -> 0x02DA3050`;
- **no modificar globalmente el índice local**;
- no crear v14 salvo que se demuestre un bloqueo concreto;
- preservar `ThinkMode::Network(3)`;
- preservar comportamiento stock/online cuando `gLocalCoopActive=0`;
- subir a GitHub cada hallazgo, descarte y corrección antes de avanzar demasiado.

No confundas versiones históricas:

- v8 está superseded;
- hubo varias v11 experimentales;
- la **v11 canónica** de la cadena v12/v13 tiene SHA:
  `2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69`.

No vuelvas a tratar como pendientes:

- `0x049A8023`: ya identificado como DTI de `uItem`;
- pickup básico de Sub0: ya resuelto por v11/v12;
- Pad 2 para ActionCommand de pickup: ya resuelto por v12;
- ammo relief Coop: ya resuelto por v13.

Toda la investigación procede **solo del chat original documentado en este repositorio**.
