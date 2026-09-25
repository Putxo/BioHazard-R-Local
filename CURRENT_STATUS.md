# Estado actual — adaptador nativo del HUD de enero, solo fuentes

25 de septiembre de 2026. Se continúa desde PR #7 sin generar ni modificar un EXE del juego. Fuentes, pruebas y evidencia se guardan en GitHub; el cooperativo completo no está terminado ni probado en campaña.

## Avance de esta continuación

Se ha implementado `patches/hud_ownership/january_backend.hpp/.cpp` y la tabla/puentes i386 `january_calls.cpp/.S`. Conectan las operaciones específicas de enero al controlador Lifecycle del PR #7: asignadores y constructores por clase, inicialización void con postcondiciones, comprobación real de pertenencia a sUnit, firmas correctas de las fases y destructor escalar.

La construcción no añade objetos a las listas nativas. Se comprueban tanto moveline/enlaces como pertenencia real: un único elemento registrado puede tener enlaces nulos. Se preservan P1, los layouts separados de Cockpit/MiniMap y todos los parches anteriores. Las direcciones y slots son explícitos y se verifican antes de llamar.

Los slots 8/9 reciben cero argumentos; el slot 11 recibe contexto. Se mantienen gates Active/ForceSkip según la clase, filtro de vista 1 y propagación del valor temporal del gestor. No se trata el contador de hierbas como ForceSkip. El driver deberá proporcionar la rama correcta de visibilidad y la transformación/clipping.

Se añadieron callbacks checked a Lifecycle. Un rechazo de destrucción conserva el objeto y sus tokens en cuarentena, en vez de contabilizarlo como liberado; un fallo de inicialización/fase bloquea la publicación/continuación. La ruta void anterior se conserva y pasa su suite de regresión.

## Integración que aún no está activada

El adaptador tiene llamadas nativas concretas, pero NO está instalado en el proceso del juego. Necesita un driver real que compruebe identidad del PE cargado, vida y exclusividad de asignaciones, hilo, fase, Session/lifetime/epoch y exclusión de trabajo de render. Los permisos no se suplen por constantes true. El componente no convierte un puntero desconocido en seguro ni captura errores internos del motor.

Siguen pendientes la instalación selectiva de bridges de actor, driver/safe points, inscripción transitiva de callbacks, máscara P1 y transformación/clipping de las dos mitades. No se ha creado o dibujado un segundo HUD durante una partida. La cuarentena puede retener memoria y requiere diagnóstico, no una liberación a ciegas.

## Pruebas verificadas

Código `6e9f326ae26d07d25c076eb104bbc36b310de7de`: 43 escenarios / 2.516 aserciones de backend/controlador/registro con funciones de motor simuladas, normal y ASan/UBSan. La regresión anterior conserva 34 escenarios / 828 aserciones.

69 comprobaciones del original exacto y seis tests Python locales sin omisiones; original intacto con SHA `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`. En CI el único test privado se omite explícitamente.

Run `36183979550`: tres jobs correctos. El job `108232822485` ejecutó 30 llamadas de los puentes/tabla i386 reales contra callees sintéticos. El host local no ejecuta ELF32; la ejecución se hizo en el runner. No se ejecutó código del motor ni gameplay. Detalles en `research/reports/hud-january-native-validation.json`.

## Punto exacto de continuación

Implementar el driver de admisión/scheduling alrededor de las fases, creación y destrucción comprobadas de uCockpitManagerMain/uMiniMapManager. Derivar snapshots de Session/Actor del binder con lifetime/epoch, impedir doble scheduling y llamadas mientras haya render/callbacks en vuelo. Después conectar scopes de render y máscara P1 sin sustituir Self globalmente.

Documentación actual: [docs/28-hud-native-registration-and-abi.md](docs/28-hud-native-registration-and-abi.md), [docs/29-january-hud-native-adapter.md](docs/29-january-hud-native-adapter.md) y `research/hud_native_ops_state.json`.

El estado anterior permanece íntegro en [docs/history/CURRENT_STATUS-before-january-native-ops.md](docs/history/CURRENT_STATUS-before-january-native-ops.md). Los módulos de ownership y sus archivos históricos no se rehacen ni se presentan como perdidos. Las recetas de EXE anteriores no se ejecutan.

Pausa/inventario por jugador, scheduler compartido, muerte/checkpoints/cutscenes, escenas sin compañero y validación conjunta siguen abiertos. Este estado distingue implementación del adaptador, pruebas de componentes y activación real en el juego.
