# Estado actual — observador de vida y enlace PCS, sin generar binarios

Continuación de PR #9 / 9e8dfec08842d8e199a59932074220cc5581f96c. Se recuperó el trabajo interrumpido en research/hud-lifetime-observer y se corrigió antes de integrarlo. Solo fuentes, pruebas y evidencia en GitHub. No se ha generado ni modificado otro EXE del juego ni instalado hooks en un proceso.

## Implementación recuperada y completada en esta revisión

LifetimeSource consume eventos de construcción, inicio de destrucción, comienzo/final del enlace PCS y destrucción de PCS Main/Sub0. Produce Session/Actor y generaciones de los dos gestores para los componentes de HUD existentes. No inventa una vida nueva al leer PCS+44, ni convierte un reenlace del mismo actor en otro nacimiento.

Los avisos de destrucción invalidan las asociaciones de P2 sin leer el objeto que está destruyéndose. Los avisos derivados/base duplicados no cuentan como otra destrucción. Reutilizar una dirección tras un nacimiento observado genera otra identidad y no resucita los widgets anteriores. La captura relee actores, modo Pad, serial, gestores, flag local, tracker y Self, y no publica una captura parcial si hay cambios reentrantes.

Este es un observador alimentado por eventos y callbacks. Los once gateways fuente NO están instalados. Su activación requiere cobertura de los eventos antes de que empiece la construcción del mundo y un contrato de lectura/hilo válido. No permite adjuntarse tarde y tratar actores ya existentes como recién nacidos.

## Error concreto corregido antes de la integración

El BEGIN propuesto en 0x02DF4F99 quedaba detrás de la salida anticipada por PCS+50. Esa salida llegaba a END 0x02DF504B sin BEGIN, provocando un fallo de protocolo falso.

BEGIN se mueve a **0x02DF4F65**, antes del test, y continúa en 0x02DF4F6C tras reproducir los siete bytes originales. El punto antiguo deja de aceptarse. END permanece intacto. No se superponen los hooks del enlace local previo en 0x02DF4FA7/0x02DF5015. Corrección: docs/33-lifetime-early-exit-correction.md.

## Verificación de esta continuación

Código fabb38fb5028cec210603ab9aac6410be246581a, run 36193139147: los tres jobs del observador pasaron. La suite de fuente y Registry reales ejecutó 34 escenarios / 940 aserciones, normal y ASan/UBSan, con memoria/Self simulados. El harness i386 ejecutó los once gateways en doce invocaciones, incluyendo byte +50 cero y no cero, con receptor/continuaciones sintéticos. No se ejecutó el motor.

Auditor de solo lectura: 46 comprobaciones del original de enero y hash intacto. Ocho tests Python: seis pasaron localmente y dos comprobaciones del mapa de fuentes se omitieron por no estar montado el árbol completo; esas dos sí pasaron en CI. En CI pasaron siete y solo se omitió el test de la imagen privada, que pasó localmente. No se ocultan ni suman esas omisiones como pruebas de gameplay.

El auditor y su suite publicados coinciden por hash de blob con las copias locales ejecutadas. Informe: research/reports/hud-lifetime-observer-validation.json. Evidencia/contrato: docs/34-lifetime-observer-continuation.md.

## Continuación exacta y límites

La fuente entrega LifeSnapshot sin frame de presentación. El contador de sUnit encontrado se detiene en la rama de pausa: no sirve para deduplicar todos los dibujos. No se sustituye por incrementar un contador en cada callback de GUI. La fuente de presentación, exclusión real de render en vuelo y scopes de transformación/clipping siguen pendientes.

Conectar la cobertura de eventos de vida antes del mundo y la captura al ManagerHost ya existente; completar el reloj de presentación y la admisión/scope nativa sin constantes true. Mantener separados los gestores y sus marcas de fase. El código no otorga por sí solo permisos de asignación, liberación ni un fence de render.

Se conservan JanuaryBackend, Lifecycle, Registry, ManagerDriver, las correcciones de input/pickup/puertas/ayuda/guiones y los gateways anteriores. La última imagen histórica 71f5e70d... no se renumera ni se reconstruye. El estado previo se archiva íntegro en docs/history/CURRENT_STATUS-before-lifetime-observer.md; el estado estructurado de esta continuación está en research/hud_lifetime_observer_state.json.

El segundo HUD no se ha activado ni dibujado en campaña. Siguen pendientes scopes/máscara P1, pausa/inventario por jugador, comandos compartidos, flujo completo de muerte/checkpoints/cutscenes, escenas sin compañero y validación conjunta. La integración de estas fuentes no declara terminado el cooperativo local.
