# BioHazard-R-Local

Investigación para añadir cooperativo local a **Resident Evil Revelations / BioHazard Revelations (PC, MT Framework)** a partir de varias builds proporcionadas por el propietario del repositorio.

## Objetivo

Conseguir, en una sola instancia del juego:

- dos jugadores locales reales;
- P1 y P2 con entrada independiente;
- segundo mando asignado a P2;
- dos cámaras independientes;
- pantalla partida;
- reutilizar al máximo la infraestructura que ya existe en las builds de desarrollo.

## Política del repositorio

Este repositorio **no contiene ejecutables del juego, DLL propietarias, assets ni PDB propietarios**. Solo contiene:

- hashes y metadatos de las copias analizadas;
- offsets/RVA/VA y nombres encontrados;
- notas de ingeniería inversa;
- hipótesis, incluidas las descartadas;
- diffs mínimos de experimentos propios;
- scripts de análisis reproducibles;
- resultados y estado de validación.

Los binarios originales se mantienen fuera de Git.

## Build base actual

La candidata principal es **BioRevHD 30-Enero-2013.exe** (FullDebugWin32).

Motivos verificados:

1. Es mucho mayor que las demás builds (~60,7 MB).
2. Conserva gran cantidad de infraestructura y strings de depuración.
3. Su CodeView apunta a:
   `E:\BHR\Source\BioRevHD\buildout\FullDebugWin32\BioRevHD.pdb`
4. Conserva opciones de depuración `CreateRaidPlayer 1..5`, además de `ForceTwoPlatoon`, `PickingCoopLock`, `NotSendPad`, `UseOtomo`, `mSelfID` y `mPartnerID`.
5. Conserva infraestructura de varios viewports (`VIEW_0..VIEW_7`) y referencias de entrada/cámara útiles.

La build **23-Feb-2013 prototipo** sigue siendo muy útil como comparación, pero parte de la lógica de creación de jugadores de Raid ya aparece anulada/stubbed respecto a enero.

## Estado actual

La investigación se está separando en tres problemas independientes:

1. **P2 / entidad** — crear o reutilizar un segundo actor/jugador real.
2. **Input** — hacer que P2 lea el slot de mando 1 (segundo XInput) en vez del mismo input que P1.
3. **Cámara / viewport** — asignar una cámara y viewport independientes a P2 y configurar split-screen.

No se considera ninguna build experimental como “funcional” hasta verificarla dentro del juego.

## Documentación

- [Inventario de builds](docs/01-build-inventory.md)
- [Diario completo de investigación](docs/02-investigation-log.md)
- [Hipótesis y caminos descartados](docs/03-hypotheses-and-discarded-paths.md)
- [Mapa técnico actual](docs/04-current-architecture-map.md)
- [Experimentos binarios](docs/05-experiments.md)

## Principio de trabajo

Se documentan también los errores y pistas falsas. Si una cadena, campo o función parecía relacionada con cooperativo local pero después se comprobó que pertenecía a otro subsistema, queda registrada igualmente con la evidencia que llevó a descartarla.
