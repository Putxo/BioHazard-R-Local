# BioHazard-R-Local

Investigación para añadir cooperativo local a **Resident Evil Revelations / BioHazard Revelations (PC, MT Framework)**.

## Alcance de esta rama

Esta documentación contiene **únicamente el trabajo realizado en esta conversación** sobre los cuatro ejecutables aportados aquí:

- `BioRevHD 23-Feb-2013 prototipo(2).exe`
- `BioRevHD 30-Enero-2013.exe`
- `rerev Feb 7, 2024 retail.exe`
- `rerev May 17, 2013 prototipo.exe`

No se incorporan archivos, experimentos ni resultados procedentes de otros chats o trabajos anteriores.

## Objetivo

Conseguir, en una sola instancia del juego:

- dos jugadores locales reales;
- P1 y P2 con entrada independiente;
- segundo mando asignado a P2;
- dos cámaras independientes;
- pantalla partida;
- reutilizar al máximo la infraestructura ya presente en las builds de desarrollo.

## Política del repositorio

No se suben ejecutables, DLL, PDB, assets ni otros archivos propietarios del juego. Solo se documentan:

- hashes y metadatos;
- offsets/RVA/VA;
- nombres de clases, campos y funciones;
- desensamblado mínimo necesario como evidencia;
- hipótesis y caminos descartados;
- scripts de análisis reproducibles.

## Base de investigación actual

La candidata principal es **BioRevHD 30-Enero-2013.exe** porque es una build `FullDebugWin32` mucho más rica en RTTI, strings, menús de depuración y lógica eliminada posteriormente.

La build **23-Feb-2013 prototipo** se usa como comparación para detectar qué funciones fueron anuladas entre enero y febrero.

## Estado actual

La investigación está separada en tres problemas:

1. **Entidad P2** — localizar/reutilizar la infraestructura Main/Sub ya presente.
2. **Input P2** — enlazar un segundo jugador con un pad lógico independiente.
3. **Cámara/viewport P2** — asignar una cámara independiente y renderizarla en un segundo viewport.

Hallazgo más reciente: la build de enero contiene RTTI/clases diferenciadas para **`uPcsPlayerMain`**, **`uPcsPlayerSub0`** y **`uPcsPlayerSub1`**, además de un `sPcsManager` que maneja tres slots Main/Sub0/Sub1.

## Documentación

- [Alcance exacto de esta conversación](docs/00-scope-this-chat.md)

- [Inventario de las cuatro builds](docs/01-build-inventory.md)
- [Diario completo de esta conversación](docs/02-investigation-log.md)
- [Hipótesis y caminos descartados](docs/03-hypotheses-and-discarded-paths.md)
- [Mapa técnico actual](docs/04-current-architecture-map.md)
- [sGamePad, sPcsManager y clases Main/Sub](docs/05-sgamepad-spcsmanager.md)
- [Experimentos estáticos y sus limitaciones](docs/06-experiments.md)
- [RTTI/vtables y matriz nativa Main/Sub](docs/07-pcs-role-vtables-and-mapping.md)
- [ThinkMode y ruta nativa de pads](docs/08-input-thinkmode-and-pad-routing.md)
- [Roles Main/Sub y ruta real de pad](docs/07-player-roles-and-pad-routing.md)
- [ThinkMode y control local](docs/08-thinkmode-local-input.md)
- [Mapa de strings/direcciones](research/key-string-addresses.md)
- [Manifest P2 input](research/manifests/p2-input-experimental.json)
- [Manifest split-screen v2](research/manifests/splitscreen-v2-experimental.json)
- [Native input v3 manifest](research/manifests/native-input-v3-experimental.json)
- [Patcher reproducible de Native input v3](scripts/apply_native_input_v3.py)
- [Scripts reproducibles](scripts/)

## Criterio documental

También se conservan los errores de interpretación hechos **durante este chat**. Si una pista parecía útil y después se demostró que pertenecía a otro subsistema, queda registrada con la razón del descarte.
