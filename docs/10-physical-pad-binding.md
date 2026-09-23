# 10 — Asociación física Pad[0] / Pad[1] en sPad

Build: **BioRevHD 30-Enero-2013.exe**.

## Conclusión

**CONFIRMADO estáticamente:** la capa `sPad` inicializa dos pads lógicos, `Pad[0]` y `Pad[1]`, y para cada uno busca/asigna un socket DirectInput entre dos posibles dispositivos. Por tanto la arquitectura PC sí contempla dos controladores físicos independientes.

## Inicialización de dos pads lógicos

En `0x0335C22B..0x0335C260` aparece un bucle:

```text
index = 0
while (index < 2) {
    configurePad(index, 0x18)
    index++
}
```

La implementación por pad es `0x0335C5C0` y valida `index <= 1`. Para índices mayores imprime:

```text
ERROR: Pad[v%d] can't use.
```

Cada pad lógico usa una estructura de trabajo de `0x2F8` bytes:

```text
sPad + 0x3C + logical_index * 0x2F8
```

Campos observados:

```text
+0x06 = número de Pad lógico
+0x10 = socket/dispositivo físico seleccionado
```

## Selección de socket

La función recorre exactamente dos sockets, 0 y 1, y escoge uno disponible. El debug interno contiene:

```text
Work[%d] Pad[%d] Socket[%d]
```

## Inicialización DirectInput

Ruta:

```text
0x0335C7B1
 -> thunk 0x01B7BF80
 -> 0x0335E2C0
```

`0x0335E2C0` enumera dispositivos DirectInput con callback:

```text
0x01C477AC -> 0x0335ED30
```

El callback usa `context+0x10` como índice de socket y crea/guarda el dispositivo en:

```text
sPad + 0x63C + socket_index*4
```

Antes recorre los dos sockets y evita duplicar un dispositivo ya adjunto.

Strings del propio ejecutable:

```text
New JoyPad controller[p%d] is found.
Already controller[p%d] attached.
Device is created.
```

## Relación con sGamePad

Ya estaba confirmado:

```text
sPad::Pad[0]
sPad::Pad[1]
PadData[0]
PadData[1]
```

Con este análisis queda enlazada también la capa física:

```text
dispositivo DirectInput
 -> socket 0/1
 -> Pad lógico 0/1
 -> sPad::Pad[index]
 -> PadData[index]
 -> sGamePad(selector)
```

## Consecuencia para v9

La cadena de P2 queda soportada estáticamente:

```text
segundo pad lógico
 -> segundo socket/dispositivo DirectInput
 -> PadData[1]
 -> APIs sGamePad(selector=1)
 -> uNpc exacto de Sub0 en ThinkMode::Pad
```

La prueba runtime sigue siendo útil para detectar peculiaridades de drivers/modelos concretos, pero ya no es necesaria para demostrar que el motor tiene dos slots físicos independientes.
