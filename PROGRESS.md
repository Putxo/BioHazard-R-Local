# Progreso publicado y punto de continuación

Actualización: 25 de septiembre de 2026.

**Entrega actual: código, pruebas y progreso en GitHub. No generar ni entregar nuevos EXE, instaladores ni paquetes binarios mientras el propietario no los solicite expresamente.**

## Dónde está el trabajo más reciente

El código consolidado está en [research/script-member-serial-validation](https://github.com/Putxo/BioHazard-R-Local/tree/research/script-member-serial-validation).

Checkpoint de código verificado en esta revisión: [9880eee389db0bde8aff42df0865f0eb0b390869](https://github.com/Putxo/BioHazard-R-Local/commit/9880eee389db0bde8aff42df0865f0eb0b390869), merge del [PR #5](https://github.com/Putxo/BioHazard-R-Local/pull/5) en esa rama de investigación. El PR está fusionado; su destino no fue main.

Estado técnico: [CURRENT_STATUS.md](https://github.com/Putxo/BioHazard-R-Local/blob/research/script-member-serial-validation/CURRENT_STATUS.md).

Instrucción de entrega y continuidad persistida: [AGENTS.md](https://github.com/Putxo/BioHazard-R-Local/blob/research/script-member-serial-validation/AGENTS.md), commit [5118850441bf70ad8d6da2bd7d7c6505ab7bd234](https://github.com/Putxo/BioHazard-R-Local/commit/5118850441bf70ad8d6da2bd7d7c6505ab7bd234).

Este índice permite llegar desde main al progreso publicado. **No fusiona el código experimental en main, no modifica los parches y no constituye una release jugable.**

## Fuentes y evidencia ya versionadas

| Bloque | Ubicación en el checkpoint consolidado |
|---|---|
| Módulo de enrutamiento local, incluidos reenlace/puertas/ayuda | [patches/local_routing](https://github.com/Putxo/BioHazard-R-Local/tree/9880eee389db0bde8aff42df0865f0eb0b390869/patches/local_routing) |
| Corrección del callback real de pickup | [docs/17-actioncommand-owner-correction.md](https://github.com/Putxo/BioHazard-R-Local/blob/9880eee389db0bde8aff42df0865f0eb0b390869/docs/17-actioncommand-owner-correction.md) |
| Selector de guion con owner, actor Pad y serial concordantes | [research/patches/script_member_serial_guard.S](https://github.com/Putxo/BioHazard-R-Local/blob/9880eee389db0bde8aff42df0865f0eb0b390869/research/patches/script_member_serial_guard.S) |
| Pruebas del selector | [patches/test_script_serial_guard.py](https://github.com/Putxo/BioHazard-R-Local/blob/9880eee389db0bde8aff42df0865f0eb0b390869/patches/test_script_serial_guard.py) |
| Evidencia y límites de las pruebas previas | [docs/22-script-serial-validation.md](https://github.com/Putxo/BioHazard-R-Local/blob/9880eee389db0bde8aff42df0865f0eb0b390869/docs/22-script-serial-validation.md) |
| HUD y scheduler: propietarios y creación | [docs/gui-and-scheduler-ownership.md](https://github.com/Putxo/BioHazard-R-Local/blob/9880eee389db0bde8aff42df0865f0eb0b390869/docs/gui-and-scheduler-ownership.md) |
| HUD de hierbas | [docs/23-herb-hud-owner-audit.md](https://github.com/Putxo/BioHazard-R-Local/blob/9880eee389db0bde8aff42df0865f0eb0b390869/docs/23-herb-hud-owner-audit.md) |

Los constructores alternativos de la misma corrección no se aplican en cadena. Se conservan como fuentes/historial, no como una petición de generar otro ejecutable.

## Punto pendiente exacto

Continuar el ciclo de vida, creación y composición de widgets de uCockpitManagerMain en 0x02B48510, 0x02B492D0, 0x02B49A60 y 0x02B49DA0. Determinar una asociación por instancia de interfaz -> actor -> viewport. La auditoría anterior identificó búsquedas explícitas de Self en mira, equipo, mapa/hierbas y pausa; no implementó un HUD de dos jugadores.

No reemplazar globalmente Self ni reutilizar el layout de otra clase. uPcsInput+0x30 se identificó como padre uScheduler; no existe aún evidencia suficiente para tratarlo como actor del jugador. Equipo principal/secundario no significa P1/P2.

Siguen pendientes HUD/menús completos por jugador, comandos compartidos de scheduler, flujo completo de muerte/checkpoints/cutscenes, escenas sin compañero y validación conjunta en campaña.

## Alcance de esta actualización

Se verificaron las referencias remotas, el estado consolidado, el PR #5 y la auditoría de interfaz. Esta actualización publica el índice y la instrucción de entrega; no añade una corrección de gameplay ni vuelve a ejecutar las pruebas históricas. No se ha generado un nuevo EXE.

Los handoffs de la línea v13 que aún figuran en main son históricos respecto a esta continuidad. Consultar el estado de la rama indicada antes de retomar el desarrollo. No interpretar una prueba aislada de un helper como ejecución de Resident Evil Revelations.
