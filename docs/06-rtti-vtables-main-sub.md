# 06 — RTTI, herencia y vtables de Main/Sub

Build: `BioRevHD 30-Enero-2013.exe`.

Este documento recoge la reconstrucción RTTI realizada en esta conversación.

## 1. Herencia de las tres clases de jugador

MSVC RTTI confirma la misma cadena de herencia para las tres:

```text
uPcsPlayerMain
└── uPcsChara
    └── uPcs
        └── cUnit
            └── MtObject

uPcsPlayerSub0
└── uPcsChara
    └── uPcs
        └── cUnit
            └── MtObject

uPcsPlayerSub1
└── uPcsChara
    └── uPcs
        └── cUnit
            └── MtObject
```

TypeDescriptor:

```text
uPcsPlayerMain  0x054E20FC
uPcsPlayerSub0  0x054E2130
uPcsPlayerSub1  0x054E2164
```

CompleteObjectLocator:

```text
Main  0x0538C48C
Sub0  0x0538C4F8
Sub1  0x0538C564
```

Vftables:

```text
Main  0x04E1642C
Sub0  0x04E1649C
Sub1  0x04E1650C
```

Cada vtable contiene 21 entradas antes del bloque nulo/separador.

## 2. Solo tres slots cambian entre Main/Sub0/Sub1

Comparando las 21 entradas, únicamente difieren:

```text
slot 0
slot 4
slot 5
```

Los demás slots son compartidos.

### Slot 0

Es la ruta de destrucción específica de cada clase.

Thunks:

```text
Main  0x01C2AF1C -> 0x02DF5900
Sub0  0x01B9F340 -> 0x02DF5F20
Sub1  0x01BEA340 -> 0x02DF6540
```

### Slot 4

Devuelve un objeto global específico de la clase, probablemente relacionado con DTI/tipo.

```text
Main  -> 0x0559A34C
Sub0  -> 0x0559A2CC
Sub1  -> 0x0559A30C
```

No se le asigna todavía un nombre semántico definitivo.

### Slot 5 — hallazgo importante

Rutas reales:

```text
Main  -> 0x02DF5D60
Sub0  -> 0x02DF6380
Sub1  -> 0x02DF69A0
```

Las tres:

1. llaman al comportamiento base de `uPcsChara`;
2. toman `[this+0x30]`;
3. consultan `sPcsManager`;
4. usan un índice de rol distinto;
5. guardan el ID devuelto mediante la misma función.

La diferencia crítica es:

```asm
; Main
push 0

; Sub0
push 1

; Sub1
push 2
```

Por tanto, el valor 0/1/2 no es una interpretación externa: las propias clases lo codifican.

## 3. Función de sPcsManager consumida por el slot 5

La llamada acaba en:

```text
0x02DD0080
```

La función recibe:

- un valor procedente de `[player+0x30]`;
- un índice de rol 0..2.

Si `sPcsManager::mIsDebug` está activo:

```text
role 0 -> [manager+0x130C]
role 1 -> [manager+0x1310]
role 2 -> [manager+0x1314]
```

Si no está en debug:

- recorre hasta 20 entradas;
- compara el primer argumento con una tabla desde `manager+0x2C`;
- al encontrar coincidencia usa una tabla de tres DWORD por entrada desde `manager+0x7C`;
- selecciona el DWORD correspondiente a Main/Sub0/Sub1.

Esto confirma que `sPcsManager` mantiene una tabla normal de tres IDs por entrada además de los tres IDs debug.

## 4. Constructores de las clases de jugador

Cada constructor llama a la misma base y después instala su vtable.

```text
uPcsPlayerMain ctor  ~0x02DF5860
  vtable = 0x04E1642C

uPcsPlayerSub0 ctor  ~0x02DF5E80
  vtable = 0x04E1649C

uPcsPlayerSub1 ctor  ~0x02DF64A0
  vtable = 0x04E1650C
```

Los tres llaman a la misma rutina base:

```text
0x01C6B648
```

## 5. También existen actores template específicos

RTTI confirma:

```text
uPcsActor<MainPlayer>
uPcsActor<SubPlayer0>
uPcsActor<SubPlayer1>
```

y además existen:

```text
uPcsActor<Player0>
uPcsActor<Player1>
uPcsActor<Player2>
```

Esto amplía la evidencia previa basada solo en el string `uPcsActor<Player2>`.

### RTTI/vtables de actores Main/Sub

```text
uPcsActor<MainPlayer>  vtable 0x04E1657C
uPcsActor<SubPlayer0>  vtable 0x04E165EC
uPcsActor<SubPlayer1>  vtable 0x04E1665C
```

Herencia:

```text
uPcsActor<...>
└── uPcsChara
    └── uPcs
        └── cUnit
            └── MtObject
```

Constructores:

```text
MainPlayer actor  ~0x02DF6AC0
SubPlayer0 actor  ~0x02DF6FD0
SubPlayer1 actor  ~0x02DF74E0
```

## 6. El mismo patrón 0/1/2 aparece en los actores

El slot 5 de los actores Main/Sub también codifica:

```text
MainPlayer  -> push 0
SubPlayer0  -> push 1
SubPlayer1  -> push 2
```

Rutas:

```text
MainPlayer  0x02DFCA50
SubPlayer0  0x02DFCB30
SubPlayer1  0x02DFCC10
```

Esto confirma que la distinción Main/Sub no depende de una sola clase concreta: está integrada tanto en las clases `uPcsPlayer*` como en las plantillas `uPcsActor<...>`.

## 7. Consecuencia para el cooperativo local

La arquitectura ya tiene, de forma nativa:

```text
rol 0 = Main
rol 1 = Sub0
rol 2 = Sub1
```

y rutas específicas que registran cada objeto con su rol correspondiente.

La prioridad deja de ser “fabricar P2” y pasa a ser:

1. averiguar cuándo/por qué `SubPlayer0` no recibe control local;
2. localizar la selección de input dentro de Main/Sub;
3. conectar Sub0 al segundo pad;
4. después resolver CameraManage/Viewport.

## 8. Siguiente punto de análisis

Comparar las funciones específicas de Main/Sub posteriores al registro de rol y seguir sus llamadas hasta:

```text
sGamePad::PadData
mMovePcs / mMoveSubPcs
entrada analógica/botones
```

El objetivo inmediato es encontrar una lectura de pad cuyo índice dependa del rol 0/1/2.
