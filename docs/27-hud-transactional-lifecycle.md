# 27 — Ciclo de vida transaccional del HUD adicional: fuentes y pruebas

25 de septiembre de 2026. Continúa PR #6 y `docs/26-hud-resource-initialization.md`. No se ha generado ni modificado ningún EXE del juego ni ejecutado un constructor de parches. Se conservan los módulos anteriores.

## Implementación nueva

`patches/hud_ownership/lifecycle.hpp/.cpp` añade un controlador real de preparación, publicación, fases y retirada sobre el registro del PR #6. El controlador está implementado; **su backend nativo del motor no está conectado ni se ha mostrado un segundo HUD dentro del juego**. Los tests usan el controlador y registro reales, con las operaciones del motor simuladas.

La preparación recibe los dos gestores nativos y los tres widgets originales como referencias prestadas de P1. Solicita tres objetos nuevos, comprueba la clase antes de inicializarlos, verifica las postcondiciones del recurso y registra cada nuevo objeto para Sub0/vista 1. No escribe en la lista de 21 slots del cockpit ni en los arrays/alias del mini-mapa. No dibuja una preparación incompleta.

La publicación requiere un ticket vigente, sesión válida y una segunda verificación de los árboles. Si falla, queda bloqueada. Una llamada con ticket antiguo se rechaza antes de actualizar la sesión: no puede revocar una nueva generación del HUD. La publicación es atómica respecto al driver monohilo exigido por el contrato, no una transacción multihilo del motor.

La retirada invalida primero los vínculos. Las llamadas reentrantes pueden solicitar stop, pero no ejecutar destructores mientras hay una fase activa. La recolección espera un punto seguro, destruye únicamente los objetos que creó y en orden inverso. Durante todos los destructores mantiene las entradas revocadas; después retira los tokens. La pérdida del actor, el cambio de serial/lifetime/epoch y la notificación previa a destruir el gestor desactivan el conjunto.

## Verificación del árbol GUI

`capture_tree` usa las relaciones comprobadas en el original:

```
GUI+F0 -> recurso
GUI+F4 -> raíz mutable
GUI+F8 -> tabla de nodos
recurso+68 -> cabecera
cabecera+44 -> número de nodos
raíz/nodo+6C -> owner GUI
```

Comprueba clase, campos obligatorios, owner y duplicados. Relee los anclajes antes de entregar el snapshot; un fallo no publica una captura parcial. Compara las direcciones de raíz/tabla/nodos entre instancias, permitiendo compartir el recurso de plantilla. El límite de 256 nodos es una decisión conservadora de este componente, **no un límite descubierto del motor**.

Estas comprobaciones no sustituyen el bloqueo de ciclo de vida. No demuestran intervalos de asignación completos ni detectan por sí solas reutilización ABA de direcciones. El backend debe garantizar que cada objeto creado es nuevo, exclusivamente poseído y no inscrito en listas nativas; el driver debe impedir liberaciones concurrentes.

Si un objeto inesperado comparte estructuras, cambia de clase o queda con recursos parciales cuya propiedad no se puede demostrar, se pone en cuarentena. No se llama a ciegas a su destructor, porque podría liberar datos de P1. Ese caso puede retener memoria y requiere diagnóstico del adaptador; no se presenta como recuperación automática ni como garantía de ausencia de fugas.

## Fases y máscara de vista

El dispatch acepta fases 8, 9 y 11, conservando su contexto opaco. No ejecuta dos veces la misma fase del frame. El draw adicional se admite solo para vista 1. La máscara mDrawView se aplica de forma temporal y se restauran solo sus diez bits, conservando otros indicadores que haya cambiado la llamada.

Esto no implementa las transformaciones ni el clipping. El backend de fase todavía debe reproducir las comprobaciones nativas de visibilidad, orden, estado de interfaz y preparación del render. Los originales P1 siguen bajo el gestor stock: este controlador no les aplica todavía su máscara para media pantalla.

## Rectificación cerrada de la pista 0x69

El punto todavía sin nombre al final de docs/26 queda identificado. `0x02B4019A` pasa 0x69 a `0x01C3B515 -> 0x02CD29E0`. Esa rutina separa los veinte bits inferiores y los siete bits siguientes. Sus consumidores escriben:

- `GUI+14C`, con el diagnóstico literal `Argument [prio] is max over.`;
- `GUI+150`, con `Argument [layer] is max over.`.

Por tanto la constante configura prioridad/capa, no un ID de jugador. No se cambia para inventar una identidad P2. Esto descarta esa colisión concreta; no prueba que todos los registros globales, animaciones y callbacks GUI permitan duplicación sin adaptación.

## Pruebas ejecutadas

Sobre los fuentes del commit `5bdc9526b80cd59924f6db073b176b0de2754c0b`:

- 34 escenarios y 828 aserciones del controlador/registro, en ejecución normal y con AddressSanitizer/UndefinedBehaviorSanitizer. No se suman ambas ejecuciones como escenarios distintos.
- 52 comprobaciones estáticas del original de enero mediante `scripts/audit_hud_resources.py`. SHA-256 antes/después: `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.
- Seis tests Python locales, sin omisiones: entrada exacta, rechazo de otra imagen, original intacto y protección de destinos. En CI la prueba que requiere el original privado se omite explícitamente.
- Sintaxis freestanding i386 comprobada sin enlazar una imagen del juego.
- GitHub Actions run `36180203408`: jobs `lifecycle (native)` y `lifecycle (sanitized)` completados con éxito. Las funciones nativas del motor no se ejecutan.

Los casos incluyen fallo de cada construcción/inicialización, salida anticipada, retorno indebido del puntero de P1, clase incorrecta, llamadas antiguas, stop reentrante, fallo de escritura/restauración de flags y pérdida/cambio del actor. Se añadieron regresiones durante el desarrollo para un ticket antiguo que invalidaba una sesión nueva y para la retirada segura después de una inicialización interrumpida.

Los seis blobs de código/tests/workflow publicados coinciden con los archivos locales ejecutados; identidades en `research/reports/hud-lifecycle-validation.json`.

## Contrato todavía pendiente con el motor

Este cambio no instala un hook ni implementa un lector de punteros vivos por arte de magia. Faltan:

1. Backend nativo de creación realmente desvinculada de los registros globales y callbacks, más su verificación tras cada constructor.
2. Driver de Session/lifetime/epoch y notificación de destrucción con un punto seguro real del motor, incluyendo trabajo de render en vuelo. Las escrituras del backend deben ser atómicas de un DWORD o fallar sin modificarlo.
3. Conexión de los bridges existentes, orden de fases, visibilidad, transformaciones/clipping y enmascarado/restauración de P1.

Solo debe existir un controlador propietario de esos tokens de gestores en cada Registry. Si otro controlador ya los abrió, se rechaza el alta; no roba sus entradas. Los métodos se usan en el hilo del juego; no hay sincronización para llamadas concurrentes ni callbacks después de liberar la instancia del propio controlador.

El siguiente análisis concreto es seguir los consumidores de prioridad/capa y la inscripción de la unidad GUI en grupos desde `uBioGUI/uGUI` y los gestores, para implementar el backend sin doble scheduling. Después conectar el driver con el binder y los puntos de creación/destrucción ya identificados. Pausa/inventario, scheduler, muerte/checkpoints/cutscenes, escenas sin compañero y gameplay siguen abiertos.
