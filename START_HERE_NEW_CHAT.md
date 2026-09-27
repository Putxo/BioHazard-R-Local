# Continuar el cooperativo local

Rama canónica: `research/script-member-serial-validation`.
Leer CURRENT_STATUS.md, docs/50-runtime-composition.md y docs/51-runtime-installer.md.

PR24 integrado, merge 7f1f3029ea2d63dac8527834eac51c4496381ac3, 19 workflows PASS.
Pausa owner J2 ya tiene input/lifetime: no volver al checkpoint PR19.

El usuario solicita completar todas las funciones y subir progresos a GitHub.
**Prohíbe abrir o ejecutar el juego.** Las pruebas de gameplay las hará él.
Trabajar con fuentes, análisis estático, compilación y tests sintéticos. No
interpretar CI, mocks o una copia parcheada como cooperativo terminado.

Nuevo pipeline: tools/build_january_base.py -> tools/build_runtime.py ->
tools/runtime_image.py. Es experimental y actualmente cubre tres widgets HUD;
faltan HUD completo/Genesis/acciones/scripts/checkpoints/cutscenes/sin partner.
