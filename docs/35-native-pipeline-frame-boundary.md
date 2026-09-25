# 35 — Ciclo nativo exterior para identificar las fases del HUD

Continúa PR #10 / 7bd05d2018739dfa47641b21b793df8b75d81662. Se analiza el original local de enero, SHA-256 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69, 60.748.800 bytes, en solo lectura. No se genera otro EXE.

## Un contador distinto, pero no un fence

La cadena literal sRender::begin en 0x04F013D4 se referencia desde 0x033BFDAE, dentro de 0x033BFD80. Esta rutina incrementa sRender+0x33DE0 en 0x033BFE47..0x033BFE5B. El getter 0x034FD470 lee el mismo campo. Antes existe una salida por sRender+0x20; no debe tratarse cada invocación de begin como un incremento.

El campo se utiliza también para elegir buffers módulo 3 en 0x0370D1F0 y 0x0370D230. No es prueba de presentación completada ni de GPU o callbacks drenados. sRender::end 0x033BFFC0 contiene una salida por espera en 0x033C00B6 y una llamada virtual de dispositivo en 0x033C00F3 seguida de comprobación de 0x88760868. Por tanto no se afirma una correspondencia uno-a-uno entre ese contador y imágenes realmente presentadas.

## Ciclo exterior identificado

La vtable 0x04E38B3C corresponde a sSkeletonMain: vtable[-1] -> COL 0x053965A8 -> descriptor 0x054EEE9C -> .?AVsSkeletonMain@@. Su virtual 6 apunta a 0x01B826B4 -> 0x02F50680.

La función 0x02F50680..0x02F50F5E agrupa la iteración nativa de subsistemas. Contiene:

- 0x02F507F6 -> 0x01BE37D9 -> 0x033BFD80, begin del renderer;
- llamadas de actualización de subsistemas después de begin, incluido el slot de GUI;
- 0x02F50D28, virtual 6 del renderer, cuya vtable 0x04F042D8 apunta a 0x01C15E73 -> 0x033C7330;
- 0x02F50E7E -> 0x01BEBCC7 -> 0x033BFFC0, end del renderer;
- un único retorno normal en 0x02F50F5E.

La ventana completa tiene 2.271 bytes, SHA-256 a2c8f20788ce3a48cb01f468bb92241fca65eb82e723b26131266e670f88d894. Las cuatro bifurcaciones condicionales directas de su cuerpo saltan a puntos internos posteriores (0x02F50942, 0x02F50A2C, 0x02F50AE3); no saltan el epílogo. Esto no modela excepciones ni callbacks de motor no retornantes.

## Entradas seleccionadas

BEGIN en 0x02F506A0, seis bytes 894df88b4df8: guarda ECX en [EBP-8] y vuelve a cargarlo. El aviso recibe ECX original y el EBP de esa invocación, antes del replay. Continúa en 0x02F506A6.

END en 0x02F50F4B, nueve bytes 5f5e5b81c4d0000000: restaura EDI/ESI/EBX y suma 0xD0 a ESP. El aviso recibe [EBP-8] y EBP antes del replay. Continúa en 0x02F50F54; los flags finales deben ser los del ADD original, no los anteriores al gateway.

El número nuevo se incrementará una vez por BEGIN de este ciclo exterior, nunca por cada manager, widget o vista. END cerrará la ventana de captura. Dos gestores en la misma invocación comparten número; otra iteración puede obtener otro aunque el contador de simulación de sUnit siga detenido por pausa. Se denomina ciclo CPU de actualización/dibujo, NO número de Present exitosos.

## Límites de la conexión

Este límite acredita identidad temporal de una invocación en el hilo observado, no vida de actores, cobertura de todos los callbacks, permiso Structural/Destroy, render drenado ni proyección/scissor. Un callback ejecutado fuera de la ventana o en otro hilo debe rechazarse, no recibir el último número como si fuera vigente. La correlación de callbacks asíncronos, si los hay, requiere evidencia adicional y no se inventa copiando el contador del productor.

El proveedor compondrá este número con LifeSnapshot del PR #10 y volverá a comprobar la ventana después de capturar vida. Los scopes reales de dibujo y el bloqueo de render permanecen como permisos separados. Los nuevos gateways se entregan solo como fuentes; no están instalados en el proceso ni activan el HUD por sí solos.
