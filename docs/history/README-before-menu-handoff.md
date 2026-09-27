# BioHazard-R-Local

Investigación e implementación experimental de cooperativo local, en una sola instancia, para Resident Evil Revelations 1 de PC.

**Estado actual: implementación parcial con pruebas de componentes. No es todavía una versión completa de cooperativo local validada jugando.**

## Empezar aquí

Leer [CURRENT_STATUS.md](CURRENT_STATUS.md), [el informe de la continuación](docs/20-local-routing-built-and-tested.md) y [research/current_state.json](research/current_state.json). Los documentos antiguos conservan errores históricos rectificados después; no deben prevalecer sobre esas tres referencias.

## Candidato más reciente construido

```text
LOCAL ROUTING EXPERIMENTAL
SHA-256 0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378
60.755.456 bytes
```

Parte del candidato owner-fix `e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a`, no de un EXE retail arbitrario. El original de enero es `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.

Añade reenlace de Sub0, selección de actor/serial/mando en puertas gimmick, guards de selector local, selección recíproca en la ruta de ayuda y cuatro montajes de ActionCommand ligados al actor. Conserva el parche de recogida con su owner corregido.

## Construcción y pruebas

```sh
python3 patches/build_local_routing.py BASE_OWNER_FIX.exe SALIDA_NUEVA.exe --report INFORME_NUEVO.json
python3 tests/run_without_game.py
python3 tests/run_without_game.py --sanitize
```

La construcción requiere GNU g++/as/ld con soporte i386. Las pruebas públicas no usan ejecutables ni assets del juego. El EXE derivado permanece local y no se publica en este repositorio.

CI comprobada: [run 36122601817](https://github.com/Putxo/BioHazard-R-Local/actions/runs/36122601817), cuatro jobs correctos, incluidos Windows x86 con motor simulado y Linux i386. Esto no equivale a ejecutar el juego.

## Lo que sigue pendiente

HUD e inventarios/menús por jugador, comandos genéricos FSM/PCS, muerte/checkpoints/cutscenes completos, persistencia y creación de P2 en escenas sin partner, y validación dentro de la build con sus datos compatibles. No se presenta la coincidencia de hashes ni la CI como prueba de campaña funcional.

El [probe de Windows](docs/runtime-probe.md) registra observaciones limitadas y de solo lectura; no convierte una sesión en una validación completa.

## Historial y política

Se conservan código, evidencia, hipótesis y rectificaciones. Los anteriores archivos de entrada están archivados sin cambiar sus bytes en [docs/history](docs/history). Los documentos 00–19 siguen disponibles.

No subir EXE, DLL, PDB, assets propietarios ni objetos de compilación. Sí se conservan fuentes originales del parche, tests, hashes y metadatos. No modificar globalmente la identidad de red ni convertir Network en control local.
