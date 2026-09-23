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
