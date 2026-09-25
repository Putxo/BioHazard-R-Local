# 37 — Entrada común de dibujo GUI: no añadir escalado sin seguir el contexto

Análisis adicional de solo lectura durante la continuación del proveedor de fotograma. Original de enero: 60.748.800 bytes, SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`. No se construye un EXE ni se modifica el render.

## Recorrido comprobado

| Widget | Thunk de dibujo | Implementación | Llamada común |
|---|---|---|---|
| Reticle | `0x01C6EF7D` | `0x02B40740` | `0x02B40800 -> 0x01C19FFA` |
| MainEquipWin | `0x01C6445B` | `0x02B3BBA0` | `0x02B3BC11 -> 0x01C19FFA` |
| MapBaseAndHerb | `0x01C67381` | `0x02B624E0` | `0x02B62551 -> 0x01C19FFA` |

El thunk común lleva a `0x02CD28E0`; su llamada de `0x02CD2931` pasa por `0x01BAA132` hasta `0x035DD4E0`. Es una ruta condicionada por el estado de la GUI, no una autorización para invocarla fuera del manager o saltarse sus gates.

En `0x035DD5ED` se obtiene el argumento de contexto con `mov ecx,[ebx+8]`. Este método tiene un frame alineado dinámicamente: EBX es la base de argumentos; no debe sustituirse por una lectura de EBP propia de otra rutina.

`0x035DD5F0 -> 0x01BE516F -> 0x02B02F00` obtiene una estructura a partir de ese contexto. El helper suma `0xBC` al objeto en `0x02B02F26` antes de copiarla. La rutina de dibujo la compara con información cacheada en la instancia GUI a partir de `+0x168`, evidenciada por `0x035DD605`.

Dos llamadas relacionadas siguen hacia `0x035E1280` y `0x035E1790` por los thunks `0x01C70CD8` y `0x01C62980`, desde `0x035DD622/0x035DD697`. La segunda contiene operaciones de razones entre componentes de dimensiones y escrituras en el array de la GUI a partir de `+0x1C8`. Esto es una pista para seguir la transformación nativa, no un nombre definitivo del tipo o de cada componente.

El recorrido también construye un contexto auxiliar mediante `0x035DD6E9 -> 0x01C09484` y alcanza su limpieza por `0x035DD7F6 -> 0x01C852EB`. Su existencia no demuestra un fence ni que no queden trabajos encolados después del draw.

## Consecuencia para el parche

La GUI ya consume datos del contexto de dibujo. Añadir directamente un factor 0,5 al widget podría aplicar una segunda transformación o alterar el diseño de P1. **No se implementa aquí ese escalado.** Primero deben identificarse los productores y el significado de `context+0xBC`, sus valores en las vistas TOP/BOTTOM y los consumidores finales que fijan proyección, viewport y recorte.

Se mantienen separadas cuatro cuestiones: seleccionar el actor correcto, seleccionar la vista por mDrawView, transformar coordenadas y recortar el resultado. La evidencia de una no prueba las otras. Los campos `GUI+0x148`, sus bits y los arrays internos no se renombran como IDs de jugador sin metadata que lo sustente.

## Evidencia guardada

`research/reports/gui-common-draw-entry-map.json` conserva 20 ventanas de instrucciones/thunks verificadas contra el original. Es un mapa focalizado del recorrido; no contiene assets ni una función propietaria completa. Se releyeron sus 20 ventanas y se recalculó el hash del original al cerrar la investigación.

La prueba de fotograma de docs/36 no se contabiliza como validación de este dibujo. No se ejecutó ninguna función del motor, no se instaló el HUD ni se comprobó la salida gráfica en campaña.

Siguiente punto exacto: seguir los productores de la estructura devuelta desde `0x02B02F00` y las operaciones de `0x035E1280/0x035E1790`; contrastar el scissor/proyección final y el scope de restauración. Preservar el snapshot de vida y el ciclo CPU ya conectados, y no confundir el retorno de esa función con GPU drenada.
