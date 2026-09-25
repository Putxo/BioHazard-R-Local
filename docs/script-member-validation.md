# Continuación construida: comandos de guion del Sub0 local

Fecha: 25 de septiembre de 2026. Estado: **EXPERIMENTAL; NO VALIDADO JUGANDO**.

Esta continuación no vuelve a empezar en pickups ni sustituye los cambios anteriores de puertas, ayuda y reenlace. Añade dos rutas concretas de selección de mando para acciones de guion con un actor Sub0 identificado. No equivale a terminar todos los QTE, la interfaz o la campaña cooperativa.

## Archivo construido

| Imagen | SHA-256 | Bytes |
|---|---|---:|
| Original de enero | `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69` | 60.748.800 |
| Base LOCAL ROUTING | `0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378` | 60.755.456 |
| SCRIPT EXPERIMENTAL | `71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4` | 60.755.456 |

La base LOCAL ROUTING fue reconstruida a partir de los tres archivos publicados del módulo (`core.cpp`, `hooks.S`, `link.ld`) en `5bcf15adf2f3e6526c71d5b6d3fdbeb9a841559f`. El resultado coincidió exactamente con su SHA documentado y revirtió a la base owner-fix. Esto acredita identidad de entrada, no jugabilidad.

## Corrección de código

Se reemplaza la entrada de `cFsmAction::0x02976C30` y `cFsmActionPcs::0x029FF600` por una función propia de 72 bytes en `0x01C95340`. Los cuerpos originales retornaban cero, con ABI `thiscall` y `ret 4`.

La nueva función devuelve 1 solamente cuando se cumplen todas estas condiciones:

- El owner no es nulo y la sesión local está activa.
- Su vtable es exactamente `cFsmActionPcsSub` (`0x04DCF41C`). No se lee el layout extendido en otras clases.
- El tracker Sub0 no es nulo y ese actor sigue en ThinkMode::Pad. Cpu y Network quedan excluidos.
- El contexto `owner+0x1078` coincide con el actor rastreado. El contexto no se desreferencia a ciegas.
- El serial del actor es no negativo y coincide con `owner+0x1074`.

En cualquier otro caso se conserva el retorno cero original. Los guiones Main/globales no se convierten indiscriminadamente a P2. La función no llama al motor, no escribe memoria y preserva ECX y los registros no volátiles.

El cambio incremental tiene **92 bytes efectivamente distintos**, limitados a los dos prefijos, la función nueva y el checksum PE. Se mantiene el tamaño y las secciones anteriores; la reversión reproduce exactamente LOCAL ROUTING. El callback `uPcsInput::0x02DEE8D0` no se modifica.

## Coordinación con el trabajo paralelo

La primera construcción de esta rama (`a7abf154...`) leía el modo del tracker antes de comparar el contexto. Se adoptó el orden más defensivo de `research/script-member-serial-validation` en `ec64e21a...`: primero debe coincidir el contexto y solo entonces se desreferencia el actor. La nueva salida coincide **exactamente**, por SHA y tamaño, con el candidato paralelo `71f5e70d...`; no se apilan los dos parches. El test añadido verifica un tracker inválido con contexto distinto sin leer ese tracker.

La función ensamblada final tiene SHA `500a82d8071f5c40ece76397fee1bac97888768a3e9029d52222d4d933eb7896`. El builder de esta rama es una alternativa reproducible, no una segunda modificación para aplicar encima del candidato paralelo.

## Pruebas ejecutadas y sus límites

**Local, con las imágenes reales aportadas/construidas:** `patches/test_script_member.py` pasó 16 tests, sin errores ni omisiones. Incluyen identificación de clases y enlaces originales, los bytes del candidato, ambos saltos de entrada, exclusividad de cambios, reversión y rechazos de CLI.

El test de condiciones recorre 2.592 combinaciones de flag, clase, tracker, modo, contexto y serial. Un intérprete limitado ejecuta exclusivamente las instrucciones del selector y comprueba las lecturas de memoria, registros y limpieza de pila. No interpreta el motor ni constituye una prueba de gameplay.

**CI pública de fuentes:** commit `2affdb8d5c9bc4022c69fad87eb0d20c31cd27d6`, workflow `Script member leaf tests`, run `36127918860`, dos jobs correctos:

- `portable-bytecode`: comprobaciones portables del selector; tres tests que requieren las imágenes privadas quedan omitidos en CI.
- `native-i386`: ejecuta la función ensamblada real en un proceso de pruebas Linux i386 con fixtures en las direcciones de sus dos globals. Prueba 2.592 combinaciones y la convención ECX/ret4, registros y pila. No enlaza ni ejecuta código del motor.

El entorno local sí ensambla ese ejecutable i386, pero su kernel no permite ejecutarlo; la ejecución nativa anterior está acreditada por el log del job `108048218769` en GitHub. No se presenta como ejecución Windows del juego.

**Instalador acumulativo:** 17 comprobaciones de transformación/rechazos pasaron, incluidas operaciones reales de CLI original→candidato→original, protección de originales, payload alterado y rangos inválidos. El generador reproduce el mismo payload por SHA.

## Instalador reproducible desde el original

`patches/portable_script_member/apply.py` aplica un payload verificado con Python 3.10 o posterior, sin compiladores. Nunca ejecuta el juego, sube archivos ni reemplaza la entrada o una salida existente. `--reverse` genera una copia original separada.

El paquete local incluye `payload.json`. El repositorio conserva su generador `make_payload.py`, que acepta solamente las dos imágenes identificadas arriba y reproduce su hash:

`deef32213898d607995c41b2986e5de4b864da4401c8763a6361fd68513d528d`

El payload contiene 151 rangos mínimos: 1.676 bytes efectivos modificados respecto al original y 6.656 bytes añadidos. No es una copia completa del EXE. Los EXE permanecen fuera de GitHub.

## Continuación exacta y límites abiertos

Leer también `docs/gui-and-scheduler-ownership.md`. Se ha confirmado que el padre consultado por uPcsInput es un `uScheduler`, no un jugador. Sus comandos compartidos no deben asignarse arbitrariamente a Pad 2.

En la interfaz se ha identificado `uCockpitManagerMain`, la creación de la mira y cuatro búsquedas explícitas de Self. La siguiente implementación debe coordinar instancia de widget, actor y viewport; sustituir globalmente Self por Sub0 sería incorrecto.

Este candidato **no implementa** el HUD/pausa completo por jugador, el flujo completo de muerte/checkpoints/cutscenes ni la creación de P2 en escenas sin compañero. No hay una partida ejecutada con él ni prueba de render y dos mandos juntos. Los 16 tests y CI no cierran esos requisitos.
