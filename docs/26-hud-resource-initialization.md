# 26 — Inicialización de recursos GUI y separación de instancias

Continuación desde PR #6 / `4d8b6833e6fff3d6ccacdebb935c907795806c7b`. Se leyó exclusivamente el original local de enero, tamaño 60.748.800, SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`. Ningún ejecutable se modifica o genera.

## Recursos e inicialización verificados

| Widget | Inicializador | Plantilla literal | VA de la string |
|---|---|---|---|
| Reticle | `0x02B402F0` | `id_bhr\10_cockpit\reticle` | `0x04DE53F4` |
| MainEquipWin | `0x02B3B280` | `id_bhr\10_cockpit\equip_wp_main` | `0x04CDDD14` |
| MapBaseAndHerb | `0x02B61A70` | `id_bhr\10_cockpit\map_win` | `0x04DE823C` |

Las tres rutas solicitan el recurso con los mismos argumentos de tipo `0x057ACA68` y flag `1`, y llaman al virtual `+0x6C` de la instancia. En las tres vtables ese virtual es `0x01C32F50 -> 0x035DEE10`. No se ha atribuido un nombre a flags no demostrado por metadata.

## El recurso no es el árbol mutable del widget

`0x035DEE10` ejecuta primero el virtual `+0xB0` (limpieza), valida el recurso y lo guarda en `this+0xF0` en `0x035DEECC`. Llama a `0x01BD91A3` sobre esa referencia.

Después reserva y construye una estructura de 0x80 bytes, almacenada en `this+0xF4` en `0x035DEFD7`. Reserva además la tabla de nodos `this+0xF8`. El bucle de `0x035DF101..0x035DF129` llama a la factoría virtual de cada tipo de nodo y guarda su resultado en esa tabla. `0x035DF223..0x035DF232` asigna a cada nodo la instancia GUI como owner mediante `0x01B87F1F -> 0x035E8B20` (escritura `node+0x6C`).

El getter de nodo `0x01C33C93 -> 0x02B0BCF0` lee `this+0xF8` e indexa su tabla. Por tanto los índices numéricos usados por estos inicializadores son locales a esa tabla; no es válido concluir que son IDs globales de jugador.

**Consecuencia:** usar la misma plantilla no implica reutilizar el mismo árbol mutable. La implementación debe crear cada objeto por su constructor/inicializador, no copiar bytes ni apuntar P2 al árbol de P1. Esto no prueba aún ausencia de todos los conflictos de registro global de GUI.

## Liberación y fallo parcial

El virtual `+0xB0` de las tres clases es `0x01C31E25 -> 0x035E08A0`. Su ruta elimina la estructura `+0xF4`, libera la tabla `+0xF8`, limpia esos campos y libera el recurso `+0xF0` con `0x01C3C159` en `0x035E0ABA`, dejándolo a cero en `0x035E0AC2`. Los inicializadores liberan también su referencia temporal después del enlace: `0x02B40374`, `0x02B3B304`, `0x02B61AF4`.

La instancia completa se destruye por su virtual cero con argumento 1, como los managers. No deben liberarse por separado los nodos y después volver a ejecutar el destructor de la instancia.

**Advertencia:** esos inicializadores retornan `void`; no se puede interpretar EAX como un booleano de éxito. En sus rutas de recurso ausente desactivan la unidad y salen. El adaptador debe comprobar postcondiciones de recurso/árbol antes de publicar una instancia.

## Creación nativa observada

La mira se reserva con tamaño `0x2E0`, alineación `0x10`, llama al constructor `0x01C19CCB -> 0x02B40140`, y después al virtual `+0x14` (inicialización). Se ve en `0x02B48F0F..0x02B48F74`. Mapa/hierbas reserva `0x2C0` con alineación `0x10` y constructor `0x01C6392A -> 0x02B61890`, en `0x02B68183..0x02B681A7`.

El constructor de Reticle pasa la constante 0x69 a `0x01C3B515 -> 0x02CD29E0`; esa ruta usa un campo de 20 bits y otra transformación. No se inventa aquí un ID nuevo para P2 ni se da por resuelta la inscripción global. Crear objetos fuera de los arrays/listas originales evita tocar los slots de P1, pero requiere un dispatch separado y demostrar los consumidores de ese identificador.

## Implementación siguiente

Añadir un ciclo de vida transaccional al registro existente: preparar instancias separadas, verificar recursos y asociación, publicar juntas solo tras validación, invalidar antes de liberar y mantener tombstones hasta que no puedan llegar callbacks antiguos. Debe retirar únicamente las instancias que él creó. No publicar un segundo HUD parcialmente inicializado ni duplicar la liberación del alias +0x40 del mini-mapa.

Sigue pendiente instalar el adaptador en fases reales del motor y verificar recursos/IDs, transformaciones y clipping dentro de la campaña. Esta evidencia no declara un HUD 2P terminado.
