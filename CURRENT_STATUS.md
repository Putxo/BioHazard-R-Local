# Estado actual — fases del HUD separadas por gestor

25 de septiembre de 2026. Progreso solo en fuentes, tests y evidencia. No se ha generado/modificado un EXE del juego ni instalado hooks. El cooperativo completo continúa sin estar terminado ni probado en campaña.

## Avance sobre PR #8

Se corrigió un defecto de integración de Lifecycle: su dispatch agrupaba tres widgets y una sola marca de fase. Cockpit y MiniMap necesitan recorridos y contextos separados. La nueva dispatch_manager comprueba familia/dirección/clase, ejecuta solo sus widgets y cuenta cada fase por gestor. No se rehacen Registry, JanuaryBackend ni los parches de input, pickup, puertas, ayuda o guiones.

ManagerDriver conecta los eventos de seis entradas ordinarias comprobadas a ese despacho. Comprueba ticket Live, hilo, frame, generaciones de gestores y contexto. Los eventos antiguos no sustituyen el estado nuevo; los cambios de vida solicitan stop sin ejecutar destructores dentro del evento.

Hay seis gateways i386 fuente que preservan registros, flags, pila y estado x87/MMX/XMM/MXCSR, y reproducen sus dos instrucciones desplazadas. El minimapa engancha su entrada después de la condición de omisión, no el epílogo común que también recibe el camino oculto. La tabla de continuaciones no es un constructor de EXE.

## Pruebas de esta continuación

Código 78dcc277d7cc1b21bcfa28983abf6bc1348922b6: 35 escenarios / 1.362 aserciones con motor simulado, normales y ASan/UBSan; cinco tests Python locales sin omisiones, con 31 witnesses del original exacto leído sin modificar.

Run 36187871978: los tres jobs del componente correctos. El job 108245604011 ejecutó los seis gateways i386 reales contra receptor/continuaciones sintéticos y verificó conservación de estado. La prueba privada del original se omite expresamente en CI. No se ha ejecutado el motor del juego.

## Qué sigue sin estar activado

Los gateways NO están instalados. ManagerHost todavía requiere un proveedor nativo fiable de frame/Session y generaciones de actores/gestores, y scopes reales de entrada/salida de render. phase_scope aporta solo permiso de rama: no concede Structural/Destroy ni certifica render drenado. No se han reemplazado esas precondiciones por true.

El sink tiene binding previo único y vida de proceso; no implementa hot-unload ni concurrencia arbitraria. Los objetos de P1 siguen bajo sus managers originales. Enmascarado de P1, transformación/clipping y el HUD completo dibujado siguen abiertos.

## Continuación exacta

Seguir el binder 0x02DF4F30 y las notificaciones de vida de actores/gestores para producir ManagerFrame/Session fiables; determinar frame y exclusión real del render. Mantener separadas ambas familias y no volver a conectar la antigua dispatch agrupada a los hooks del motor. Después conectar scopes y activar selectivamente los gateways sin generar un EXE mientras no se solicite.

Documentación nueva: docs/30-hud-manager-phase-boundaries.md y docs/31-hud-manager-phase-driver.md. Estado estructurado: research/hud_manager_phase_state.json. Las precondiciones y código anteriores siguen en docs/29 y JanuaryBackend.

Estado anterior conservado íntegro en docs/history/CURRENT_STATUS-before-manager-phase-driver.md. Los estados JSON anteriores y la última imagen histórica 71f5e70d... no se renumeran ni se consideran un HUD integrado.

Pausa/inventario por jugador, comandos compartidos de scheduler, muerte/checkpoints/cutscenes, escenas sin compañero y validación conjunta siguen pendientes. Esta actualización distingue fuentes conectadas entre sí, pruebas aisladas y activación efectiva dentro del juego.
