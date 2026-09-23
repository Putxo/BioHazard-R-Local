# 04 — Mapa técnico actual

Build de referencia: **BioRevHD 30-Enero-2013.exe**.

## Arquitectura de trabajo

```text
sPcsManager
├── Main  -> uPcsPlayerMain
├── Sub0  -> uPcsPlayerSub0
└── Sub1  -> uPcsPlayerSub1

sGamePad
├── mStartPadNo @ +0x970
└── PadData / estructuras de entrada

uCameraManage
├── mCameraIdx
├── mTargetNo
└── mPadNo @ +0x88

sCamera
└── Viewport[0..7]
```

## 1. sPcsManager

Slots confirmados:

```text
0 = Main Player
1 = Sub Player 0
2 = Sub Player 1
```

Campos debug:

```text
+0x1308 mIsDebug
+0x130C mDebugPlayerMainID
+0x1310 mDebugPlayerSub0ID
+0x1314 mDebugPlayerSub1ID
```

Rutina común tras cambiar IDs:

```text
0x01BC1F58 -> 0x02DD16F0
```

La rutina procesa los tres slots consecutivamente.

## 2. Clases de jugador

RTTI identificado:

```text
app::game::pcs::uPcsPlayerMain
app::game::pcs::uPcsPlayerSub0
app::game::pcs::uPcsPlayerSub1
```

Esto convierte la investigación actual en un problema de **reactivar/conectar clases ya existentes**, no de inventar un segundo jugador desde cero.

## 3. Entrada

`sGamePad::mStartPadNo`:

```text
sGamePad + 0x970
default = 0
```

Regiones observadas:

```text
+0x668
+0x7E8
```

separadas por `0x180`, con construcción de dos elementos de `0xC0`.

Interpretación actual: candidatos a bloques `PadData`, todavía pendiente de reconstrucción completa.

Objetivo:

```text
uPcsPlayerMain -> PadData 0
uPcsPlayerSub0 -> PadData 1
```

sin cambiar globalmente el pad principal.

## 4. Movimiento PCS

Cadenas/rutas activas:

```text
mMovePcs
mMoveSubPcs
```

Pendiente determinar si controlan:

- movimiento real;
- selección del PCS activo;
- modo debug;
- o exposición de estado en herramientas internas.

## 5. Cámara

`uCameraManage` contiene:

```text
mCameraIdx
mTargetNo
mPadNo
```

El campo identificado `mPadNo` está en:

```text
+0x88
```

y no pertenece a `uPlayer`.

Cadena encontrada:

```text
uCameraManage::setCameraIdx() : uPlayer::switchCamera()
```

Esto confirma una relación directa entre el gestor de cámara y operaciones del jugador.

## 6. Viewports

`sCamera` contiene infraestructura:

```text
VIEW_0
VIEW_1
VIEW_2
VIEW_3
VIEW_4
VIEW_5
VIEW_6
VIEW_7
```

El análisis del constructor indica objetos Viewport reales, de tamaño aproximado `0x190`.

Objetivo futuro:

```text
Viewport A -> Camera Main
Viewport B -> Camera Sub0
```

con regiones separadas de pantalla.

## 7. Rutas no válidas como solución principal

```text
mCameraList[0]/[1] -> uObjModel
mPadViewportNo     -> sVibration
```

No volver a tratarlas como hooks P1/P2 salvo nueva evidencia.

## 8. Orden de implementación

```text
1. Main/Sub0 existen simultáneamente
2. input independiente
3. camera independiente
4. segundo viewport
5. compatibilidad de gameplay
```

No se mezclará pantalla partida con input antes de demostrar que Sub0 puede recibir un segundo mando de forma aislada.


---

### Corrección de la salida stock de cámaras

`sGameCamera+0xCE0` y `sGameCamera+0xCE4` son dos `uCameraManage` reales correspondientes a Self y Partner.

La inicialización stock **no** los dibuja simultáneamente:

```text
Self View / Partner View
        |
        v
seleccionan uno de los dos uCameraManage
        |
        v
VIEW_0
```

Los callbacks debug llaman a `0x0203E8A0`, que activa el manager seleccionado, desactiva el otro y enlaza el seleccionado a VIEW_0 usando el helper nativo `0x01EBD610`.

VIEW_4 no contiene el Partner Manager: recibe un tercer objeto de cámara de `0xB0` bytes creado durante la inicialización, asociado a la salida debug/free-view.

La estrategia actual de pantalla partida es:

```text
Self uCameraManage    -> VIEW_0 -> TOP(2)    -> display 0
Partner uCameraManage -> VIEW_1 -> BOTTOM(3) -> display 0
```

activando ambos managers simultáneamente y enlazándolos mediante el helper nativo.

`uCameraManage::mPadNo +0x88` ya no forma parte del parche nuevo: el nombre parecía prometedor, pero todavía no existe evidencia de que seleccione `sGamePad` o un dispositivo físico.


---

## 9. Cámara Self/Partner confirmada

`sGameCamera` contiene dos punteros `uCameraManage` reales:

```text
+0xCE0 Self
+0xCE4 Partner
```

Los callbacks debug `Self View` y `Partner View` usan esos punteros y convergen en `0x0203E8A0`.

La función stock hace que solo uno esté activo a la vez y lo enlaza a VIEW_0.

Para cooperativo local, v4 modifica únicamente esa política:

```text
Self    -> activo -> VIEW_0 -> TOP    -> display 0
Partner -> activo -> VIEW_1 -> BOTTOM -> display 0
```

Se usa el helper nativo de enlace de cámara a viewport:

```text
0x01C34D5A
```

y las mismas funciones de preparación/activación usadas por Capcom:

```text
check ready  0x01B7BA8A
init mode 13 0x01B8AAEE
activate     0x01BF353F
```

VIEW_4 queda intacto porque pertenece a una tercera cámara/debug.

`uCameraManage::mPadNo +0x88` queda fuera del parche actual: no se ha demostrado que seleccione el dispositivo físico.
