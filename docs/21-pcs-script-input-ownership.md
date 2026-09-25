# 21 — Selectores de guion: ownership de PcsSub

Fecha: 2026-09-25. Continuación desde `5bcf15adf2f3e6526c71d5b6d3fdbeb9a841559f`, sin sobrescribir ramas paralelas. Análisis de los EXE locales; no se descargó ningún EXE de GitHub.

## Identidad y reproducción de la base

Original enero: `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69` (60.748.800 bytes).

Se recompilaron las copias exactas de `core.cpp`, `hooks.S` y `link.ld` de la rama build-audit. Sus blobs son respectivamente `5aa0960d646540fce9ee13c92c483c31997ac401`, `c5df39261afa1bdff1782807bd4b75fd61387d1e` y `e28d640bffd784e7523f1effa9c1832a72dfbcb5`.

La reconstrucción desde owner-fix `e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a` reprodujo exactamente LOCAL ROUTING EXPERIMENTAL:

`0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378`, 60.755.456 bytes.

No se vuelve a empezar por pickups ni se modifica la implementación anterior de puertas/reanimación.

## 1. Dos callbacks pertenecen a dos grupos del mismo objeto de guion

| Callback | Thunk | Registro del delegate | Grupo de comandos |
|---|---|---|---|
| cFsmAction `0x02976C30` | `0x01BBB149` | push del thunk en `0x029707B1` | seis comandos, `this+0xA0`, stride `0x110` |
| cFsmActionPcs `0x029FF600` | `0x01C7E054` | push del thunk en `0x029FD2E7` | seis comandos, `this+0x860`, stride `0x110` |

Ambos callbacks devuelven cero y terminan con `ret 4`. Los grupos se instalan mediante `0x01C7F26A` en `0x029708CE` y `0x029FD3F5`. Los límites de seis elementos se comprueban en `0x029708AB` y `0x029FD3D8`.

El constructor de cFsmActionPcsSub `0x02A75BC0` llama con el mismo ECX al de cFsmActionPcs en `0x02A75BE6`. Este llama al de cFsmAction en `0x029FD276`, también sin ajuste de this. Por tanto los dos grupos heredados pueden tener como owner el objeto PcsSub completo.

RTTI de gameplay verificado por vtable[-1] -> CompleteObjectLocator+0x0C -> TypeDescriptor+8:

- cFsmAction: `0x04DBD47C`.
- cFsmActionPcs: `0x04DC7AD8`.
- cFsmActionPcsMain: `0x04DCBFAC`.
- cFsmActionPcsSub: `0x04DCF41C`; nombre `.?AVcFsmActionPcsSub@fsm@game@app@@`.

El constructor Sub instala `0x04DCF41C` en `0x02A75BEE` y pone a cero `+0x1078`. No se interpreta ese offset en un objeto de otra clase.

## 2. +0x1078 es el contexto de actor, no un número de mando

El setter `0x01C3696B -> 0x02DCFBC0` recibe tres argumentos. Guarda el tercero en `this+0x1078` mediante `0x02DCFC01`, y termina con `ret 0x0C`.

Su caller en `0x02DCFA8B`, dentro de `0x02DCF960`, proporciona:

- argumento 3: objeto de `sPcsManager+0xB44 + grupo*0x44 + slot*4`, push en `0x02DCFA59`;
- argumento 2: serial de la tabla paralela en `+0x704`, push en `0x02DCFA70`;
- argumento 1: slot, push en `0x02DCFA74`.

El productor `0x02DD04E0` escribe el objeto en la tabla `+0xB44` en `0x02DD0549`, obtiene su serial mediante `0x02DD0562 -> 0x01BEDBB7`, y lo guarda en la tabla `+0x704` en `0x02DD057A`. No se intercambian las dos tablas.

El resolver Sub `0x02A91F90` devuelve `[this+0x1078]` cuando se solicita el selector 16. Otras acciones Sub pasan ese mismo puntero a las implementaciones comunes.

Esto permite decidir ownership por igualdad con el Sub0 vivo rastreado, sin sustituir globalmente Self ni el índice de red.

## 3. Corrección construida para los dos callbacks

Los dos prefijos de entrada, de nueve bytes, saltan a un helper original en `0x01C95340`. La zona contenía 54 bytes CC en la base exacta. La rutina mantiene `ret 4` y no llama al motor ni escribe memoria.

Devuelve 1 solamente si se cumplen todas estas condiciones:

1. owner no nulo;
2. indicador local distinto de cero;
3. vtable EXACTA de cFsmActionPcsSub;
4. tracker Sub0 no nulo;
5. contexto `owner+0x1078` igual al tracker;
6. ThinkMode del actor igual a Pad (1).

Devuelve 0 para PcsMain, acciones genéricas, otros actores, contexto no resuelto, modo local inactivo y Network/Cpu. La identidad se compara antes de desreferenciar el actor. ECX y registros preservados por ABI permanecen intactos; EAX/EDX son temporales.

La ejecución común del ActionCommand en `0x01E8CB20` consulta `0x02DB2B50`, `0x02DB0080` y `0x02DAEB40`. Los cuatro puntos de carga de selector comprobados en esas funciones ya están corregidos en la base actual. No se altera de nuevo sGamePad.

Candidato experimental, NO validado jugando:

`a7199ab890cdc004eaa85ebad587a6af0cc3efdc035a96607ac7f2ce5e940e9b`

Tamaño: 60.755.456 bytes. Diferencias: 54 bytes del helper, 18 bytes en las dos entradas y dos bytes efectivos del checksum PE: **74 bytes** en total. Secciones y tamaño idénticos. La reversión reconstruye la base `0c019d43...` byte por byte.

Helper SHA-256: `962da0c9ff2da1afa6ae58a1e22036d5dd93b2513a1f42d3762095b9fd59aae7`.

## 4. Pruebas efectuadas

Pasaron 12 métodos de test, sin fallos ni omisiones. Incluyen una matriz de 720 combinaciones del selector interpretando sus bytes extraídos del candidato, owner nulo, conservación de registros, lecturas limitadas, siete rechazos CLI, rechazo de una mutación que aceptaría Network, correspondencia entre assembly y bytes, RTTI/delegates, preservación del módulo anterior y reversión completa.

El intérprete solo admite las instrucciones del helper. No ejecuta el motor, no es un emulador del juego y no valida una partida. Una prueba separada de ejecución ELF i386 fue rechazada por el entorno; no se cuenta como ejecución x86 nativa.

## 5. Tercer selector: uPcsInput, todavía separado

`0x01C0758A -> 0x02DEE8D0` pertenece a uPcsInput, vtable `0x04E15EF4`. Su padre es uPcs, vtable `0x04E15F64`, no un actor. Su comando está en `+0x50`; no se puede leer `+0x1078` en esta clase.

El campo `uPcs+0x30` se establece en `0x02DEE004` al encontrar el uScheduler que contiene ese objeto. DTI uScheduler: `0x0579963C`; su registro usa la string `uScheduler` de `0x04EC5D84` en `0x04B00ACF`.

El resolver de `sPcsManager 0x02DD0C80` compara schedulers de 16 PcsMain (stride 0x1070) y 17 PcsSub por grupo (stride 0x1080). El getter usado en esa comparación devuelve `cFsmActionPcs+0xF90`. Se está siguiendo esta relación para atribuir uPcsInput a un actor sin confundir grupo de guion con índice de mando. El candidato descrito arriba deja ese tercer callback intacto.

## Alcance que no debe exagerarse

Este cambio corrige dos selectores en objetos PcsSub de actor identificado. No significa que todos los QTE, guiones, interfaces o escenas de la campaña estén cubiertos. HUD, pausa/inventario por jugador, muerte/checkpoints, cámaras forzadas y creación de P2 en escenas sin partner siguen pendientes de implementación o validación. No se ejecutó Resident Evil Revelations.
