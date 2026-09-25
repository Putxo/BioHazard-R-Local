# 34 — Continuación del observador de vida: recuperación, corrección y pruebas

Se parte del PR #9 integrado en 9e8dfec08842d8e199a59932074220cc5581f96c. El turno interrumpido dejó ocho archivos en research/hud-lifetime-observer, head d18aa28011118d01e6117eb5fd223218cd1b8b37. Se recuperaron y revisaron, sin rehacer el registro, el controlador ni el adaptador nativo anteriores.

Esta continuación solo entrega fuentes, tests y evidencia. No se genera, modifica o ejecuta otro EXE del juego.

## Qué implementa la fuente

lifetime_source.hpp/.cpp mantiene una tabla acotada de vidas observadas para actores y los dos gestores. Se alimenta de tres finales de constructor, cuatro inicios de destructor de actor/gestor, dos avisos del binder y dos destructores de PCS. El comienzo de un actor se observa en la base uNpc; al admitir el actor en un rol se exige la vtable final exacta uNpc o uPlayer y ThinkMode::Pad. No se amplía esta cobertura a clases desconocidas por semejanza de nombre.

Los tokens llevan generaciones nuevas al observar otro nacimiento. Un binding de la misma instancia conserva su vida. Un binding no demuestra que un puntero haya nacido: sin el aviso previo no se produce un snapshot válido.

La destrucción revoca inmediatamente el registro del HUD correspondiente antes de la captura siguiente y no lee campos del objeto destruido. Los avisos de uPlayer y su destructor base uNpc pueden describir la misma vida: el segundo no vuelve a incrementar el epoch ni provoca otra liberación. Esta fuente no libera recursos.

La captura comprueba gestores seleccionados, actores ya observados, seriales no negativos, modo Pad, tracker Sub0, flag local y resultado del getter Self nativo. Relee anclajes después de ese getter; cualquier evento reentrante que cambie la revisión hace fallar la captura sin modificar el output del caller. Los widgets revocados no recuperan la asociación anterior aunque vuelva el flag local.

El código admite llamadas únicamente en el hilo armado. No implementa sincronización para destrucciones arbitrarias en otros hilos: instalarlo exige demostrar y respetar ese contrato, no convertir un rechazo en prueba de seguridad multihilo. Los límites de 256 vidas y ocho niveles de binding son límites conservadores del componente, no límites atribuidos al juego. Su agotamiento bloquea la fuente.

## Fallo descubierto al contrastar los gateways con el flujo original

La primera versión ubicaba BEGIN en 0x02DF4F99. El branch de PCS+50 en 0x02DF4F6E podía saltarlo, pero aun así llegar al END de 0x02DF504B. Las pruebas aisladas de registros/pila no comprobaban que ambos puntos se recorrieran en pareja.

La corrección 82964fe1f49631cbac60b074305409caaa315573 mueve BEGIN a 0x02DF4F65, antes del test. Reproduce MOV EAX,[EBP-8] y MOVZX ECX,byte [EAX+50], y continúa en 0x02DF4F6C. La tabla de eventos rechaza el punto antiguo. END sigue donde estaba y no se pisan los cambios previos del binder. La evidencia concreta está en docs/33; esa rectificación prevalece sobre la propuesta histórica de docs/32.

El harness actualizado comprueba los once gateways en doce invocaciones. BEGIN se ejecuta con byte cero y con 0xAB, dejando los bytes vecinos no nulos para comprobar la extensión de byte, no una lectura de DWORD. Conserva la comprobación de registros, flags, pila y campos definidos del estado x87/XMM/MXCSR. El receptor y las continuaciones son funciones sintéticas; no se llama al motor.

## Pruebas verificadas del código publicado

Head de código/auditor: fabb38fb5028cec210603ab9aac6410be246581a. Run de GitHub Actions: 36193139147.

| Comprobación | Lugar real de ejecución | Resultado |
|---|---|---|
| LifetimeSource y Registry, memoria y Self simulados | CI, job 108262744234 | PASS |
| Misma suite con ASan/UBSan | CI, job 108262744015 | 34 escenarios, 940 aserciones, PASS |
| Once gateways de producción, doce invocaciones | CI i386, job 108262744183 | PASS, ambos valores del byte BEGIN |
| Mapa de eventos/assembly/continuaciones y rechazo del BEGIN antiguo | CI | PASS |
| 46 instrucciones/RTTI/branches seleccionados del original exacto | Contenedor local, solo lectura | PASS |
| Suite Python con original local | Contenedor local | 8 tests, 6 pasados y 2 omitidos por árbol de fuentes incompleto |
| Misma suite Python en checkout completo sin imagen privada | CI | 8 tests, 7 pasados y 1 omitido por imagen privada ausente |

Las dos pruebas omitidas localmente son las del mapa de fuentes, que sí se ejecutaron en CI. El test de imagen privada omitido en CI sí pasó localmente. No se presentan como ocho pruebas locales completas ni como ejecución del juego.

Se verificó la identidad de las copias locales/publicadas:

- scripts/audit_lifetime_observer.py: blob 14d9182992bebd4e9a6565ab09118e9498bf2dab.
- tests/hud_ownership/test_lifetime_audit.py: blob 1285a327e1ec844d196d108386699aec353caf98.

Original leído: 60.748.800 bytes, SHA-256 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69. El hash se volvió a calcular después de las pruebas. Los sitios se contrastaron también, en solo lectura, con la imagen histórica ya existente 71f5e70d...; no se produjo otra imagen.

El auditor comprueba ventanas seleccionadas y relaciones concretas, no demuestra todo el grafo de control o cobertura de todos los destructores derivados. Sus tests rechazan entradas incorrectas, destinos existentes y alias del original.

## Lo que no debe darse por resuelto

LifeSnapshot contiene Session, actores y generaciones de gestores, pero deliberadamente no inventa un frame. La evidencia de docs/32 muestra que el contador de simulación de sUnit se detiene por pausa; usarlo como reloj único del dibujo podría descartar presentaciones necesarias. Tampoco se incrementa un contador por cada uno de los dos gestores.

Faltan instalación antes de construir el mundo, lectura serializada y cobertura completa de vida para los objetos admitidos, conexión a ManagerHost y un reloj de presentación contrastado. El permiso de una rama GUI no certifica render drenado, propiedad de asignaciones ni scissor/proyección configurados. Los scopes y el fence deben proceder del motor; no existen defaults true.

Siguiente trabajo: completar la fuente de presentación/admisión, conectar snapshots y notificaciones de vida con los componentes existentes y verificar los scopes de dibujo de ambas mitades. Mantener la máscara P1 y la retirada segura, y no duplicar scheduling ni confundir un reenlace con vida nueva. Pausa/inventario, scheduler compartido, muerte/checkpoints/cutscenes, escenas sin compañero y gameplay siguen pendientes. No se ha dibujado un segundo HUD dentro del juego con este código.
