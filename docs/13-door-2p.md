# 13 — Puertas 2P y estado por participante

Build principal: **BioRevHD 30-Enero-2013.exe**.

Base experimental canónica vigente: **v13 SYMMETRIC AMMO RELIEF**.

> Nota de continuidad: este documento empezó cuando la línea estaba en v11, pero la investigación siguió a v12 y v13 antes de cerrar la auditoría de puertas. **No existe todavía un parche de puertas/v14.** El contenido técnico de puertas sigue vigente; la base a preservar es v13.

Este documento inicia la auditoría de interacciones de mundo después de haber cerrado input, cámara, ItemBox y pickup.

## 1. Infraestructura nativa de puertas para dos jugadores

La build FullDebug contiene clases/strings explícitos:

```text
uDoor2pBase
cDoor2pBaseClosedState
Door2pBasePlayer_NetParam
cDoor2pBaseWaitState_PL
cDoor2pBaseCancelState_PL
uTwoOpenDoor
cTwoOpenDoorClosedState
cTwoOpenDoorOpenState
cTwoOpenDoorOpenState_PL
cTwoOpenDoorStopedState
```

También conserva la ruta de fuente:

```text
...\door_2p\cdoor2pbasestate.cpp
```

Por tanto las puertas que requieren dos participantes tienen una implementación específica, no son una puerta SP con un simple flag.

## 2. Layout relevante de uDoor2pBase

El registro de propiedades alrededor de:

```text
0x02586650
```

muestra estado explícitamente duplicado por índice 0/1.

Campos confirmados:

```text
+0x1010 mReadyFlag[0]
+0x1011 mReadyFlag[1]

+0x1018 mGuestStatusFlag[0]
+0x1019 mGuestStatusFlag[1]

+0x1024 mLocalFlag[0]
+0x1025 mLocalFlag[1]

+0x0FD1 mCallNpcFlag
+0x103C mNpcNearRange
+0x1060 mpDoorModel
+0x1074 mNetOrigin
```

También existen bloques paralelos por participante alrededor de:

```text
+0x1012/+0x1013
+0x1014/+0x1015
+0x1016/+0x1017
+0x101C + index*4

+0x1026/+0x1027
+0x1028/+0x1029
+0x102A/+0x102B
+0x1030 + index*4
```

Todavía no se asignan nombres semánticos a todos esos campos.

## 3. Setter local por actor — 0x02588D20

Thunk:

```text
0x01C9217B -> 0x02588D20
```

Solo se localizaron dos callsites directos:

```text
0x0257185D
0x0257198A
```

La función recibe un **actor** y obtiene de él el índice de participante mediante:

```text
0x01BEDBB7 -> 0x01CB7610
```

Después valida que el índice esté dentro de 0/1.

Para el índice válido escribe:

```text
mLocalFlag[index]
[this + 0x1030 + index*4]
[this + 0x1026 + index]
[this + 0x1028 + index]
[this + 0x102A + index]
```

La función retorna con:

```text
ret 0x18
```

### Punto importante

El setter **no filtra categoría `pl`/ `np`**.

Su criterio es el índice 0/1 derivado del actor.

Esto encaja con Sub0: si la acción de puerta le pasa el actor Sub0 real, la infraestructura puede escribir de forma nativa el slot 1.

## 4. Setter de ready-state

Otra rutina:

```text
0x02589040
```

recibe un índice explícito 0/1 y actualiza:

```text
mReadyFlag[index]
[this + 0x101C + index*4]
[this + 0x1012 + index]
[this + 0x1014 + index]
[this + 0x1016 + index]
mGuestStatusFlag[index]
```

retorna con:

```text
ret 0x1C
```

## 5. La lógica de puerta consume ambos slots

En la zona `0x02589123...` existen comprobaciones separadas de:

```text
mReadyFlag[0]
mReadyFlag[1]
```

y ciertos caminos requieren que ambos estén activos.

También se observan comprobaciones separadas de:

```text
mGuestStatusFlag[0]
mGuestStatusFlag[1]
```

alrededor de `0x025894C9/0x025894D8`.

Por tanto el objeto de puerta no está limitado estructuralmente a un único participante.

## 6. Uso del índice local global

Otras rutinas de puerta llaman:

```text
0x01C85962 -> 0x02DA3050
```

y usan el resultado directamente como índice 0/1 en arrays de la puerta.

Esto reproduce la arquitectura online habitual: cada proceso tiene un único índice de jugador local aunque el objeto de puerta almacene estado de ambos participantes.

**Todavía no se considera un bloqueo del local co-op**, porque primero hay que comprobar si los setters actor-específicos ya actualizan el slot de Sub0 y si las lecturas por índice global pertenecen solo a sincronización/feedback local.

## 7. Próximo paso

Antes de crear cualquier v12:

1. identificar el owner/estado de la función que contiene los callsites `0x0257185D` y `0x0257198A`;
2. reconstruir exactamente qué actor se pasa a `0x02588D20`;
3. comprobar si el wrapper/action de PcsSub puede llegar con Sub0;
4. separar lógica de gameplay de lógica de red/visual que usa el índice local global;
5. parchear únicamente si se demuestra un bloqueo concreto.

No se modificará globalmente el índice de jugador local.


## 8. Owner exacto de 0x02571710 — CONFIRMADO

La función que contiene los dos callsites prioritarios es:

```text
0x02571710
```

La vtable de `cDoor2pBaseClosedState` empieza en:

```text
0x04D5B02C
```

y su slot virtual 10 (offset `+0x28`) contiene:

```text
0x01C054A1 -> 0x02571710
```

El constructor:

```text
0x02570880
```

instala literalmente esa vtable.

El DTI global:

```text
0x055816B0
```

se registra con la string:

```text
cDoor2pBaseClosedState
```

Por tanto `0x02571710` es comportamiento real del estado cerrado 2P, no una rutina genérica sin owner demostrado.

La especialización `cTwoOpenDoorClosedState` también reutiliza ese mismo virtual:

```text
constructor 0x0257A1E0
vtable      0x04D5B9EC
DTI         0x055818BC
string      cTwoOpenDoorClosedState
```

Así que la ruta analizada llega a una puerta 2P concreta usada por el juego.

## 9. El objeto de puerta en 0x02571710 es uDoor2pBase — CONFIRMADO

Al entrar en `0x02571710`, el argumento del estado se convierte mediante:

```text
0x01C649C9 -> 0x02575680
```

contra el DTI:

```text
0x05581BC0 = uDoor2pBase
```

El resultado se conserva en:

```text
[ebp-0x14]
```

y ese mismo puntero se usa como `this` en:

```text
0x0257185D -> 0x01C9217B -> 0x02588D20
0x0257198A -> 0x01C9217B -> 0x02588D20
```

## 10. Actor del primer callsite: pPl / Self local — CONFIRMADO

La primera ruta obtiene el actor mediante:

```text
0x01C16468 -> 0x026D7EF0
```

`0x026D7EF0`:

1. obtiene `sNetworkManage`;
2. consulta el índice/identidad local mediante `0x01C85962`;
3. resuelve el actor correspondiente;
4. devuelve ese actor.

Después:

```text
0x01C78451 -> checked cast a uPlayer
```

El actor resultante se pasa como primer argumento a `0x02588D20` en `0x0257185D`.

Los asserts de `cdoor2pbasestate.cpp` conservan además el nombre fuente:

```text
pPl
```

para la ruta del jugador.

Conclusión: el primer callsite actualiza el slot correspondiente al Self/P1 local.

## 11. Actor del segundo callsite: pPt / participant no-Self — CONFIRMADO

La segunda ruta usa:

```text
0x01C4C9C3 -> 0x026D7F60
```

`0x026D7F60` recorre actors i:

- obté l'índex local;
- llegeix `actor+0xE3C` via `0x01BEDBB7`;
- descarta el candidat que coincideix amb l'índex local;
- retorna el participant no-Self.

Després el resultat passa pel mateix checked cast a `uPlayer`.

Just abans de continuar, els asserts literals del mateix fitxer font validen:

```text
(pPt) != NULL && TO_PTR(pPt) != 0xcdcdcdcd
pPt is invalid pointer(%p)
```

Per tant el segon callsite de `0x0257198A` és explícitament la ruta del partner/participant no-Self.

El setter `0x02588D20` no comprova `pl/np`; deriva el slot només de:

```text
actor+0xE3C
```

i accepta índex 0 o 1.

Això és compatible estructuralment amb el Sub0 exacte que la línia local-coop ja rastreja i converteix de Cpu a Pad.

## 12. El predicat de xarxa separa la gestió online i offline — CONFIRMAT estructuralment

Els dos punts:

```text
0x02571765
0x02571877
```

consulten:

```text
sNetworkManage
 -> 0x01BFEF16
 -> 0x02DA2820
```

No s'assigna encara un nom C++ inventat a aquest mètode, però el seu ús és inequívocament el d'un predicat de sessió/xarxa.

A `0x02571877`:

```text
predicate != 0 -> salta el bloc pPt local
predicate == 0 -> resol pPt i executa la ruta del segon participant
```

Això separa la gestió remota online de la gestió directa del partner en la mateixa instància.

## 13. Els usos de l'índex local global pertanyen a la branca de sync online — CONFIRMAT

La funció de sincronització de `uDoor2pBase` al voltant de:

```text
0x025871C0...
```

torna a consultar el mateix predicat de xarxa.

### Quan el predicat és cert

Usa:

```text
0x01C85962 -> local member/index
```

a:

```text
0x02587278
0x02587320
```

per seleccionar només l'estat del participant local i construir/enviar el paquet de sincronització.

### Quan el predicat és fals

La funció salta a:

```text
0x025873FC
```

i aplica directament els dos participants amb:

```text
push 0
call 0x01C6DC90 -> 0x02589040

push 1
call 0x01C6DC90 -> 0x02589040
```

És a dir, fora de la ruta online no depèn d'un únic índex local: aplica explícitament els slots 0 i 1.

Això descarta, de moment, el temor que `0x01C85962` sigui un bloqueig global per al cooperatiu local de portes.

## 14. La lògica de ready consumeix ambdós slots també offline — CONFIRMAT

`0x02589100` comprova directament:

```text
mReadyFlag[0] @ +0x1010
mReadyFlag[1] @ +0x1011
```

i després compara l'estat paral·lel dels dos participants.

Quan el predicat de xarxa és fals, també torna a resoldre `pPt` mitjançant:

```text
0x01C4C9C3
```

abans de decidir si l'estat 2P està complet.

Per tant la decisió de gameplay de la porta no està estructuralment reduïda al membre local de xarxa.

## 15. Conclusió actual de portes

Amb l'evidència actual:

- `cDoor2pBaseClosedState` processa Self i partner per separat;
- el partner té una ruta offline explícita (`pPt`);
- `0x02588D20` selecciona slot per actor, no per categoria `pl/np`;
- la sincronització offline aplica slots 0 i 1 explícitament;
- les lectures basades en l'índex local global observades fins ara són de la branca online.

**No hi ha encara un bloqueig demostrat que justifiqui v14.**

El següent punt és auditar l'entrada/interacció dels estats:

```text
cDoor2pBaseWaitState_PL
cDoor2pBaseCancelState_PL
```

i comprovar si l'ActionCommand/input del partner respecta el selector Pad 1 restaurat per v7 o si queda ancorat al pad principal.
