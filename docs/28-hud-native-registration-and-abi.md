# 28 — Registro nativo y ABI de las unidades HUD de enero

Continuación de PR #7 desde `b50210223326eceef637a817fd6ddd129c59d6d8`. Análisis de solo lectura del original de enero de 60.748.800 bytes, SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`. No se genera ni modifica un EXE del juego.

## Construir no equivale a añadir a sUnit

El constructor común cUnit `0x03477500` deja los siete bits 3..9 de `unit+0x0C` a 127 y los enlaces `+0x14/+0x18` a cero. La cadena examinada es widget → uBioCockpitGUI/uBioGUI → uGUI `0x035DCFB0` → base `0x03357740` → cUnit. El argumento cero que pasan los widgets a la base no se interpreta como un supuesto modo detached: no se ha demostrado esa semántica.

La inscripción real `0x01BF2BC6 → 0x0326A0D0` conserva diagnósticos literales `sUnit::addBottom` y `sUnit : MAX_MOVELINE OVER!`. Escribe enlaces y sustituye el índice de moveline. El cockpit la llama en `0x02B4C61B` para su slot `+0x8C`, después de reconstruir su lista; no añade allí todos los widgets de esa lista.

La función `0x0326AA90` recibe el puntero de unidad, recorre las listas bajo sus llamadas de sincronización y devuelve 1 si lo encuentra, 0 si no. Termina en `ret 4`. Por tanto los enlaces cero no son prueba suficiente de desvinculación: una lista con un solo elemento puede tenerlos a cero. El adaptador debe comprobar tanto los marcadores locales como la pertenencia real a sUnit. No debe borrarlos para forzar una desvinculación.

Esto acota una ruta de construcción directa sin añadir otra inscripción deliberada. No demuestra por sí solo ausencia de todos los callbacks/registros transitivos ni un punto seguro de render: esos permisos siguen siendo responsabilidad del driver.

## Asignadores y constructores concretos

| Tipo | Tamaño | Asignador cdecl(size, alignment) | Constructor thiscall sin argumentos |
|---|---:|---|---|
| Reticle | 0x2E0 | 0x01C11706 → 0x02B3FDD0 | 0x01C19CCB → 0x02B40140 |
| MainEquipWin | 0x370 | 0x01C15775 → 0x02B3AD10 | 0x01B7D1F0 → 0x02B3B080 |
| MapBaseAndHerb | 0x2C0 | 0x01C7855A → 0x02B61520 | 0x01C6392A → 0x02B61890 |

Los tres callsites de asignación pasan alineación 0x10 y limpian ocho bytes en el caller. El inicializador virtual 5 no recibe argumentos y devuelve void. El destructor escalar virtual 0 recibe flag 1 y limpia cuatro bytes. No se debe llamar al destructor y después liberar el mismo bloque otra vez.

## Rectificación del contrato de fases

Los recorridos de los gestores invocan los slots 8 y 9 **sin argumentos de stack**. Solo el slot 11 recibe el contexto de dibujo. Un adaptador que pasara ese contexto como argumento a todas las fases desequilibraría la pila o no reproduciría la firma real.

Witnesses de cockpit: llamadas `0x02B49834` (slot 8), `0x02B49CB2` (slot 9), y `0x02B49E4B` (slot 11, precedida del push del contexto en `0x02B49E3F`). MiniMap hace la misma separación.

Antes del slot 8 se propaga el multiplicador temporal del manager al widget: getters/setters `0x01EB7280/0x01EB72C0`, campo `+0x1C`. No se interpreta ese valor como delta de frame sin evidencia adicional.

El cockpit comprueba Active y ForceSkip antes de ejecutar sus widgets. El Active auditado es el bit `0x4000` de `unit+0x0C`, mediante `0x01C326DB → 0x01CB25D0`; ForceSkip es un byte de cockpit en `+0x294`. En MiniMap el update ordinario no contiene ese mismo gate y su draw sí comprueba Active y mDrawView. **No se debe leer +0x294 de MapBaseAndHerb como ForceSkip: es mHerbHaveNum.**

El contexto de dibujo expone el índice de vista en `context+0x158 & 0xFF`. Las ramas globales de modo/visibilidad del manager y la transformación/clipping no quedan sustituidas por estos gates de widget.

## Próximo bloque de código

Implementar operaciones nativas con estas firmas, targets permitidos y comprobaciones de pertenencia, conectables al controlador existente. Los rechazos de inicialización/fase/destrucción deben propagarse: un callback void que rechaza silenciosamente destruir no puede hacer que el controlador dé el objeto por liberado. Mantener bloqueo de activación si no existen permisos reales de hilo, ciclo de vida y render. Ninguna prueba aislada de este adaptador se presentará como ejecución del juego.
