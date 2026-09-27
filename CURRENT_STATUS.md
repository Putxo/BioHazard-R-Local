# Estado actual del cooperativo local

27/09/2026. Rama técnica: `research/script-member-serial-validation`.
PR25 integrado: `6a7144f513f93b9128af7606c45a016973cf833d`, 19 workflows PASS.

**El cooperativo completo sigue en desarrollo. El juego no se ha ejecutado.**
El propietario prohíbe abrirlo y hará personalmente las pruebas jugando.

## Implementación disponible

| Área | Alcance real |
|---|---|
| Input | Sub0 exacto usa Pad1; conversión Cpu -> Pad; Network preservado; acciones específicas aún en auditoría |
| Cámaras | View0 TOP y View1 BOTTOM, targets separados; cutscenes pendientes |
| Inventario | Pack gameplay propio; menú global muestra Sub0 con owner J2 |
| Pausa | Una pausa global; abrir/navegar/confirmar/cancelar por owner; lifetime y nested 6/7 implementados |
| HUD | Clones de Reticle, MainEquipment, MapHerb y SubEquipment; faltan otros widgets e iconos |
| Recursos HUD | Grafo completo conectado, selección desde sIDCockpit, lifetimes y retirada |
| Pickup/puertas | Correcciones existentes conservadas; arbitraje y prompts no están completos |
| Guiones | Dos callbacks PcsSub con owner+serial; cobertura QTE parcial |
| Rescate | Routing parcial existente; muerte/checkpoints pendientes |

La compilación acumulativa nueva conecta 36 hooks HUD/menú sobre la base de
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

Completar HUD (HP, subarma/granadas, interacción y Genesis), auditoría de acciones
del mando 2, scripts/QTE, muerte/revive/checkpoints, cutscenes y escenas sin partner.
La compatibilidad trabajada es January 30 2013; no atribuirla a retail/Feb/May.
Seguir publicando avances verificables; nunca subir EXE, assets o símbolos del juego.

Detalle: [runtime](docs/50-runtime-composition.md), [instalador](docs/51-runtime-installer.md).


Panel de subarmas y rollback de máscaras: [detalle](docs/52-subweapon-hud.md).
