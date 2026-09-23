# 02 — Diario completo de investigación de esta conversación

> Este documento registra únicamente el trabajo realizado en este chat, incluidos los caminos que parecían prometedores y después se descartaron.

## Convenciones

- **CONFIRMADO**: observado directamente en los binarios de este chat.
- **HIPÓTESIS**: interpretación todavía pendiente de prueba funcional.
- **DESCARTADO**: pista investigada que no sirve para la función que inicialmente parecía tener.
- **CORRECCIÓN**: interpretación anterior refinada o corregida durante este mismo chat.

Las direcciones indicadas pertenecen a **BioRevHD 30-Enero-2013.exe** salvo que se especifique otra build.

---

# 1. Triaje inicial de las cuatro builds

Las cuatro copias son PE32 de 32 bits.

La primera diferencia importante fue el tamaño:

```text
30-Enero-2013   ~60.7 MB
23-Feb-2013     ~16.1 MB
17-May-2013     ~16.7 MB
07-Feb-2024     ~14.3 MB
```

La ruta CodeView de enero contiene:

```text
FullDebugWin32
```

mientras febrero/mayo/retail son ROMRelease/MasterRelease.

**Conclusión:** enero pasa a ser la base principal para descubrir infraestructura eliminada posteriormente.

---

# 2. Primeras búsquedas: P2, pads, cámaras y cooperativo

En la build de enero aparecieron strings como:

```text
Pad2
X(2P)
for2P
Player 2
uPcsActor<Player2>
Coop
Raid
mCameraIdx
VIEW_0 ... VIEW_7
```

También aparecieron opciones internas de Raid:

```text
CreateRaidPlayer 1
CreateRaidPlayer 2
CreateRaidPlayer 3
CreateRaidPlayer 4
CreateRaidPlayer 5
UseOtomo
ForceTwoPlatoon
PickingCoopLock
NotSendPad
mSelfID
mPartnerID
```

La combinación sugería que la build FullDebug conservaba rutas internas para crear y gestionar varios participantes.

---

# 3. `CreateRaidPlayer`: enero frente a febrero

## Enero

La cadena `CreateRaidPlayer 2` está registrada en un menú de depuración y enlazada con código ejecutable real.

Ruta aproximada:

```text
menu registration
  -> thunk/delegate ~0x01C3D0EA
  -> implementation ~0x01D4B160
```

Los números 1/2/3/... se traducen en índices consecutivos que llegan a la implementación común.

**CONFIRMADO:** no son simples strings huérfanos.

## Febrero

En la build del 23-Feb-2013 el menú todavía existe, pero al seguir el callback se llega a:

```text
wrapper ~0x0041ED40
  -> target 0x0079C2D0
     -> ret 4
```

**CORRECCIÓN durante el chat:** inicialmente se resumió como “el callback es un stub”. La formulación exacta es que el wrapper sigue existiendo, pero el destino funcional final fue sustituido por un stub.

**Consecuencia:** enero conserva lógica que febrero ya perdió.

---

# 4. Infraestructura de cámaras y viewports

Se localizaron:

```text
VIEW_0
VIEW_1
...
VIEW_7
```

El constructor de `sCamera` crea/reserva varios objetos Viewport reales.

Durante el análisis se estimó un tamaño aproximado de:

```text
0x190 bytes por Viewport
```

y se observaron conceptos de:

- región;
- cámara;
- visibilidad;
- prioridad.

**HIPÓTESIS fuerte:** el motor ya puede manejar varias vistas simultáneas y no parece limitado a una única cámara renderizada.

---

# 5. Pista descartada: `mCameraList[0]/[1]`

Al principio aparecieron referencias directas a:

```text
mCameraList[0]
mCameraList[1]
```

Parecía una pareja natural P1/P2.

Al seguir los xrefs, esas referencias quedaron asociadas a infraestructura de **`uObjModel`**, no a una lista global inequívoca de cámaras de jugadores.

**DESCARTADO como hook principal de pantalla partida.**

Se conserva en el diario porque fue una pista razonable pero incorrecta para el objetivo.

---

# 6. Pista descartada: `mPadViewportNo`

El nombre parecía describir exactamente un vínculo:

```text
pad -> viewport
```

Sin embargo, el contexto mostró:

```text
mPadVibrationEnable
mCameraVibrationEnable
mPadViewportNo
mVibPad
mVibCamera
sVibration.cpp
```

**DESCARTADO como selector general P1/P2.**

Pertenece al subsistema de vibración y no debe usarse como prueba de la arquitectura principal de control/render.

---

# 7. Ruta de input del jugador

Se identificó una ruta que alimenta estado de gameplay con conceptos equivalentes a:

```text
mMoveAnalog
mRotateAnalog
mAimAnalog
run
aim
```

La observación inicial fue que el jugador base termina usando el pad lógico 0.

La hipótesis de trabajo pasó a ser:

```text
P1 -> pad lógico 0
P2 -> pad lógico 1
```

sin duplicar toda la lógica de movimiento.

---

# 8. `mPadNo +0x88`: interpretación inicial y corrección

String:

```text
mPadNo
raw 0x03163824
VA  0x04CDC824
```

Se observó un registro de propiedad asociado a:

```text
this + 0x88
```

## Interpretación inicial

Se creyó provisionalmente que podía ser:

```text
uPlayer::mPadNo
```

## Nueva evidencia

En el mismo bloque de propiedades aparecieron:

```text
mCameraIdx
mTargetNo
mPadNo
uCameraManage
```

y el constructor asociado alrededor de:

```text
0x0206A070
```

inicializa el campo `+0x88`.

Además se encontró la cadena:

```text
uCameraManage::setCameraIdx() : uPlayer::switchCamera()
```

## Corrección

**CONFIRMADO:** el `mPadNo` identificado en `+0x88` pertenece a **`uCameraManage`**, no debe etiquetarse como `uPlayer::mPadNo`.

Esto evitó continuar por una ruta de parche incorrecta.

---

# 9. `uCameraManage`

Campos/properties relevantes encontrados:

```text
mCameraIdx
mTargetNo
mPadNo
```

Constructor aproximado:

```text
0x0206A070
```

Inicializaciones observadas:

```text
+0x70 = -1
+0x74 = 0
+0x78 = 0
+0x7C = -1
+0x80 = 0
+0x84 = -1
+0x88 = -1
+0x8C = ...
```

La relación entre CameraManage y el jugador es real, pero todavía está pendiente demostrar si cada jugador posee un CameraManage completamente independiente en gameplay.

---

# 10. Nueva pista de input: `mStartPadNo`

String:

```text
mStartPadNo
VA 0x04E11DD0
```

Al seguir su contexto se identificó inequívocamente la clase:

```text
sGamePad
mIsUsePadEx
mStartPadNo
ExPadData
sGamePad::PadData
sgamepadpc.cpp
```

Constructor:

```text
~0x02DAC0F0
```

Campo confirmado:

```text
sGamePad + 0x970 = mStartPadNo
```

El constructor lo inicializa a 0.

También se observaron regiones de datos desde:

```text
sGamePad + 0x668
sGamePad + 0x7E8
```

separadas por `0x180`, con construcción de 2 elementos de `0xC0` bytes.

**HIPÓTESIS:** corresponden a estructuras `PadData` de dos dispositivos.

Una ruta alrededor de:

```text
0x02DAE020
```

lee `mStartPadNo` y lo usa en la lectura real del estado del pad.

**Conclusión:** `mStartPadNo` participa realmente en la entrada, pero parece ser una propiedad global de `sGamePad`, por lo que cambiarla no basta para asignar P2.

---

# 11. Hallazgo importante: `sPcsManager`

Junto a `sGamePad` apareció un bloque de strings explícito:

```text
Main Player
Sub Player 0
Sub Player 1
uPCS
mMoveSubPcs
mMovePcs
Use Pcs Buffer
mDebugPlayerSub1ID
mDebugPlayerSub0ID
mDebugPlayerMainID
mIsDebug
sPcsManager::uPcsFsm
sPcsManager
```

En `.rdata` hay un enum real:

```text
Main Player  -> 0
Sub Player 0 -> 1
Sub Player 1 -> 2
```

Esto cambió la prioridad de la investigación: ya no era necesario depender exclusivamente de `CreateRaidPlayer`.

---

# 12. Offsets de Main/Sub en `sPcsManager`

Se siguieron los getters/setters del bloque debug.

Campos confirmados:

```text
sPcsManager + 0x1308 : mIsDebug
sPcsManager + 0x130C : mDebugPlayerMainID
sPcsManager + 0x1310 : mDebugPlayerSub0ID
sPcsManager + 0x1314 : mDebugPlayerSub1ID
```

Setters aproximados:

```text
mIsDebug            ~0x02DD1460
MainID              ~0x02DD14C0
Sub0ID              ~0x02DD1520
Sub1ID              ~0x02DD1580
```

Getters:

```text
MainID getter  0x02DCDF10
Sub0ID getter  0x02DCDF50
Sub1ID getter  0x02DCDF90
```

Los tres setters de ID llaman a una rutina común:

```text
thunk 0x01BC1F58
 -> 0x02DD16F0
```

Esta rutina:

1. comprueba `mIsDebug`;
2. recorre una colección de hasta 64 elementos;
3. para cada elemento válido itera exactamente 3 veces;
4. lee consecutivamente Main/Sub0/Sub1 desde `+0x130C`;
5. propaga el ID a un objeto asociado.

**CONFIRMADO:** los tres slots se consumen juntos en código real.

---

# 13. `mMovePcs` y `mMoveSubPcs`

Se localizaron xrefs para:

```text
mMovePcs
mMoveSubPcs
```

Rutas aproximadas:

```text
mMovePcs    -> ~0x02DCB123 / ~0x02DCB2A1
mMoveSubPcs -> ~0x02DCB185 / ~0x02DCB2ED
```

y callbacks/getters en la zona:

```text
~0x02DCB370 / ~0x02DCB820
~0x02DCB3B0 / ~0x02DCB8C0
```

**HIPÓTESIS activa:** pueden formar parte del mecanismo que distingue movimiento del jugador principal y los subjugadores.

Todavía no se ha confirmado que equivalgan a “habilitar control local del P2”.

---

# 14. Hallazgo más reciente: clases C++ Main/Sub distintas

Al seguir RTTI y los objetos globales usados por `sPcsManager`, se identificaron clases diferenciadas:

```text
app::game::pcs::uPcsPlayerMain
app::game::pcs::uPcsPlayerSub0
app::game::pcs::uPcsPlayerSub1
```

**CONFIRMADO:** Main/Sub0/Sub1 no son únicamente nombres de menú ni tres IDs dentro del mismo tipo genérico; existen clases C++ diferenciadas en el ejecutable.

Esto es actualmente la evidencia más fuerte de que la arquitectura del juego ya contempla varios roles de jugador dentro del mismo proceso.

---

# 15. Estado actual

La investigación queda dividida así:

## Entidad

```text
uPcsPlayerMain
uPcsPlayerSub0
uPcsPlayerSub1
```

Prioridad: reconstruir herencia, constructores, vtables y relación con `sPcsManager`.

## Input

```text
sGamePad
mStartPadNo
PadData[?]
mMovePcs
mMoveSubPcs
```

Prioridad: encontrar cómo Main/Sub seleccionan una entrada concreta.

## Cámara

```text
uCameraManage
mPadNo
mCameraIdx
mTargetNo
sCamera::Viewport
VIEW_0..VIEW_7
```

Prioridad posterior: enlazar un CameraManage distinto con Sub0 y un segundo viewport.

---

# 16. Próximos pasos

1. reconstruir RTTI/herencia de `uPcsPlayerMain/Sub0/Sub1`;
2. localizar constructores y métodos virtuales específicos de Sub0;
3. identificar desde esos métodos la ruta de lectura de pad;
4. comprobar si `sGamePad::PadData` se indexa por jugador o por pad;
5. demostrar primero **input independiente** sin pantalla partida;
6. después enlazar una cámara independiente;
7. finalmente activar un segundo viewport.

No se considera resuelto ningún paso solo porque aparezca un string o un segundo modelo; cada capa se validará por separado.


---

# 17. Hipótesis intermedia: febrero como target por XInput

Antes de seguir `CreateRaidPlayer` hasta su implementación final hubo una hipótesis temporal:

```text
Enero  -> usarlo como mapa FullDebug
Febrero -> usarlo como target práctico porque ya importa XINPUT1_3.dll
```

La idea era razonable porque enero no muestra `XINPUT1_3.dll` en su tabla normal de imports, mientras febrero/mayo/retail sí.

Después de seguir `CreateRaidPlayer 2`, la hipótesis se corrigió:

- febrero conserva el menú/wrapper;
- el destino funcional investigado termina en `0x0079C2D0: ret 4`;
- enero conserva implementación real.

**CORRECCIÓN:** enero pasa a ser también la base experimental principal, no solo el mapa de símbolos/debug.

Se mantiene este cambio de criterio en el diario para que no parezca que enero fue elegido desde el principio sin alternativas.

---

# 18. Experimento estático P2 INPUT

Durante esta conversación se generó:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT EXPERIMENTAL.exe`

SHA-256:

`6c55444a820d05d0b0a15cafad0e8e7d0ec5bc67c1a548631a47d9ef2e6f0729`

Objetivo: comprobar estáticamente si modos internos secundarios podían reentrar en la ruta local de input y seleccionar pad lógico 1.

Puntos modificados:

```text
0x27A0CE7 -> redirect a 0x27A1318
0x27A1318 -> stub de decisión de modo
0x27A1340 -> selector pad 0/1

call sites:
0x27A0D03
0x27A0D49
0x27A0D6D
0x27A0DBF
0x27A0E29
0x27A0E8A
```

Diff contra el original:

- mismo tamaño;
- 79 bytes distintos;
- 10 rangos contiguos.

**Estado:** solo verificación estática. No hubo ejecución del juego, por lo que no se etiquetó como “P2 funcional”.

El detalle completo se conserva en `docs/06-experiments.md` y `research/manifests/p2-input-experimental.json`.

---

# 19. Experimento estático SPLITSCREEN v2

Después de localizar Self/Partner CameraManage, `mPadNo` y varios Viewports, se generó:

`BioRevHD 30-Enero-2013 LOCAL COOP SPLITSCREEN EXPERIMENTAL v2.exe`

SHA-256:

`9952e14daedfa05afceea6e1456f7d18aa1ed071ea900c7aef2cc7c8eeeec089`

Incluye el experimento de input anterior y añade una prueba estática de:

```text
Self CameraManage    -> mPadNo 0
Partner CameraManage -> mPadNo 1

Viewport 0 -> Self    / TOP
Viewport 1 -> Partner / BOTTOM
```

Hook/cave documentados:

```text
0x203E3A9 -> 0x203E3BD
cave 0x203E3BD..0x203E44B
activate impl ~0x2069AB0
```

Diff total contra el original:

- 242 bytes diferentes;
- 11 rangos;
- 163 bytes nuevos de cambios de cámara/viewport + los 79 bytes del experimento de input.

**Estado:** solo verificación estática.

No demuestra actor P2, cámara Partner válida, HUD dual ni estabilidad de escenas.

---

# 20. Pista comparativa: Revelations 2 / Fluffy Manager

Se consideró como referencia conceptual que herramientas/mods de Resident Evil Revelations 2 habían trabajado con cooperativo local sobre la misma familia tecnológica MT Framework.

La intención era comprobar si podía sugerir patrones de separación de pad/cámara/viewport.

**Resultado de esta conversación:** no se usó ningún offset, binario ni implementación de Revelations 2 como evidencia para Revelations 1.

Se conserva como pista comparativa descartada para evitar que en el futuro parezca una dependencia oculta del parche.

---

# 21. Fallo de herramienta: `pefile`

Se intentó usar Python con:

```python
import pefile
```

El entorno respondió:

```text
ModuleNotFoundError: No module named 'pefile'
```

No se convirtió en dependencia del proyecto.

El trabajo siguió con:

- `objdump`;
- `strings`;
- `file`;
- `sha256sum`;
- Python sin dependencias externas para búsqueda/diff de bytes.

Los scripts reproducibles añadidos al repositorio siguen ese mismo criterio.

---

# 22. Verificación byte a byte de los experimentos

Los dos experimentos fueron comparados directamente contra el original de enero.

## P2 INPUT

```text
79 bytes diferentes
10 rangos
tamaño final = tamaño original
```

## SPLITSCREEN v2

```text
242 bytes diferentes
11 rangos
tamaño final = tamaño original
```

Esto confirma que fueron parches estrechos sobre la misma imagen PE, no una reconstrucción accidental o una sustitución masiva del binario.

No implica validación funcional.

---

# 23. Petición de trazabilidad en GitHub

El usuario pidió subir **todo el proceso de esta investigación** a:

`Putxo/BioHazard-R-Local`

Se verificó durante esta conversación que:

- el repositorio existía;
- estaba inicialmente vacío;
- la rama por defecto era `main`;
- la cuenta conectada tenía permisos de escritura/admin.

El criterio pedido por el usuario se concretó después en:

> solo el progreso de este chat, incluyendo lo que parecía servir y después se descartó.

Por eso el repositorio conserva explícitamente:

- hipótesis fallidas;
- correcciones;
- experimentos no validados;
- limitaciones;
- herramientas fallidas;
- siguiente trabajo pendiente.

---

# 24. Estado técnico actual después de documentar todo lo anterior

La evidencia más fuerte ya no es `CreateRaidPlayer` por sí sola.

La ruta actual prioritaria es:

```text
uPcsPlayerMain
uPcsPlayerSub0
uPcsPlayerSub1
        |
        v
sPcsManager Main/Sub0/Sub1
        |
        v
mMovePcs / mMoveSubPcs
        |
        v
sGamePad::PadData
        |
        v
input independiente
```

Después:

```text
Sub0
 -> uCameraManage independiente
 -> Viewport independiente
 -> split-screen
```

La siguiente confirmación que falta es localizar una diferencia concreta Main/Sub0 que llegue a la selección del `PadData` o equivalente, en lugar de seguir imponiendo el índice de pad mediante una inferencia externa.


---

# 25. Vtables Main/Sub y especializaciones uPcsActor

Se reconstruyeron las vtables de:

```text
uPcsPlayerMain  0x04E1642C
uPcsPlayerSub0  0x04E1649C
uPcsPlayerSub1  0x04E1650C
```

Base RTTI:

```text
uPcsChara
vtable 0x04E1631C
```

Solo difieren 3 de 21 slots. El slot funcional más importante fija rol 0/1/2 y consulta `sPcsManager`.

También se identificaron:

```text
uPcsActor<Player0> vtable 0x04E166CC
uPcsActor<Player1> vtable 0x04E1673C
uPcsActor<Player2> vtable 0x04E167AC
uPcsActor<SubPlayer0>
uPcsActor<SubPlayer1>
```

Las especializaciones Player0/1/2 repiten la separación fija de rol 0/1/2.

Detalle: `docs/07-player-roles-and-pad-routing.md`.

---

# 26. Corrección de mMovePcs / mMoveSubPcs

Las strings japonesas adyacentes muestran:

```text
PCS番号      = número de PCS
サブPCS番号  = número de sub-PCS
```

Por tanto `mMovePcs` y `mMoveSubPcs` dejan de tratarse como candidatos de input/movimiento. Son controles de selección/número de PCS.

---

# 27. getPadNo de uPlayer devuelve siempre 0

Thunk:

```text
0x01C6C746 -> 0x027A27B0
```

La implementación hace únicamente:

```asm
xor eax,eax
ret
```

El thunk tiene al menos 129 call sites en gameplay.

La zona pertenece a `uplayer.cpp`.

---

# 28. sGamePad recibe pad number pero fuerza mStartPadNo

Se siguieron varias llamadas que hacen:

```text
uPlayer -> getPadNo -> argumento a sGamePad
```

Sin embargo métodos de `sGamePad` como:

```text
0x02DB1570
0x02DB1910
0x02DB1C90
0x02DAF000
```

ignoran el selector recibido y leen `this+0x970` (`mStartPadNo`).

Al mismo tiempo `sGamePad` sí posee exactamente dos bloques `PadData` y un operador indexado con límite 2.

Nueva conclusión:

> el backend de input ya es 2-pad, pero la capa PC de gameplay está forzada al pad inicial/global.

Esto redefine la solución de input P2: hay que restaurar tanto la identidad 0/1 de cada uPlayer como el respeto del parámetro por sGamePad.


---

# 29. ThinkMode identificado: Pad / Network / Cpu / Invalid

La rama 1/2/3 de la actualización de input ya no se trata como un enum desconocido.

La build contiene explícitamente:

```text
ThinkMode::Pad
ThinkMode::Network
ThinkMode::Cpu
ThinkMode::Invalid
```

y el flujo ejecutable establece:

```text
0 Invalid
1 Pad
2 Network
3 Cpu
```

La rama 2 consume `cPlayerPadSyncData`, confirmando que es input sincronizado/remoto.

---

# 30. uCallThink::mThinkMode confirmado

El objeto instalado en `cCharaStateManager+0x524` es `uCallThink`.

Su metadata registra directamente:

```text
uCallThink+0x34 = mThinkMode
```

El getter usado por uPlayer termina leyendo exactamente ese campo.

---

# 31. uPlayer+0xE40 y setter de ThinkMode

El constructor de uPlayer inicializa:

```text
uPlayer+0xE40 = 2 (Network)
```

La ruta:

```text
0x01BE3257 -> 0x027F1290
```

actúa como setter de ThinkMode:

- propaga el modo a cCharaStateManager/uCallThink;
- actualiza `uPlayer+0xE40`.

La entrada virtual de vtable `+0x110` conduce mediante `0x01BB8B60 -> 0x0278CC40` a la propagación de este estado.

---

# 32. Corrección del experimento P2 INPUT

El primer experimento forzaba modos 2/3 a la rama local.

Ahora queda corregido:

```text
2 = Network
3 = Cpu
```

Por tanto el enfoque correcto para P2 local es poner Sub0/Player1 en:

```text
ThinkMode::Pad = 1
```

y asignarle pad 1, no reutilizar Network/Cpu como señal de P2.

Detalle completo: `docs/08-thinkmode-local-input.md`.
