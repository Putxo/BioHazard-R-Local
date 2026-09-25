# Estado actual — ciclo CPU y captura de vida conectados a las fases del HUD

Continuación del PR #10 / `7bd05d2018739dfa47641b21b793df8b75d81662`. Solo fuentes, pruebas y evidencia en GitHub. No se ha generado/modificado otro EXE del juego ni instalado hooks. El cooperativo completo sigue pendiente.

## Implementación añadida

`pipeline_clock.hpp/.cpp` identifica una invocación CPU del ciclo exterior de `sSkeletonMain`, con BEGIN `0x02F506A0` y END `0x02F50F4B`. Empareja receptor/EBP/hilo y mantiene secuencias sin desbordamiento. No incrementa un frame por cada widget o gestor ni utiliza el contador de simulación detenido por pausa. **No es un contador de Present exitosos ni un fence del render.**

`PipelineFrameProvider` conecta ese reloj y el `LifetimeSource` existente a `ManagerHost::sample`. Produce un ManagerFrame coherente con Session y generaciones; comprueba que la invocación siga abierta después de capturar vida. El driver, controlador y adaptador anteriores se utilizan directamente, no se sustituyen por otra implementación.

Hay dos gateways de ciclo CPU con conservación de registros/flags/pila/estado FP y replay de sus instrucciones. BEGIN usa ECX original porque su local aún no está inicializado. END conserva el efecto real del ADD a ESP. Sus fuentes no están instaladas.

Se corrigió la propagación de un fallo de restauración: si una entrada abre el scope y el ciclo cambia, lo restaura antes de rechazar el despacho. Si esa restauración falla, el callback opcional scope_failed solicita stop inmediato del ManagerDriver. No espera al siguiente fotograma ni destruye dentro de la fase.

## Pruebas comprobadas

Código `77f28384b5b64e06cbeed0c9b15214dcaf37537a`, run `36196102073`: tres jobs correctos. Reloj 22 escenarios / 1.231 aserciones, local y CI normal/ASan/UBSan. Cadena de siete componentes del proveedor: 20 escenarios / 1.266 aserciones en CI normal y sanitizada, con memoria, motor y scopes simulados. Dos gateways en tres invocaciones i386 en CI; no se atribuye ejecución ELF32 al contenedor local, que no la admite.

Auditor: ocho tests locales sin omisiones, siete más una omisión explícita del original privado en CI. Treinta y una comprobaciones de ventanas/RTTI/calls y cuerpo exterior de 2.271 bytes. Original intacto: SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`. La regresión Local routing pasó el run `36196102080`.

Informes y detalle: `research/reports/hud-pipeline-frame-validation.json`, `research/hud_pipeline_frame_state.json` y docs/35–36. No se contabilizan tests aislados como ejecución de campaña.

## Punto exacto de continuación

La lectura del código de dibujo identificó que Reticle, MainEquipWin y MapBaseAndHerb convergen por `0x01C19FFA -> 0x02CD28E0 -> 0x035DD4E0`. Esta ruta consulta datos de `context+0xBC` mediante `0x02B02F00` y actualiza estructuras propias de la GUI. El mapa de veinte ventanas está en `research/reports/gui-common-draw-entry-map.json` y el recorrido en docs/37.

Seguir los productores de esas dimensiones y sus consumidores `0x035E1280/0x035E1790` antes de aplicar escalado adicional. Completar scope nativo, proyección/viewport/scissor y máscara de P1, sin sustituir globalmente Self ni duplicar una transformación que ya pueda realizar el motor.

## Lo que todavía no se ha activado

No están instalados los gateways de ciclo, vida o gestores. El proveedor exige callbacks dentro del intervalo/hilo observado y no inventa correlación para trabajo asíncrono. No otorga permisos de construcción, vida exclusiva de asignaciones ni render drenado. Los scopes de motor siguen siendo servicios pendientes, no constantes true.

Pausa/inventario por jugador, comandos compartidos, muerte/checkpoints/cutscenes, escenas sin compañero y validación conjunta siguen abiertos. No se ha dibujado el HUD P2 dentro de una partida. La implementación actual resuelve la conexión de fuente entre ciclo/vida/fases, no el cooperativo completo.

Se preservan todos los parches anteriores y los módulos Registry, LifetimeSource, Lifecycle, JanuaryBackend y sus gateways. Solo se amplía el callback opcional del ManagerHost para propagar el fallo de restauración. El estado previo se conserva íntegro en `docs/history/CURRENT_STATUS-before-pipeline-frame.md`; la imagen histórica 71f5e70d... no se reconstruye ni se renumera.
