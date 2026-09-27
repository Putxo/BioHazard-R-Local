# Estado actual del cooperativo local

27/09/2026. Rama técnica: `research/script-member-serial-validation`.
PR48 integrado: `37b2895b745c2f5d605ba205dd4416581eb6215c`, 20 workflows PASS.

**El cooperativo completo sigue en desarrollo. El juego no se ha ejecutado.**
El propietario prohíbe abrirlo y hará personalmente las pruebas jugando.

## Implementación disponible

| Área | Alcance real |
|---|---|
| Input | Sub0 exacto usa Pad1; conversión Cpu -> Pad; Network preservado; acciones específicas aún en auditoría |
| Cámaras | View0 TOP y View1 BOTTOM, targets separados; cutscenes pendientes |
| Inventario | Pack gameplay propio; menú global muestra Sub0 con owner J2 |
| Pausa | Una pausa global; abrir/navegar/confirmar/cancelar por owner; lifetime y nested 6/7 implementados |
| HUD | Clones de Reticle, MainEquipment, MapHerb, SubEquipment, Damage, Heal, Scope y Scanner (detector conectado, activación pendiente); faltan asfixia, Genesis y otros widgets |
| Recursos HUD | Grafo completo conectado, selección desde sIDCockpit, lifetimes y retirada |
| Pickup/puertas | ActionIcon2D por miembro/vista; prioridad y parada del bucle por miembro; otros gates y prompts completos pendientes |
| Guiones | Dos callbacks PcsSub con owner+serial; cobertura QTE parcial |
| Rescate | Routing parcial existente; muerte/checkpoints pendientes |

La compilación acumulativa nueva conecta 131 hooks HUD/menú/ActionIcon sobre la base de
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
