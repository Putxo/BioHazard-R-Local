# 13 — Puertas 2P y estado por participante

Build principal: **BioRevHD 30-Enero-2013.exe**.

Base experimental vigente: **v11 PICKUP**.

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
