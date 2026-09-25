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


## 16. WaitState_PL: omissió real de Pad 2 demostrada

Vtable:

```text
cDoor2pBaseWaitState_PL = 0x04D5B220
```

L'update/estat real conté dues consultes directes a `sGamePad`:

```text
0x025746A7 -> 0x01B94F53 -> 0x02DB0CF0
0x025747E2 -> 0x01C0DC6E -> 0x02DB0DC0
```

Abans de totes dues, la porta calcula:

```text
0x02574661 -> 0x01C6C746 -> 0x027A27B0
```

Aquest és el selector de jugador/pad ja reutilitzat per la línia local-coop.

Stock:

```text
0x027A27B0 -> 0
```

v6/v7/v13:

```text
actor == gSub0Npc -> 1
resta             -> 0
```

El mateix resultat es passa com únic argument tant a `0x02DB0CF0` com a `0x02DB0DC0`.

### Getter 0x02DB0CF0 — ja corregit a v7

Stock:

```asm
0x02DB0D25 mov edx,[ecx+0x970] ; mStartPadNo
0x02DB0D2F mov ecx,[eax+0x970] ; mStartPadNo
```

v7+:

```asm
0x02DB0D25 mov edx,[ebp+8]
0x02DB0D2F mov ecx,[ebp+8]
```

### Getter 0x02DB0DC0 — omès per v7

v13 encara conté:

```asm
0x02DB0DF5 mov edx,[ecx+0x970]
0x02DB0DFF mov ecx,[eax+0x970]
```

Tot i que la funció rep el mateix selector i acaba amb `ret 4`.

La cerca completa de callsites confirma que:

```text
0x01C0DC6E -> 0x02DB0DC0
```

té un únic caller directe en tot `.text`:

```text
0x025747E2
```

Per tant és un bloqueig concret i acotat de `cDoor2pBaseWaitState_PL`: una de les dues consultes ja usa PadData[1] per Sub0 i l'altra encara consulta el pad global principal.

## 17. Candidat v14 — DOOR WAIT PAD2

S'ha construït sobre v13 sense tocar cap altra lògica:

```text
BioRevHD 30-Enero-2013 LOCAL COOP v14 DOOR WAIT PAD2.exe
SHA-256:
5ae917d141aed577d0cb111e8c17eb2efe64d6940a52d7cc00ad1f3faff74e49
```

Canvis:

```text
0x02DB0DF5
8b9170090000 -> 8b5508909090

0x02DB0DFF
8b8870090000 -> 8b4d08909090
```

És exactament el mateix patró de restauració del selector que v7 ja aplica a la funció germana.

Verificació:

```text
mateix tamany PE: sí
bytes efectivament diferents vs v13: 10
rangos: 2
revertir els dos punts -> SHA v13 exacte
```

Builder:

```text
patches/build_v14_door_wait_pad2.py
```

Manifest:

```text
research/manifests/local-coop-v14-door-wait-pad2.json
```

Estat:

**STATICALLY VERIFIED ONLY — runtime pendent.**

Encara no es declara canònica fins acabar l'auditoria de totes les classes `door_2p` i comprovar si convé incorporar algun altre bloqueig de porta a la mateixa v14.


## 16. WaitState_PL calcula selector por actor y lo pasa a sGamePad — CONFIRMADO

La vtable de `cDoor2pBaseWaitState_PL` es:

```text
0x04D5B220
```

y su update principal contiene dos consultas a `sGamePad`.

Dentro del update, el actor activo está en:

```text
[ebp-0x14]
```

Antes de leer input:

```asm
0x02574661  mov ecx,[ebp-0x14]
0x02574664  call 0x01C6C746   ; selector de pad del actor
0x02574669  mov [ebp-0xBC],eax
```

La línea v6/v7 redefine ese selector para que:

```text
P1   -> 0
Sub0 -> 1
```

El mismo valor se pasa a dos APIs de `sGamePad`:

```text
0x025746A7 -> 0x01B94F53 -> 0x02DB0CF0
0x025747E2 -> 0x01C0DC6E -> 0x02DB0DC0
```

Ambas funciones terminan con `ret 4`, por lo que reciben un selector explícito.

## 17. 0x02DB0CF0 ya fue restaurada por v7 — CONFIRMADO

Stock PC en `0x02DB0CF0` ignoraba el selector y cargaba:

```text
sGamePad+0x970 = mStartPadNo
```

en:

```text
0x02DB0D25
0x02DB0D2F
```

Ambas direcciones forman parte de `SITES` en:

```text
patches/build_v7_fullpad.py
```

v7 las sustituye por lecturas de:

```text
[ebp+8]
```

Por tanto la consulta principal de `WaitState_PL` ya queda:

```text
P1   -> PadData[0]
Sub0 -> PadData[1]
```

en la línea canónica v13.

## 18. 0x02DB0DC0 sigue anclada a mStartPadNo, pero está en una rama Coop-only

Stock `0x02DB0DC0` tiene el mismo ABI, pero conserva:

```asm
0x02DB0DF5  mov edx,[ecx+0x970]
0x02DB0DFF  mov ecx,[eax+0x970]
```

Esos dos sitios no aparecen en el parche v7.

Sin embargo, el callsite `0x025747E2` solo se alcanza después de exigir simultáneamente:

```text
cSystemData<Game>::mGameMode == Coop
actor ThinkMode == Pad
timer/estado local válido
actor asociado al mismo objeto esperado
```

La comprobación de GameMode es:

```text
0x02574768 -> 0x01C076CF
```

La línea canónica local-coop v13 **no cambia mGameMode globalmente**; sigue en Campaign.

Consecuencia:

- el agujero de selector en `0x02DB0DC0` existe en stock PC;
- pero esa llamada concreta no se ejecuta en la ruta local-coop Campaign actual;
- no demuestra por sí sola un bloqueo de puerta local;
- no se parcheará todavía.

La consulta realmente activa para la interacción general de `WaitState_PL` es `0x02DB0CF0`, ya cubierta por v7.

## 19. Estado de v14

Todavía **no** se crea v14.

Para justificarlo falta demostrar que alguna lógica necesaria en local-coop:

1. queda detrás de `GameMode::Coop` y debe replicarse explícitamente con `gLocalCoopActive`; o
2. usa una API de input no restaurada en una ruta que sí se ejecuta en Campaign local.

Siguiente paso: reconstruir la semántica de la rama Coop-only de `WaitState_PL` y auditar `cDoor2pBaseCancelState_PL`/transiciones asociadas.
