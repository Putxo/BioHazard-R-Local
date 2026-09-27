# Trabajo y entrega del progreso

Actualización solicitada por el propietario el 25 de septiembre de 2026.

## Entrega vigente: código y progreso en GitHub, no ejecutables

Continuar la implementación y guardar sus fuentes, pruebas, evidencias y estado en este repositorio. No sustituir ese trabajo por la generación o entrega de candidatos binarios.

Mientras el propietario no solicite expresamente un ejecutable:

- No generar nuevos EXE del juego ni ejecutar constructores que creen una imagen parcheada. No entregar EXE, instaladores ni ZIP de distribución en el chat.
- No subir EXE, DLL, PDB, assets propietarios ni objetos de compilación a GitHub. Conservar los ejecutables aportados únicamente como entradas de análisis local de solo lectura.
- Publicar avances mediante commits de código fuente, tests e informes verificables. Las respuestas deben identificar el commit o PR real, los cambios realizados y el punto pendiente, sin enlaces a binarios.
- Conservar fuentes y constructores históricos; que estén versionados no autoriza a ejecutarlos para producir otro EXE. No borrar archivos ni historial para aplicar esta preferencia.

## Continuidad

Antes de escribir, verificar ramas y PR remotos, leer CURRENT_STATUS.md y research/current_state.json y revisar los diffs. Preservar el trabajo paralelo, evitar force-push y no aplicar dos veces los constructores alternativos del mismo cambio.

Checkpoint consolidado de código anterior a esta instrucción: 9880eee389db0bde8aff42df0865f0eb0b390869, integrado mediante PR #5 en research/script-member-serial-validation. La publicación de instrucciones o índices no convierte los cambios experimentales en una implementación completa ni los fusiona automáticamente en main.

Punto pendiente documentado: docs/gui-and-scheduler-ownership.md y docs/23-herb-hud-owner-audit.md. Continuar el ciclo de vida y asociación instancia de HUD -> actor -> viewport en uCockpitManagerMain, sin reemplazar globalmente Self ni interpretar uPcsInput+0x30 como jugador: la auditoría lo identifica como uScheduler.

## Evidencia y pruebas

Distinguir análisis estático, pruebas de componentes y ejecución real del juego. No afirmar pruebas que no se hayan ejecutado ni usar hashes reproducibles como prueba de funcionamiento de gameplay. La prohibición de generar EXE no impide seguir analizando las entradas locales y desarrollando código fuente o tests de componentes sin generar una imagen del juego.

Persistir cada bloque verificable antes de avanzar demasiado. HUD/menús por jugador, comandos de scheduler compartidos, muerte/checkpoints/cutscenes, escenas sin compañero y validación conjunta en campaña siguen pendientes; no darlos por cerrados por actualizar documentación.
