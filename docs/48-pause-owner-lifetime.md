# 48 — PauseHD: entrada física y lifetime del owner

27 de septiembre de 2026. Continuación de `23109d5` en
`research/pause-owner-lifetime-audit`. Se incorporaron los avances remotos
de `research/pause-owner-pad1` detectados durante el análisis; consultar
también `docs/47-pause-owner-pad1-input.md`.

## Evidencia nueva reproducible

`scripts/audit_pause_owner.py ORIGINAL` verifica SHA256 y 19 comprobaciones
de bytes/calls sobre el original de enero suministrado por el propietario.

| Dirección | Consumo | Bridge fuente |
|---|---|---|
| 0x02C3183B | Lista PauseHD, +1A0: confirmar/cancelar | pause_1a0 |
| 0x02C31848 | Lista PauseHD, +1AC: repetición arriba/abajo | pause_1ac |
| 0x02C31FDC | Confirmación PauseHD, +1A0 | pause_1a0 |
| 0x02C31FE9 | Confirmación PauseHD, +1AC | pause_1ac |

`0x01BD9F18 -> 0x01CB3190` carga mStartPadNo; el getter
`0x01BE9080 -> 0x01CB31F0` lee `sPad + member*0x2F8 + 0x1AC`.
El contrato del wrapper es ECX=sPad y `ret 4`. Las máscaras de navegación
de la lista son `0x10010` y `0x40040`. Se mantienen los helpers globales
de ratón/coordenadas y los índices de sGameFlags.

## Cierre y estados secundarios

`0x02CDD415` es el commit común de la máquina de estados: carga this y
estado desde EBP, luego escribe `this+0x24`. Continuación `0x02CDD41E`.
Observar este punto cubre las transiciones que pasan por esa función,
incluidos callers alternativos; no demuestra que no existan escrituras
directas por fuera de ella.

State6 consulta si el estado previo era 8; state8 consulta si era 6.
Hay una relación real 8/6. No se debe interpretar 6 como una segunda
pausa ni afirmar que se ha resuelto su navegación. State7 también mantiene
pausa global y modifica `+0x4C`. La implementación inicial debe descartar
ownership al abandonar su superficie, incluidos 6/7, hasta auditar sus
retornos. Esto es conservador pero deja pendiente conservar J2 en nested menus.

## Sincronización de la rama heredada

El snapshot inicial `9373b73` tenía callbacks ABI ausentes, valores esperados
desincronizados y un warning fatal de GCC. Los commits remotos hasta
`23109d5` ya corrigieron esos tres problemas y añadieron la auditoría
de entrada. Se preservan esas correcciones; no son cambios nuevos de este bloque.

## Límite de este hallazgo

Análisis estático sobre el EXE original; no se ejecutó el juego ni se
instalaron hooks. Estos cuatro sitios cubren lista y confirmación de
PauseHD; no prueban cobertura completa de las pantallas de opciones.
La instalación conjunta del cooperativo y la campaña siguen pendientes.

## Implementación añadida

- `MenuOwnerRouter` captura puntero y serial exactos de Sub0 al abrir.
  Cada lectura valida ambos, ThinkMode Pad y modo local activo; al fallar
  borra owner/surface de forma permanente hasta otra apertura.
- Owner0 local usa Pad0 aunque mStartPadNo apunte a otro pad. Fuera del
  scope local, el fallback conserva el selector original del juego.
- Fallar una lectura del pad propietario también revoca ownership.
- Se rechazan direcciones cuyo cálculo de campos desbordaría PE32.
- `rev_menu_gate_state_commit` reproduce los nueve bytes originales,
  notifica el estado confirmado y vuelve a `0x02CDD41E`. Conserva registros
  enteros y flags. Es un JMP inline, no un callsite `ret 4`.
- `observe_state` limpia al salir de 5/8, en cambios de cockpit y ante
  Sub0 inválido. Un caller alternativo sin opener no crea owner=J2.
- `rev_menu_reset_session()` permite limpiar también reutilización de las
  mismas direcciones/seriales; **falta conectar este callback al teardown
  real del runtime**. Sin esa conexión no se declara resuelto el cambio
  de sesión con direcciones idénticas.

Validación local: componente Win32 MSVC `/W4 /WX`, **42 escenarios y 161
aserciones PASS**; 3 tests Python PASS, incluido original privado; auditor
exacto **19 comprobaciones PASS**. El test ABI i386 ahora comprueba ocho
gateways, balance de pila, flags, registros y que el store precede al
callback. Su ejecución Linux y sanitizers se comprueban en CI por separado.

Pendiente inmediato: continuidad de owner en 8/6 y state7, opciones,
teardown real y escrituras de estado fuera de la función auditada. Mantener
explícitamente `gameplay_executed=false` hasta probarlo dentro del juego.
