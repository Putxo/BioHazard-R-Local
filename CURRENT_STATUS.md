# Estado actual — recursos y ciclo de vida del HUD adicional

25 de septiembre de 2026. Entrega de fuentes, tests y evidencia en GitHub. No se ha generado un EXE del juego, instalador ni ZIP en esta continuación. La implementación completa del cooperativo sigue abierta.

## Avance nuevo sobre PR #6

Se implementó `patches/hud_ownership/lifecycle.hpp/.cpp`: preparación de tres widgets independientes, comprobación de sus árboles de recursos, publicación por ticket vigente, dispatch de fases y retirada diferida con invalidación. Usa el registro y las clases del PR #6 sin cambiarlos. No altera los parches previos de input, pickup, puertas, ayuda o guiones.

El controlador preserva los originales P1 y no amplía sus listas/arrays nativos. Una preparación incompleta no se publica. Desactiva el conjunto si cambia el actor o sesión, no destruye en mitad de callbacks y rechaza solicitudes de generaciones antiguas antes de tocar la sesión nueva. Las estructuras de propiedad dudosa quedan en cuarentena en vez de invocar destructores que podrían afectar a P1.

**Este controlador está escrito y probado como código fuente, pero su backend nativo no está implementado/conectado. No se ha creado ni dibujado el segundo HUD dentro del juego.**

## Evidencia nueva del original

Reticle, MainEquipWin y MapBaseAndHerb enlazan una plantilla de recurso a una raíz y tabla de nodos propias de cada instancia. El owner de los nodos se escribe en +6C. Los inicializadores retornan void: hay que comprobar sus postcondiciones, no EAX como booleano.

La constante 0x69 de Reticle configura prioridad y capa, comprobado por consumidores y diagnósticos literales; no se interpreta como un ID de jugador. Esto no resuelve todavía todos los registros globales de GUI.

Detalles: [docs/26-hud-resource-initialization.md](docs/26-hud-resource-initialization.md) y [docs/27-hud-transactional-lifecycle.md](docs/27-hud-transactional-lifecycle.md).

## Verificación

34 escenarios / 828 aserciones del controlador con registro real y motor simulado, tanto normal como con ASan/UBSan. Seis tests Python locales sin omisiones y 52 comprobaciones estáticas del original exacto, que conserva su hash. Sintaxis freestanding i386 verificada sin crear una imagen del juego.

CI del código `5bdc9526b80cd59924f6db073b176b0de2754c0b`: run `36180203408`, dos jobs correctos (normal y sanitizado). La prueba privada del original no se ejecuta en CI. Informe `research/reports/hud-lifecycle-validation.json`.

## Punto pendiente exacto

Implementar el backend nativo tras seguir la inscripción de unidades GUI en grupos y los consumidores de prioridad/capa. Debe garantizar objetos nuevos desvinculados de listas nativas para no ejecutarlos dos veces. Conectar un driver real de Session/lifetime/epoch, notificaciones de destrucción y puntos seguros de render; luego enlazar bridges, fases, visibilidad y transformaciones/clipping para P1/P2.

El controlador no puede garantizar vida de punteros sin ese driver. Su cap de 256 nodos es conservador y la cuarentena puede retener memoria. No se presenta como un HUD 2P integrado ni como una campaña comprobada.

Pausa/inventario por jugador, comandos compartidos de scheduler, muerte/checkpoints/cutscenes y escenas sin compañero siguen pendientes. El último EXE histórico sigue siendo 71f5e70d...; no se ha construido otro ni aplicado este componente a aquella imagen.

Se preserva el estado anterior íntegro en [docs/history/CURRENT_STATUS-before-hud-lifecycle.md](docs/history/CURRENT_STATUS-before-hud-lifecycle.md). `research/hud_lifecycle_state.json` describe esta continuación; los estados y recetas previos permanecen históricos y no se ejecutan para entregar binarios.
