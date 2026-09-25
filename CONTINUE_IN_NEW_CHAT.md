# Prompt de continuación

Continúa exclusivamente Putxo/BioHazard-R-Local. Lee CURRENT_STATUS.md, docs/20-local-routing-built-and-tested.md y research/current_state.json; verifica main, ramas y PRs antes de escribir. No rehagas pickup desde v12 ni vuelvas a 0x049A8023.

El candidato LOCAL ROUTING EXPERIMENTAL construido es SHA-256 0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378, 60.755.456 bytes. Base inmediata owner-fix e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a. El constructor y el módulo están en patches/build_local_routing.py y patches/local_routing/.

Ya se implementaron reenlace del mismo Sub0 local, cadena sensor/actor/serial/callback de puertas gimmick, guards del selector, cuatro montajes de ActionCommand y selección recíproca en la ruta de ayuda. Hay pruebas locales de lógica, puentes e integridad, y CI de fuentes con mocks en Linux i386 y Windows x86. No se ejecutó el juego y no está implementado el cooperativo completo.

Pendientes concretos: ownership de selectores de guion cFsmAction 0x02976C30, cFsmActionPcs 0x029FF600 y uPcsInput 0x02DEE8D0; HUD/pausa/inventarios por jugador; muerte/checkpoints/cutscenes; creación y persistencia de P2 en escenas sin partner; prueba de orden de callbacks y vida de bindings en el motor real. No cambiar a ciegas todos los ceros ni reemplazar Self o el serial global.

El probe tools/runtime_probe.py es de solo lectura y está limitado por hash al candidato. No hay todavía observaciones de una partida real con él. No llamar a sus tests con fixtures una validación de gameplay.

Preserva Network y trabajo paralelo. Usa los EXE solo localmente, jamás los subas a GitHub. Publica código, tests, evidencia y rectificaciones; no prometas una versión completa basándote únicamente en hashes o CI.
