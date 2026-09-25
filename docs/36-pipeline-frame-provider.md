# 36 — Reloj del ciclo CPU y proveedor de fotograma conectado al HUD

Continúa el PR #10, integrado en `7bd05d2018739dfa47641b21b793df8b75d81662`. Entrega exclusivamente fuentes, pruebas y evidencia. No se ha generado/modificado un EXE del juego ni instalado hooks.

## Reloj elegido y significado exacto

La evidencia de docs/35 identifica el ciclo exterior de `sSkeletonMain` en `0x02F50680`. Los puntos `0x02F506A0` y `0x02F50F4B` delimitan su recorrido normal de actualización/dibujo. `PipelineClock` genera un número por entrada de ese ciclo, no por widget, gestor ni vista. No utiliza el contador de simulación de sUnit que se detiene con la pausa.

El número identifica una invocación CPU. **No representa Present exitosos y no demuestra que el render o la GPU estén drenados.** Si el motor llama a un gestor desde otro hilo o fuera del intervalo observado, la captura se rechaza; no se le asigna el último número por conveniencia. La correlación de posibles trabajos asíncronos aún necesita validación en el motor.

El reloj exige el hilo armado y empareja el objeto receptor y EBP de la invocación. Un BEGIN anidado, END sin BEGIN o pareja distinta bloquea el reloj; no reutiliza una secuencia antigua. La secuencia y su revisión no desbordan: el agotamiento produce fallo sin envolver el contador. El componente no lee memoria del juego ni otorga permisos de construcción o destrucción.

## Proveedor conectado, no otro contrato vacío

`PipelineFrameProvider::callbacks()` implementa el callback `ManagerHost::sample` usando el `LifetimeSource` ya integrado. La cadena de fuentes es:

```
PipelineClock + LifetimeSource
  -> PipelineFrameProvider
  -> ManagerDriver
  -> Lifecycle::dispatch_manager
  -> JanuaryBackend
```

Antes de capturar vida guarda el ciclo vigente; después vuelve a comprobar número, revisión, receptor y marco de la misma invocación. Entrega conjuntamente el número y la Session con generaciones de actores/gestores. Si hubo cierre/reapertura o una notificación que invalida la captura, el output del caller permanece intacto. No fabrica nacimientos a partir de punteros ni incrementa un frame por cada callback GUI.

Este protocolo verifica la captura; no transforma la memoria en un bloqueo multihilo. `LifetimeSource::capture` mantiene su efecto de actualizar/revocar Registry. El driver detiene el conjunto al fallar el sample; no se afirma una transacción que revierta todos los efectos internos de otros componentes.

El proveedor reenvía lector e identidad de hilo a su contexto correcto. Conserva como servicios obligatorios separados la preparación y restauración real del dibujo. Si no existen, el despacho no se autoriza. No hay implementación equivalente a `enter_scope = true` en producción.

## Restauración de un contexto abierto durante una captura que caduca

La entrada exige el mismo ManagerFrame que entregó la última captura y el mismo intervalo aún abierto. Una entrada exitosa consume esa captura. Si el servicio abre un scope y dentro de esa llamada se cierra o cambia el ciclo, el proveedor restaura el scope antes de devolver false. El driver no ejecuta ninguna fase de widgets en ese caso.

Se encontró y corrigió un punto de propagación: si esa restauración falla, el proveedor queda en fallo persistente. El nuevo callback opcional `ManagerHost::scope_failed` informa al driver, que solicita stop **en ese mismo evento** en lugar de esperar a la siguiente captura. No llama a destructores dentro de la fase ni vuelve a ejecutar una restauración ya intentada. La ruta histórica de hosts sin ese callback se conserva mediante valor nulo por defecto.

Los callbacks deben respetar el contrato de entrada fallida sin cambios y permanecer válidos durante la vida del driver. Estas protecciones no sustituyen la implementación pendiente del scope nativo ni capturan excepciones internas del motor.

## Dos gateways, instrucciones originales preservadas

`pipeline_gateways.S` conserva registros, flags, pila y estado x87/MMX/XMM/MXCSR. BEGIN toma el receptor del ECX guardado: el local `[EBP-8]` todavía no estaba inicializado en ese punto. Después reproduce el MOV original que lo escribe. END usa el local ya inicializado y reproduce los tres POP y el ADD a ESP; conserva los flags producidos por ese ADD, no unos flags anteriores incorrectos.

Los dos gateways pasan EBP como identificador de la invocación. El retorno del observador no cambia la ejecución del código original. Son fuentes sin instalar; `pipeline_continuations.ld` solo define destinos y no construye una imagen del juego. Requieren x86 con FXSR/SSE, pila suficiente y sink de vida de proceso; no admiten hot-unload ni estado AVX arbitrario.

## Pruebas efectivamente ejecutadas

Código actual: `77f28384b5b64e06cbeed0c9b15214dcaf37537a` (incluye la corrección de restauración). GitHub Actions run `36196102073` terminó con tres jobs correctos.

| Comprobación | Ejecución comprobada | Resultado |
|---|---|---|
| PipelineClock, secuencias, intervalos y agotamiento | Contenedor local y CI, normal y ASan/UBSan | 22 escenarios / 1.231 aserciones |
| Cadena completa de fuentes reloj/vida/proveedor/gestor/controlador/backend | CI, memoria, llamadas del motor y scopes simulados | 20 escenarios / 1.266 aserciones |
| Gateways de producción | CI i386, receptor y continuaciones sintéticos | 2 gateways / 3 invocaciones |
| Auditor de original y mapa de fuentes | Contenedor local | 8 tests, 0 omisiones |
| Misma suite sin original privado | CI | 7 tests pasados, 1 omitido explícitamente |
| Ventanas/RTTI/calls y hash del cuerpo exterior | Original local en solo lectura | 31 comprobaciones |

La prueba i386 inicial está en el job `108271391313`, run `36195853881`, y se repitió correctamente en el job `108272171238`, run actual. Verifica BEGIN con local envenenado, END con ECX distinto, callback que devuelve false, argumentos, alineación, DF, flags, registros y campos definidos de FP. El contenedor local rechaza ELF32; no se le atribuye esa ejecución.

El log del job `108272171459` muestra 22/1.231 y 20/1.266 con ASan/UBSan. El run previo `36195853881` tenía 1.265 aserciones del proveedor; la adicional del actual comprueba la retirada inmediata tras la restauración fallida. No se suman esos resultados como escenarios de gameplay. La regresión Local routing del mismo head también pasó el run `36196102080`.

El cuerpo exterior tiene 2.271 bytes y SHA-256 `a2c8f20788ce3a48cb01f468bb92241fca65eb82e723b26131266e670f88d894`. Original intacto antes/después: `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`, 60.748.800 bytes. El auditor y su mapa son reproducibles desde `scripts/audit_pipeline_frame.py`; no prueban rutas de excepción ni ejecución asíncrona real.

## Continuación exacta

Ahora existe una implementación concreta de sample que conecta vida y ciclo al ManagerDriver. Sigue pendiente instalar las entradas verificadas antes del mundo y suministrar permisos de memoria/asignaciones/hilo, exclusión real de render, scope nativo, máscara P1 y transformación/clipping de ambas vistas. Tener abierto el ciclo CPU no concede permiso Structural o Destroy.

El análisis posterior de dibujo en docs/37 identifica que los tres widgets convergen en una ruta GUI que ya lee datos del contexto. Seguir sus dimensiones, transformación y consumidores antes de añadir un segundo escalado. Pausa/inventario, comandos compartidos, muerte/checkpoints/cutscenes, escenas sin compañero y pruebas conjuntas conservan sus pendientes. No se ha creado ni dibujado el HUD P2 dentro del juego con esta integración de fuentes.
