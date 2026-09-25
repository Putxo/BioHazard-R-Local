# 29 — Adaptador de operaciones nativas del HUD de enero

25 de septiembre de 2026. Continúa PR #7 y docs/28 desde `b50210223326eceef637a817fd6ddd129c59d6d8`. Entrega solo fuentes, tests y evidencia. No se genera, modifica ni ejecuta un EXE del juego.

## Cambio implementado

`patches/hud_ownership/january_backend.hpp/.cpp` implementa las operaciones concretas de la build de enero para el controlador Lifecycle ya existente: asignación alineada, construcción, inicialización virtual, consulta de pertenencia a sUnit, fases y destructor escalar. `january_calls.cpp/.S` aporta la tabla real de llamadas y sus puentes i386. En una compilación no i386 la tabla nativa queda vacía, no realiza casts de direcciones de 32 bits a llamadas de 64 bits.

Los callbacks de `JanuaryBackend::callbacks()` se conectan directamente a `Lifecycle`. No es otro controlador independiente ni una reimplementación del registro anterior. Los tres widgets son Reticle, MainEquipWin y MapBaseAndHerb; las direcciones, tamaños y slots permitidos se fijan en `JanuaryType` y se contrastan con las vtables antes de operar.

El adaptador NO está activado dentro del proceso del juego. Faltan los hooks de driver que establezcan vida de punteros, permisos de hilo/fase/render y las transformaciones de la interfaz. Sin esos permisos devuelve fallo. Tener implementadas las llamadas nativas no demuestra que el segundo HUD ya se cree o se dibuje durante una partida.

## Separación de propiedad y scheduling

La asignación usa la función específica de cada clase con su tamaño comprobado y alineación 16, seguida de su constructor sin argumentos. Nunca copia memoria del objeto de P1. Antes de inicializar se comprueban clase, marcadores locales de desvinculación y pertenencia real a sUnit.

Una unidad con enlaces next/prev nulos puede estar registrada como único elemento de una lista. Por eso se llama también al comprobador `0x0326AA90`. No se borran los enlaces ni se cambia el moveline para fingir que el objeto está desvinculado. En cockpit se comprueba además su enlace +290; no se aplica ese layout al mapa/hierbas.

La configuración recibe los dos gestores y tres originales prestados. El adaptador rechaza direcciones que coincidan o se solapen con sus bloques conocidos y exige al driver una certificación del intervalo completo devuelto por el asignador. Esa certificación no está implementada como un `true` constante: es una precondición todavía pendiente del driver real.

Los objetos con construcción incierta, pertenencia inesperada o alias de recursos se retienen en cuarentena. El adaptador no ejecuta un destructor sobre un bloque crudo o una dirección prestada. Puede retener memoria; no se presenta como garantía de recuperación o ausencia de fugas.

## ABI de llamadas y fases

Los puentes distinguen cuatro firmas: cdecl sin argumentos, cdecl con tamaño/alineación, thiscall sin argumentos y thiscall con un argumento. Conservan los registros no volátiles, restablecen la pila y pasan el objeto por ECX.

Los slots 8 y 9 no reciben el contexto de dibujo como argumento de stack. Solo el slot 11 lo recibe. El inicializador y las fases son void: EAX se ignora y el resultado del adaptador indica admisión/postcondiciones, no un supuesto resultado booleano del motor.

El slot 8 propaga el valor temporal +1C del gestor a la unidad. El adaptador rechaza valores negativos, NaN o infinitos como política conservadora propia, no como una regla atribuida al motor.

Cockpit aplica los gates Active y ForceSkip de su tipo. MiniMap conserva su ruta distinta: no interpreta +294 como ForceSkip, pues allí es mHerbHaveNum. El dibujo adicional exige vista 1 del contexto y mDrawView. El Lifecycle mantiene el scope de máscara, evita duplicar fases del mismo frame y restaura solo sus diez bits.

Después de una fase se vuelven a comprobar permisos y pertenencia. Si la llamada inscribe la unidad o invalida el contexto, no se continúa ejecutándola como si siguiera desvinculada. Esto no sustituye las ramas de visibilidad global del manager ni prepara por sí solo proyección, scissor o clipping.

## Fallos ahora propagados al controlador

Se añadieron tres callbacks opcionales checked al final de Backend. Conservan compatibilidad de fuente con los callbacks void históricos y tienen prioridad cuando existen.

Antes, una implementación void podía rechazar internamente una destrucción y el controlador no tenía cómo distinguirla de una realizada. Ahora un rechazo deja el objeto registrado y en cuarentena; no se pone su puntero a cero ni se retiran sus tokens como si se hubiera liberado. Un rechazo de fase o inicialización impide continuar/publicar y solicita retirada segura.

Las pruebas conservan la ruta de callbacks antigua y comprueban las nuevas. Una destrucción correcta llama una vez al destructor escalar con flag 1; no vuelve a liberar el mismo bloque ni lee memoria del objeto después de esa llamada.

## Pruebas ejecutadas

Código publicado en `6e9f326ae26d07d25c076eb104bbc36b310de7de`; los trece blobs de producción, auditor, tests y workflow coinciden con las copias locales probadas.

- Backend + Lifecycle + Registry reales: 43 escenarios y 2.516 aserciones, en ejecución normal y con AddressSanitizer/UndefinedBehaviorSanitizer. Las operaciones del motor son simuladas; ambas ejecuciones no se suman como escenarios distintos.
- Regresión del controlador anterior: 34 escenarios / 828 aserciones, correcta en CI normal y sanitizada.
- Auditor de solo lectura: 69 witnesses de bytes, destinos, slots y ABI sobre el original local exacto. Seis tests Python locales sin omisiones. En CI solo se omite explícitamente el test que necesita la imagen privada.
- GitHub Actions run `36183979550`: tres jobs correctos, backend native/sanitized y native-call-abi.
- Job `108232822485`: ejecutó la tabla de invocación y los puentes i386 de producción en 30 llamadas a funciones sintéticas. Comprobó argumentos, retorno, alineación, registros no volátiles y pila. No ejecutó funciones de Resident Evil Revelations.
- El host local no permite ejecutar ELF32 y devolvió Exec format error; esa prueba se ejecutó realmente en el runner de CI, no se atribuye al contenedor local.
- SHA del original antes/después: `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.

Informe e identidades: `research/reports/hud-january-native-validation.json`. La suite heredada Local routing source tests también pasó el run `36183979372` del mismo commit.

## Contrato y siguiente integración concreta

Conexión de fuente prevista, no un inicializador automático:

```cpp
JanuaryBackend backend(host, january_native_calls());
// host debe proceder del driver validado, nunca de permisos constantes.
if (!backend.configure(parents, original_widgets)) return;
Lifecycle hud(registry(), backend.callbacks());
```

El host debe validar la identidad del PE cargado y las direcciones, vida de las asignaciones, lectura/escritura serializada y permiso de ejecución actual. Structural requiere hilo del juego, ausencia de callbacks activos y trabajo de render drenado. Phase requiere la rama nativa correcta del gestor, los bridges de actor instalados y el contexto de vista/transformación preparado. Los checks del adaptador no capturan ni reparan excepciones del motor.

El siguiente trabajo es el driver de admisión y scheduling en torno a las fases/creación/destrucción ya localizadas de uCockpitManagerMain y uMiniMapManager: producir Session/lifetime/epoch fiables, excluir callbacks/render en vuelo y conectar el binder sin doble scheduling. Continúan pendientes la instalación selectiva de bridges, máscara P1, transformación/clipping y la revisión de registros/callbacks transitivos. Pausa/inventario, scheduler compartido, muerte/checkpoints/cutscenes, escenas sin partner y gameplay no quedan cerrados por este módulo.
