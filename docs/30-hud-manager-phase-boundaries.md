# 30 — Fases del HUD: gestores distintos y entradas de recorrido

Continuación de PR #8, base f420a90f903af9d23069f18dc8978588d063bf34. Se leyó el original local de enero (60.748.800 bytes, SHA-256 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69). No se genera ni modifica una imagen del juego.

## Problema al conectar el controlador anterior

Lifecycle::dispatch tiene un único contador de fases por fotograma y ejecuta sus tres widgets en cada llamada. No distingue el recorrido del cockpit del de uMiniMapManager. Conectar esa misma llamada a ambos gestores ejecutaría el mapa bajo la fase/contexto del cockpit; la siguiente llamada del minimapa sería rechazada como duplicada. Es un defecto del contrato de integración del componente, no un fallo observado jugando.

Se necesitan fases independientes por gestor y conservar los contextos de cada uno. La antigua llamada agrupada solo sirve para las pruebas históricas con permiso explícito para ambos gestores; no debe conectarse a los hooks nativos.

## Puntos concretos comprobados

| Gestor | Fase | Entrada al recorrido | Bytes de dos instrucciones completas |
|---|---:|---|---|
| Cockpit | 8 | 0x02B497CA | 8b45f88b483c |
| Cockpit | 9 | 0x02B49C5B | 8b45f88b483c |
| Cockpit | 11 | 0x02B49DE3 | 8b45f88b483c |
| MiniMap | 8 | 0x02B6847B | 8b45f883c030 |
| MiniMap | 9 | 0x02B685B3 | 8b45f883c030 |
| MiniMap | 11 | 0x02B68724 | 8b45f883c030 |

En todos esos puntos [ebp-8] contiene el gestor. En dibujo [ebp+8] conserva el contexto; las fases 8/9 no reciben ese argumento. Los puntos cockpit están en la rama que recorre la lista desde +3C; no en las ramas especiales que procesan únicamente ciertos slots. Los puntos MiniMap preparan el array +30 de cuatro elementos.

## No enganchar el epílogo común del minimapa

En el dibujo de MiniMap, 0x02B686E8 consulta un estado; si no procede dibujar, 0x02B6870B salta directamente a 0x02B687F0. Ese epílogo también es la salida normal del bucle. Un callback incondicional allí confundiría la salida de omisión con permiso de dibujo.

La entrada 0x02B68724 se alcanza después de la condición y de obtener el índice de vista en 0x02B68713. No se atribuye un nombre semántico inventado al estado probado. En Cockpit, las ramas especiales también eluden las entradas ordinarias 0x02B497CA y 0x02B49C5B.

## Alcance

Estos seis puntos demuestran admisión a un recorrido de ese gestor, no ausencia de trabajo de render en vuelo ni un safe point para construir/destruir. Los hooks de fase no deben conceder permiso Structural ni inventar un lifetime del actor. Las transformaciones/clipping, sincronización de render, producción de Session/epoch y activación de hooks siguen separados.

Siguiente implementación: dispatch por gestor en Lifecycle, puentes fuente que preserven registros/flags/estado de coma flotante y adaptador que rechace contexto, gestor, hilo o ticket incorrectos. Conservar los tests históricos y añadir regresiones de los dos gestores en el mismo fotograma.
