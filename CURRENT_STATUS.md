# Estado actual del cooperativo local

08/10/2026. Rama técnica: `research/script-member-serial-validation`.
PR68 integrado: `61b704d67b2916a9fc5eeba250fc4ab7d804ae39`, 20 workflows PASS.
Prioridad actual: J2 sigue en IA según el propietario. PR60 corrige Cpu(3) -> Pad(1); la cadena de entrega necesitaba además aceptar las bases corregidas y rechazar las antiguas. Véase [corrección de construcción](docs/89-cpu3-cumulative-build.md). No se da por validado el movimiento jugando.

**El cooperativo completo sigue en desarrollo. El juego no se ha ejecutado.**
El propietario prohíbe abrirlo y hará personalmente las pruebas jugando.

## Implementación disponible

| Área | Alcance real |
|---|---|
| Input | **Corrección nueva pendiente de retest gameplay:** Sub0 exacto usa Pad1; Cpu(3) -> Pad(1); Network(2) preservado. El ejecutable exacto probado por el propietario sigue sin identificarse; se confirmó el comparador histórico erróneo con 2 en las bases antiguas. |
| Cámaras | View0 TOP y View1 BOTTOM, targets separados; cutscenes pendientes |
| Inventario | Pack gameplay propio; menú global muestra Sub0 con owner J2 |
| Pausa | Una pausa global; abrir/navegar/confirmar/cancelar por owner; lifetime y nested 6/7 implementados |
| HUD | Clones de Reticle, MainEquipment, MapHerb, SubEquipment, Damage, Heal, Scope y Scanner (detector conectado, activación pendiente); faltan asfixia, Genesis y otros widgets |
| Recursos HUD | Grafo completo conectado, selección desde sIDCockpit, lifetimes y retirada |
| Pickup/puertas | ActionIcon2D por miembro/vista; prioridad y parada del bucle por miembro; otros gates y prompts completos pendientes |
| Guiones | Dos callbacks PcsSub y uPcsInput por scheduler/actor/serial; cobertura QTE parcial |
| Rescate | Routing parcial existente; muerte/checkpoints pendientes |

La compilación acumulativa nueva conecta 426 hooks y diez correcciones de ejes/estado HUD/menú/ActionIcon sobre la base de
input, cámara, pickup, puertas y serial de guion. Se ha generado y verificado
estáticamente una copia local de trabajo. No se ha instalado en Steam ni abierto.
No constituye una entrega final.

## Hallazgos recientes

- PR21/22/23 resolvieron input y lifetime del owner de pausa y lecturas inválidas.
- PR24 conectó el runtime y corrigió el bridge del inventario: el finder stock
  recibe un predicado y devuelve con ret 4.
- La instalación acumulativa encontró dos salidas de máscara P1 emitidas dentro
  de .note.GNU-stack: se descartaban al enlazar. Ya se ubican en .text y se
  verifica que todos los símbolos de enganche estén en la sección ejecutable.
- El fuente pickup_sub0_v11.S vigente era un experimento de 55 bytes; el builder
  necesitaba el original de 248 bytes. Se recuperó del commit 7f7998fc y se guarda
  como pickup_sub0_canonical_v11.S. Los hashes históricos hasta v14 vuelven a coincidir.

## Siguiente trabajo

Completar HUD (HP, interacción y Genesis), arbitraje de comandos por miembro, auditoría de acciones
del mando 2, scripts/QTE, muerte/revive/checkpoints, cutscenes y escenas sin partner.
La compatibilidad trabajada es January 30 2013; no atribuirla a retail/Feb/May.
Seguir publicando avances verificables; nunca subir EXE, assets o símbolos del juego.

Detalle: [runtime](docs/50-runtime-composition.md), [instalador](docs/51-runtime-installer.md).

Panel de subarmas y rollback de máscaras: [detalle](docs/52-subweapon-hud.md).

Icono de interacción: [productor y arbitraje pendientes](docs/53-action-icon-findings.md).

Nuevo aislamiento del dibujo: [ActionIcon por vista](docs/54-action-icon-view-routing.md).
PR28 pasó 19 workflows: [prioridad del bucle por miembro](docs/55-action-priority-loop.md).
Nuevo bloque: [dibujo inmediato local](docs/56-local-draw-scheduling.md).
Persisten gates/flags globales, historial y productores 3D por completar.
El juego no se ha ejecutado.

Indicador de daño separado: [alimentación por actor](docs/57-damage-hud.md).

Genesis: [progreso global, productores y recompensa](docs/58-genesis-ownership-boundaries.md).

Curación por actor: [seis productores y cola por lifetime](docs/59-heal-hud.md).

Genesis: [contador fuente y gateways; integración pendiente](docs/60-genesis-progress.md).

Genesis: [clon y consumidores integrados; activación/objetivos pendientes](docs/61-scanner-clone.md).

Genesis: [cámara propia y límites de productores](docs/62-genesis-camera-producers.md).

Genesis: [vida observada de objetivos](docs/63-genesis-target-lifetime.md).

Genesis: [estado de detección por propietario](docs/64-genesis-target-view-state.md).

Genesis: [lecturas nativas durante la fase del clon](docs/65-genesis-target-consumers.md).

Genesis: [productores de detección por propietario](docs/66-genesis-detector-routing.md).

Genesis: [retirada síncrona y listas privadas](docs/67-genesis-target-removal.md).

Genesis: [notificaciones del mundo y reentrada del detector](docs/68-genesis-world-notifications.md).

Genesis: [arma equipada, retención y límites de activación](docs/69-genesis-equipped-weapon.md).

Apuntar: [visibilidad de personaje y arma en la segunda cámara](docs/70-aim-secondary-visibility.md).

Visor de arma: [octavo widget, recursos y actor propios](docs/71-scope-widget.md).

Visor de arma: [apertura y cierre al apuntar con J2](docs/72-scope-aim-events.md).

Genesis: [retirada durante la notificación de fin de escaneo](docs/73-genesis-completion-boundary.md).

Genesis: [inventario del actor en los ocho estados internos](docs/74-genesis-state-actor-routing.md).

Genesis: [copia privada del recurso mutable de filtros](docs/75-genesis-private-filter-resource.md).

Genesis: [vida observada de los tres efectos auxiliares](docs/76-genesis-effect-lifetimes.md).

Genesis: [pertenencia de auxiliares y máscaras View1](docs/77-genesis-effect-ownership.md).

Genesis: [retirada de los tres auxiliares antes del Scanner](docs/78-genesis-effect-retirement.md).

Genesis: [filtro de efectos por vista en sUnit](docs/79-genesis-effect-views.md).

Genesis: [contexto nativo de dibujo](docs/80-genesis-effect-draw-scope.md) y [gestores compartidos de activación](docs/81-genesis-activation-globals.md).

Genesis: [vida del arma y admisión de la retención](docs/82-genesis-weapon-lifetime.md).

Genesis: [barreras en los usos intermedios del arma](docs/83-genesis-weapon-use-barriers.md).

Genesis: [encendido privado de efectos sin escrituras globales de J2](docs/84-genesis-private-effect-toggle.md).

Genesis: [corrección de color por vista y admisión de dibujo](docs/85-genesis-color-per-view.md).

Genesis: [dibujo Hunter sin reutilizar la caché entre vistas locales](docs/86-genesis-hunter-draw-cache.md).


Corrección gameplay de input: [Cpu(3) -> Pad(1)](docs/88-sub0-cpu3-pad1-runtime-correction.md).

Control J2: [conservar propiedad ante solicitudes posteriores de CPU](docs/90-local-input-mode-ownership.md).

Control J2: los productores de ambos sticks y cruceta ignoraban su argumento de mando. Ocho lecturas corregidas en el instalador; [evidencia y regresiones](docs/91-pad-axis-producers.md). Movimiento jugando aún sin validar.

Control J2: [conservar el mando físico durante teclado/ratón y consultas de estado por slot](docs/92-physical-pad-preservation.md). Cambio de dispositivo y opciones aún pendientes.

Opciones: [25 lecturas del mando conectadas al dueño de Pause; configuración/guardado por jugador pendientes](docs/93-options-menu-input.md).

Opciones: [cinco ajustes del mando del dueño, publicación global conservando los valores de J1](docs/94-options-controller-settings.md). Persistencia independiente de J2 y rutas completas de cancelar/defaults pendientes.

Entrada: [teclado para J1, cambio de dispositivo y restauración del mando de J2](docs/95-keyboard-owner-and-device-switch.md). Un único mando físico, reconexión y validación jugando pendientes.

Guiones: [mando de uPcsInput mediante scheduler y actor/serial](docs/96-script-input-scheduler-owner.md). Sin validación jugando; reparenting, lifetime de schedulers y QTE completos pendientes.

Control J2: [conservar la asociación local durante Invalid0 y reenlace al mismo actor](docs/97-suspended-local-rebind.md). La base de entrega debe conservar esta asociación; reconstruir bases anteriores. Secuencia corregida con pruebas sintéticas, sin validación jugando.
