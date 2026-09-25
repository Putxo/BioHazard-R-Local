# 31 — Despacho por gestor y seis entradas nativas de fase

25 de septiembre de 2026. Continúa PR #8 desde f420a90f903af9d23069f18dc8978588d063bf34. Solo fuentes, pruebas y evidencia. No se genera/modifica un EXE del juego ni se instalan hooks en un proceso.

## Defecto corregido en el controlador previo

Lifecycle::dispatch agrupaba los tres widgets y consumía un solo bit por fase del fotograma. Si se conectaba directamente a los dos managers, el primer gestor ejecutaba también el widget del otro y la segunda llamada quedaba descartada como duplicada. No era un fallo probado jugando: era un defecto comprobable de la conexión del componente.

La nueva dispatch_manager recibe familia y dirección del gestor. Comprueba el ticket vigente y su clase, ejecuta únicamente sus widgets y mantiene tres bits de fase por gestor. Cockpit procesa Reticle/MainEquipment; MiniMap procesa MapHerb. Ambos pueden completar 8/9/11 en el mismo fotograma, con contextos de dibujo distintos. La llamada agrupada histórica se conserva para las pruebas anteriores, no como entrada válida de un hook nativo.

No se cambian la creación de recursos, el registro de ownership ni las operaciones nativas JanuaryBackend. Se mantienen sus comprobaciones y su protocolo de retirada. Los originales P1 no son despachados por este código.

## ManagerDriver implementado

manager_driver.hpp/.cpp conecta eventos de los seis puntos comprobados en docs/30 a Lifecycle::dispatch_manager. El driver se vincula a un ticket Live y a generaciones explícitas de los dos gestores. Cada evento comprueba el hilo propietario, dirección exacta, familia, fase y contexto. Un sitio desconocido, otro gestor, una entrada de fase 8/9 con contexto o una llamada reentrante se rechazan antes de consultar el estado de sesión.

El frame proviene del proveedor externo, no se incrementa al llegar cada gestor: eso habría contado dos fotogramas donde solo hay uno. Un frame anterior no invalida la sesión nueva. Si cambia la dirección o generación del gestor, falla la captura fiable, o el actor deja de ser válido para la sesión, el driver solicita stop; nunca ejecuta destructores desde el evento de fase.

La ruta de dibujo acepta solo el byte de vista 1 obtenido del contexto +158. P1 y las vistas no destinadas a jugadores no disparan este despacho adicional. El indicador de admisión phase_scope solo permanece activo durante el dispatch y identifica exactamente widget, gestor, fase y contexto.

## Seis gateways con estado de máquina preservado

manager_gateways.S contiene fuentes jump-in para las entradas ordinarias de Cockpit y MiniMap en las fases 8/9/11. Los seis puntos, bytes desplazados y continuaciones están contrastados por el auditor; manager_continuations.ld solo define símbolos de continuación, no construye ninguna imagen.

Cada gateway conserva registros generales y EFLAGS, guarda/restaura estado x87/MMX/XMM/MXCSR mediante un área alineada de FXSAVE y limpia DF antes del callback. Después reproduce exactamente las dos instrucciones desplazadas. En MiniMap el ADD original produce sus propios flags; no se fuerzan los anteriores sobre esa instrucción. Solo fase11 lee el contexto [ebp+8]; las otras pasan cero.

Precondiciones de estos fuentes: ejecución x86 de 32 bits con FXSR/SSE, pila suficiente, imágenes y puntos de parche validados, y callbacks que no propaguen excepciones ni se descarguen mientras el hook sea alcanzable. No se reivindica soporte de estado AVX o de módulos de otra build.

La entrada de dibujo MiniMap se coloca después de su condición de omisión, no en el epílogo común 0x02B687F0. Las ramas especiales del cockpit que eluden su recorrido ordinario tampoco pasan por estos puntos. Llegar a uno de ellos acredita únicamente la rama de fase, no un punto seguro para construir/destruir.

## Contrato que no se suplanta por constantes

ManagerHost sigue necesitando implementaciones nativas de:

- thread_id y sample: frame real, Session y vida de los dos gestores fijadas durante el evento; las generaciones deben cambiar antes de destruir/reutilizar direcciones;
- enter_scope/leave_scope: comprobación de los puentes de actor instalados y establecimiento/restauración del estado de dibujo, transformación/clipping y exclusión requerida;
- los permisos completos de JanuaryHost, incluida identidad de imagen y vida/exclusividad de asignaciones.

enter_scope que devuelve false debe dejar el estado sin cambios. Una entrada exitosa siempre se empareja con leave_scope, incluso si se solicita stop durante la entrada o la fase. Si la restauración falla se revoca el despacho. Los eventos duplicados pueden entrar/salir de ese scope, pero no vuelven a ejecutar una fase del widget; por eso el scope debe ser reversible y no usar su mera entrada como un avance de animación.

phase_scope NO sustituye a JanuaryHost::permit: aporta solo la parte de pertenencia a la rama/fase. No concede Structural, Destroy, certificados de asignación ni un fence del render. Los permisos todavía ausentes siguen produciendo rechazo.

El sink es opcional y se enlaza una sola vez, antes de que los hooks sean alcanzables; el driver debe vivir tanto como ellos. No hay hot-swap, descarga, setters concurrentes ni sincronización multihilo implementados. Las notificaciones de vida deben proceder del motor, no de una lectura de puntero que supuestamente detectase ABA.

## Pruebas ejecutadas y límites

Fuentes de código 78dcc277d7cc1b21bcfa28983abf6bc1348922b6:

- 35 escenarios / 1.362 aserciones con Registry, Lifecycle y ManagerDriver reales y operaciones del motor simuladas. Ejecución local normal y ASan/UBSan correctas, repetidas tras publicar el código. No se suman como escenarios distintos.
- Cinco tests Python locales sin omisiones. El test privado verifica 31 witnesses del original de enero; SHA antes/después 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69. En CI ese único test se omite expresamente.
- Run 36187871978: driver native, driver sanitized y gateways correctos. También pasa la regresión histórica de Lifecycle.
- Job 108245604011 ejecutó los seis gateways de producción en i386, con receptor y continuaciones sintéticos. Comprueba argumentos, alineación, DF al entrar al callback, registros, flags, pila y campos definidos del estado de coma flotante. El callback altera deliberadamente ese estado para probar su recuperación.
- El contenedor local no ejecuta ELF32; la prueba nativa anterior se ejecutó en GitHub Actions. No se ejecutaron funciones de Resident Evil Revelations.

Los doce blobs de fuentes, pruebas, auditor y workflow publicados coinciden con las copias locales ejecutadas. Identidades y resultados en research/reports/hud-manager-phase-validation.json.

No se ha instalado ninguno de los seis gateways ni activado un HUD 2P. El resultado es una conexión de fuentes específica para las fases reales y una corrección del despacho; no una prueba de que esos eventos estén recibiéndose durante la campaña.

## Punto exacto siguiente

Completar el proveedor nativo de frame/vida y los scopes. Debe seguir el binding resuelto de uPcsPlayer en 0x02DF4F30, las notificaciones previas a destrucción de actores/gestores y el límite real entre render en vuelo y fase admisible. No tratar cada callback GUI como un frame nuevo ni suponer que el puntero +44 demuestra por sí solo vida segura. Después conectar la instalación selectiva, scopes de P1 y transformación/clipping. Pausa, scheduler, flujo de muerte/checkpoints/cutscenes y escenas sin compañero conservan sus pendientes.
