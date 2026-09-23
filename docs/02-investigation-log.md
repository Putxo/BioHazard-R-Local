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


---

# 25. Native Input v3: reemplazo del experimento heurístico

Después de identificar de forma consistente:

```text
ThinkMode::Pad     = 1
ThinkMode::Network = 2
ThinkMode::Cpu     = 3
```

y localizar:

- setter nativo de ThinkMode;
- wrapper virtual que sincroniza controladores internos;
- predicado nativo Self/Partner;
- getter stock de pad que devolvía 0;

se construyó una nueva prueba **desde el EXE original de enero**.

Archivo local:

`BioRevHD 30-Enero-2013 LOCAL COOP NATIVE INPUT EXPERIMENTAL v3.exe`

SHA-256:

`9de444e104f2846aa166e1460f33a110ffac5b880f374edfd31b94aef3e5c649`

## Cambios

```text
ThinkMode::Pad     -> local stock
ThinkMode::Network -> network stock
ThinkMode::Cpu + Self -> CPU stock
ThinkMode::Cpu + Partner -> setter nativo Pad -> local stock

Self    -> pad 0
Partner -> pad 1
```

Diff:

```text
73 bytes
3 rangos
mismo tamaño de EXE
```

La prueba anterior que redirigía modos 2/3 queda conservada por trazabilidad, pero **v3 es el diseño estático preferido**.

# 26. Verificación del desensamblado de v3

Se volvió a desmontar el EXE generado.

Se verificó que:

- el hook de `0x027A0CE4` aterriza en la cave;
- las salidas vuelven a las rutas stock correctas;
- `Network` no pasa por el setter Pad;
- Partner+Cpu llama a `0x01BB8B60` con 1;
- el selector de pad llama al predicado `0x01BB3192`;
- Self produce 0 y Partner produce 1.

Esto sigue siendo validación estática, no gameplay.

# 27. Riesgo identificado en v3

La conversión Partner `Cpu -> Pad` ocurre dentro de la ruta de actualización de input.

Por tanto un script que fuerce temporalmente CPU para una escena podría competir con v3.

Este riesgo se considera explícito y deberá resolverse con un gate de estado/escena o un cambio más temprano en la creación/configuración del Partner si las pruebas de gameplay lo confirman.


---

# 27. Corrección crítica: 0x027A0CA0 pertenece a uNpc / uNp<...>

La rutina que se había tratado provisionalmente como una ruta de input de “uPlayer”:

```text
0x027A0CA0
```

fue rastreada hacia atrás.

Su único caller directo localizado es:

```text
0x027885E0
  -> call thunk 0x01C0DFBB
  -> 0x027A0CA0
```

El thunk de `0x027885E0` es `0x01C5519A`.

Buscando esa entrada en vtables con RTTI se comprobó que ocupa el mismo slot virtual (**slot 51**) en:

```text
uNpc
uNp<OBrien>
uNp<Raymond>
uNp<Raymond_Injury>
uNp<Raymond_Aid>
uNp<Morgan>
uNp<Norman>
uNp<Terrorist>
uNp<Terrorist_Norman>
uNp<Terrorist_Rymond>
uNp<Terrorist_ColdRegion>
uNp<Terrorist_Rymond2>
uNp<BSAA_AgentA>
uNp<BSAA_AgentB>
uNp<BSAA_AgentC>
uNp<BSAA_Operator>
uNp<Kirk>
uNp<NSFAgent>
uNp<NSFAgentB>
uNp<Combargno>
uNp<ScientistA>
uNp<ScientistB>
...
```

Ejemplo:

```text
uNp<OBrien> vtable ~0x04D4E2BC
slot 51 -> thunk 0x01C5519A -> 0x027885E0
```

## Consecuencia

La clasificación anterior “rutina que rellena el input de uPlayer” era demasiado fuerte.

**CORRECCIÓN:** `0x027A0CA0` pertenece a la ruta de actualización/control de NPCs/partners.

Esto no vuelve inútil el experimento anterior. Al contrario, puede ser precisamente la ruta necesaria para convertir un acompañante controlado como NPC en un segundo jugador local. Pero ya no debe describirse como input nativo de `uPcsPlayerMain/Sub0`.

## 0x027A27B0

Dentro de esa ruta se llama repetidamente a:

```text
thunk 0x01C6C746 -> 0x027A27B0
```

La implementación original de enero es:

```asm
xor eax,eax
ret
```

Es decir, devuelve siempre **0**.

El experimento anterior sustituyó funcionalmente ese cero por 1 para determinados modos inferidos.

Nueva interpretación prudente:

- existe un selector/índice en la ruta NPC;
- en la build original está fijado a 0;
- aún no se ha demostrado formalmente que su nombre semántico sea “pad number”;
- las llamadas que reciben su retorno terminan en sistemas de control/entrada, por lo que sigue siendo una pista prioritaria.

## Impacto sobre el experimento P2 INPUT

El archivo experimental se mantiene como evidencia histórica, pero su descripción correcta pasa a ser:

> experimento para redirigir la ruta de control/input del NPC/partner a un segundo índice de entrada bajo modos secundarios inferidos.

No debe etiquetarse como parche ya demostrado de `uPlayer P2`.


---

# 28. cPlayerPadSyncData: entrada remota nativa del compañero

Se identificó el tipo RTTI:

```text
app::game::chara::cPlayerPadSyncData
```

con vtable `0x04D98588` y tamaño aproximado `0x28`.

Layout confirmado por metadata y serialización:

```text
+0x08 moveAnalog
+0x10 rotateAnalog
+0x18 aimAnalog
+0x20 waistRotateX
+0x24 isRun
+0x25 isAim
```

Serializer: `0x02747960`.

Deserializer: `0x02747B40`.

Canal de red:

```text
PlayerPad -> 0x332D
```

Un `cNetSyncData<uNpc>` se crea para el owner NPC y su dispatcher entrante `0x0274A680` reconoce el DTI de `cPlayerPadSyncData`.

El callback termina en:

```text
0x027B1D70
```

que copia los campos del paquete remoto a:

```text
uNpc+0x1670 move
uNpc+0x1678 rotate
uNpc+0x1680 aim
uNpc+0x1688 run
uNpc+0x1689 aim flag
```

Son los mismos campos alimentados por la ruta local/AI `0x027A0CA0`.

**CONFIRMADO:** el juego ya posee una ruta completa de control remoto del partner que converge en el mismo estado de control del NPC.

Nueva estrategia prioritaria:

```text
PadData[1] local
 -> cPlayerPadSyncData
 -> handler 0x027B1D70
 -> partner uNpc
```

Detalle completo: `docs/08-player-pad-sync.md`.


---

# 29. Productor nativo de cPlayerPadSyncData

Después de identificar el receptor remoto `0x027B1D70`, se localizó también el productor/sender:

```text
0x0274B200
```

Esta rutina construye un `cPlayerPadSyncData` temporal en la pila y obtiene los seis valores de control desde el `uNpc` owner.

### Campos rellenados

```text
packet +0x08 moveAnalog
  getter thunk 0x01BF5416 -> 0x0208B7E0
  origen uNpc+0x1670

packet +0x10 rotateAnalog
  thunk 0x01C7EB03 -> 0x02703130
  origen uNpc+0x1678

packet +0x18 aimAnalog
  thunk 0x01C6253E -> 0x02728570
  origen uNpc+0x1680

packet +0x20 waistRotateX
  thunk 0x01C3B1AA -> 0x02089BC0
  devuelve uNpc+0x16C0; el sender toma el primer float

packet +0x24 isRun
  thunk 0x01C3DD56 -> 0x0272D790
  origen uNpc+0x1688

packet +0x25 isAim
  thunk 0x01BC4D43 -> 0x0208E9D0
  cálculo basado en el estado de aim del NPC
```

El paquete actual se compara con una copia cacheada dentro de `cNetSyncData`, comenzando alrededor de `+0x70`. Solo se transmite cuando cambia o cuando corresponde un refresco periódico.

Envío observado alrededor de `0x0274B68B`, terminando en:

```text
thunk 0x01BC43D4 -> 0x02DA4C30
```

Después el paquete se copia a la cache mediante:

```text
thunk 0x01BD6A07 -> 0x0274B9A0
```

### Caller desde el control del NPC

El sender tiene un call relevante desde:

```text
0x027A1027
  thunk 0x01BED879
  -> 0x0274B200
```

Ese call está en la misma rutina de control NPC `0x027A0CA0`.

Esto demuestra el pipeline completo:

```text
estado de control del NPC
 -> cPlayerPadSyncData
 -> red
 -> cPlayerPadSyncData remoto
 -> 0x027B1D70
 -> estado de control del NPC remoto
```

---

# 30. Corrección crítica: los getters PC de sGamePad ignoran el selector recibido

La rutina NPC llama a una función selectora:

```text
thunk 0x01C6C746 -> 0x027A27B0
```

En el original:

```asm
xor eax,eax
ret
```

El resultado se pasa como argumento a varias APIs de `sGamePad`.

La hipótesis anterior era que cambiar ese 0 por 1 seleccionaría el segundo pad.

Al desmontar las implementaciones PC se comprobó que **el argumento existe pero se ignora**. Los getters leen en su lugar:

```text
sGamePad::mStartPadNo @ +0x970
```

### Métodos confirmados

```text
move analog
thunk 0x01BC4177 -> 0x02DB1570

aim analog
thunk 0x01BBA276 -> 0x02DB1740

rotate analog
thunk 0x01C39DA0 -> 0x02DB1910

alternate rotate analog
thunk 0x01C41E88 -> 0x02DB1C90

run/action boolean
thunk 0x01BA896D -> 0x02DAFFB0

aim boolean
thunk 0x01C484F9 -> 0x02DAF3C0
```

Los cuatro getters analógicos reciben dos argumentos y retornan con `ret 8`, pero la segunda entrada —el selector pasado por el caller— no se usa para escoger el PadData. Se carga `mStartPadNo` global.

Los dos getters booleanos presentan la misma idea: reciben un selector, pero la implementación termina consultando `mStartPadNo`.

### Consecuencia sobre el experimento P2 INPUT anterior

**CORRECCIÓN:** el parche antiguo que hacía que `0x027A27B0` devolviera 1 para modos secundarios no basta para separar Pad 1/Pad 2 en esta versión PC.

Aunque el flujo de control llegaba a la ruta correcta, las implementaciones PC de `sGamePad` seguían leyendo el pad global inicial.

El experimento se conserva como historial y como prueba de flujo, pero no debe considerarse una separación real de mandos.

### Nueva estrategia de parche

La API ya conserva la interfaz correcta: los getters reciben un selector.

Por tanto la ruta más limpia es restaurar esa semántica:

```text
sGamePad getter(selector)
  usar selector
  en lugar de mStartPadNo global
```

y después:

```text
Main / pad local normal -> selector 0
Partner remoto convertido a local -> selector 1
```

Esto evita cambiar globalmente `mStartPadNo`, lo cual afectaría simultáneamente a ambos jugadores.

---

# 31. Interpretación provisional de los modos NPC

En `0x027A0CA0` el modo se obtiene de un objeto cuyo getter termina leyendo `+0x34`.

Se observó además el nombre debug:

```text
ThinkModes
```

La distribución de comportamiento es:

```text
modo 1:
  calcula move/rotate/aim/run/aim desde sGamePad
  luego puede llamar al sender 0x0274B200

modo 2:
  alimenta los mismos campos desde otra estructura/ruta
  fuerte candidato a AI/otra fuente local

modo 3:
  no recalcula los tres vectores principales
  mantiene estado auxiliar
  encaja con recibir los campos asíncronamente mediante cPlayerPadSyncData
```

**Estado:** modo 1 = local y modo 3 = remoto/network es una hipótesis fuerte por comportamiento, pero todavía no se marca como nombre de enum confirmado hasta recuperar las etiquetas del metadata.


---

# 32. Construcción del experimento P2 INPUT v3

Tras confirmar que la capa PC de `sGamePad` ignora el selector, se construyó un nuevo experimento directamente sobre el EXE original de enero.

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT v3.exe`

SHA-256:

`ee05fc7d6c965aed0661165eb0ead4c6dffa1f67bc32fd758297edb86f3213e1`

La lógica deja el modo 2 completamente intacto.

Solo el modo 3 —fuerte candidato a remoto/network— se redirige a la ruta local, y el selector retorna 1 exclusivamente para ese modo.

Además se restauró el uso del selector en los seis getters de `sGamePad` usados por el control NPC.

Verificación estática:

```text
same file size: yes
different bytes: 111
contiguous diff ranges: 17
```

No se marca como funcional hasta prueba dentro del juego.

Detalle y manifest:

- `docs/06-experiments.md`
- `research/manifests/p2-input-v3.json`


---

# 33. Combinación v3 input + split-screen

Se creó una nueva build experimental combinada:

`BioRevHD 30-Enero-2013 LOCAL COOP v3 INPUT+SPLITSCREEN.exe`

SHA-256:

`6978dc6d63781718c3728160c0312737424536734c0850b9337e990fb2da677d`

La build se genera desde el original de enero, aplica el input v3 corregido y después **solo** el bloque cámara/viewport del split-screen v2.

No hereda los parches de input antiguos.

Verificación:

```text
PE32 sigue válido para objdump
mismo tamaño: sí
274 bytes diferentes
18 rangos
```

Sigue siendo una build experimental sin prueba de gameplay.


---

# 34. Confirmación de dos PadData indexables

El helper de indexación alcanzado desde los getters de `sGamePad` es:

```text
0x01C4F385 -> 0x02DBE720
```

La función valida explícitamente:

```text
index < 2
```

y calcula:

```text
element = base + index * 0xC0
```

**CONFIRMADO:** existen dos entradas seleccionables por el índice que v3 restaura.

Por tanto `selector=1` no es solo un número aceptado por la API: selecciona el segundo elemento de una colección de dos PadData.

Pendiente de runtime: confirmar qué mando físico alimenta cada slot en esta FullDebug PC.


---

# 35. ThinkMode y dos objetos sPad::Pad reales

## Etiquetas internas ThinkMode

La build de enero contiene consecutivamente:

```text
ThinkMode::Network  raw 0x0322AF38
ThinkMode::Cpu      raw 0x0322AF50
ThinkMode::Pad      raw 0x0322AF64
ThinkMode::Invalid  raw 0x0322AF78
```

El flujo ejecutable de `0x027A0CA0` distingue los valores 1, 2 y 3:

- valor 1 entra en la ruta que consulta `sGamePad`;
- valor 2 consume la ruta/estructura de CPU;
- valor 3 deja sin regenerar los vectores principales y encaja con la recepción `cPlayerPadSyncData`.

Con el conjunto completo de etiquetas internas, el mapeo operativo queda:

```text
0 = ThinkMode::Invalid
1 = ThinkMode::Pad
2 = ThinkMode::Cpu
3 = ThinkMode::Network
```

No se ha localizado todavía una tabla enum explícita que contenga nombre+valor en una misma estructura, por lo que la prueba combina las etiquetas internas con las ramas de comportamiento.

## Dos objetos low-level sPad::Pad

Además de los dos `PadData` de stride `0xC0`, `sGamePad` mantiene exactamente dos punteros de objeto low-level:

```text
sGamePad + 0x968 + index*4
index = 0..1
```

El constructor inicializa ambos a null en el bucle:

```text
0x02DAC353..0x02DAC37C
cmp index,2
```

La ruta de creación runtime:

```text
0x02DACA76..0x02DACADC
```

itera dos veces, reserva `0x2F8` bytes, llama al constructor:

```text
0x01C01EAF -> 0x03358F60
```

y almacena cada puntero en:

```text
[this + index*4 + 0x968]
```

El constructor instala la vtable:

```text
0x04EE8E4C
```

El RTTI asociado identifica el tipo:

```text
.?AVPad@sPad@@
```

Por tanto son objetos reales `sPad::Pad`.

## Actualización separada de ambos dispositivos

La actualización alrededor de `0x02DAC73E` recorre de nuevo `index=0..1` y usa:

```text
[this + index*4 + 0x968]
```

Para cada objeto copia estado desde:

```text
sPad::Pad + 0x15C
```

a un buffer separado:

```text
sGamePad + 0x198 + index*0x2F8
```

Más adelante también recorre dos entradas `PadData`:

```text
sGamePad + 0x668 + index*0xC0
```

**CONFIRMADO estáticamente:** la build mantiene y actualiza dos objetos de pad low-level y dos estados indexados de entrada. El slot 1 no es memoria reservada sin uso.

Pendiente de runtime: comprobar exactamente cómo DInput asigna los dos mandos físicos a `sPad::Pad[0]` y `sPad::Pad[1]`.


---

# 36. Corrección crítica del split-screen: mMode no es mRegion

Se reconstruyó el metadata de `sCamera::Viewport`.

El registro de tipo confirma:

```text
sCamera::Viewport
size = 0x190
```

Registro DTI observado alrededor de `0x04B014E0`:

```asm
push 0x190
...
push 0x04EC9284 ; "sCamera::Viewport"
```

El tipo `sCamera` completo se registra con tamaño:

```text
0xCE0
```

## Offsets de Viewport confirmados por property metadata

La rutina alrededor de `0x03288BC0` registra:

```text
+0x04 mpCamera
+0x08 mpTestCamera
+0x0C mpSceneTexture
+0x10 mVisible
+0x11 Scene
+0x12 mPriority
+0x13 mMode
+0x14 mDisplay
+0x18 mRegion
```

La build experimental anterior usaba como primer viewport la base:

```text
sCamera + 0x30
```

y segundo viewport:

```text
sCamera + 0x30 + 0x190 = sCamera + 0x1C0
```

Esto encaja exactamente con sus escrituras:

```text
+0x34  = viewport0.mpCamera
+0x40  = viewport0.mVisible
+0x43  = viewport0.mMode
+0x44  = viewport0.mDisplay

+0x1C4 = viewport1.mpCamera
+0x1D0 = viewport1.mVisible
+0x1D3 = viewport1.mMode
+0x1D4 = viewport1.mDisplay
```

## Error del experimento v2

El parche escribió:

```text
viewport0.mMode = 2
viewport1.mMode = 3
```

y se interpretó erróneamente como:

```text
REGION_TOP / REGION_BOTTOM
```

Pero las etiquetas `REGION_TOP` y `REGION_BOTTOM` pertenecen a **mRegion**, situado en `+0x18`, no a `mMode`.

Por tanto:

**CORRECCIÓN:** las builds split-screen v2 y v3 INPUT+SPLITSCREEN no configuran realmente TOP/BOTTOM mediante esas escrituras.

Siguen siendo útiles como experimentos de asignación de dos cámaras a dos Viewports, pero su parte de geometría de pantalla estaba mal interpretada.

## Próximo parche

El siguiente experimento debe:

1. conservar `mpCamera` y `mVisible`;
2. determinar los valores exactos del enum `mRegion`;
3. escribir:
   - `viewport0.mRegion @ sCamera+0x48`;
   - `viewport1.mRegion @ sCamera+0x1D8`;
4. no reutilizar 2/3 en `mMode` salvo que su semántica se determine por separado.


---

# 37. Rectificación de la corrección anterior: mMode sí contiene REGION_TOP/BOTTOM

La sección 36 registró una corrección provisional: al ver que `mRegion` estaba en `+0x18`, se asumió que los valores TOP/BOTTOM debían escribirse allí.

Esa conclusión fue **incorrecta**.

El análisis posterior del constructor y de la tabla de metadata de `sCamera::Viewport` demuestra:

- `mMode @ +0x13` es el enum de presets REGION_*;
- `mRegion @ +0x18` es un bloque/rectángulo de 16 bytes;
- el constructor de `mRegion` pone cuatro DWORD a cero;
- el motor deriva/usa ese rectángulo según `mMode`.

## Tabla enum exacta

Inmediatamente después de la vtable `0x04EC9954` aparece la tabla nombre/valor:

```text
FULLSCREEN   = 0
FREE         = 1
TOP          = 2
BOTTOM       = 3
LEFT         = 4
RIGHT        = 5
TOPLEFT      = 6
BOTTOMLEFT   = 7
TOPRIGHT     = 8
BOTTOMRIGHT  = 9
VIRTUAL      = 10
```

Los punteros de la tabla apuntan directamente al sufijo de las strings:

```text
REGION_FULLSCREEN
REGION_FREE
REGION_TOP
REGION_BOTTOM
...
```

## Consecuencia

Las escrituras históricas:

```text
viewport0.mMode = 2
viewport1.mMode = 3
```

**sí corresponden exactamente a TOP/BOTTOM**.

Por tanto la geometría TOP/BOTTOM del split-screen v2 no queda invalidada por la sección 36.

La sección 36 se conserva porque forma parte del proceso de investigación pedido, pero queda explícitamente superseded por esta sección.

## Layout confirmado adicionalmente

Constructor de `sCamera` alrededor de `0x0328A6A0`:

```text
base de Viewport array = sCamera + 0x30
count = 8
stride = 0x190
constructor Viewport = 0x03288A90
```

Bucle:

```asm
push 8
push 0x190
lea ecx,[sCamera+0x30]
call array_constructor
```

y posteriormente:

```text
viewport[i].Scene    @ +0x11 = i
viewport[i].Priority @ +0x12 = 7-i
```

Esto confirma estructuralmente que:

```text
Viewport0 base = sCamera+0x30
Viewport1 base = sCamera+0x1C0
```

y por tanto:

```text
Viewport0.mpCamera = +0x34
Viewport0.mVisible = +0x40
Viewport0.mMode    = +0x43
Viewport0.mDisplay = +0x44

Viewport1.mpCamera = +0x1C4
Viewport1.mVisible = +0x1D0
Viewport1.mMode    = +0x1D3
Viewport1.mDisplay = +0x1D4
```

Las direcciones usadas por el parche split-screen coinciden exactamente con estos campos.


---

# 38. Self/Partner View y corrección de VIEW_4

Se revisó completa la inicialización `sGameCamera::init` alrededor de `0x0203E120`.

La función reserva dos objetos `uCameraManage` de `0x240` bytes y los guarda en:

```text
sGameCamera + 0xCE0
sGameCamera + 0xCE4
```

Ambos usan el mismo constructor `0x01C52D78 -> 0x0205F2B0`.

## Corrección: VIEW_4 no recibe el Partner uCameraManage

La lectura inicial se resumió como “Self -> VIEW_0 / Partner -> VIEW_4”. Eso era incorrecto.

VIEW_0 recibe:

```text
getViewport(0)
setCamera([sGameCamera+0xCE0])
setDisplay(0)
setVisible(1)
```

Después el juego reserva un **tercer objeto distinto** de `0xB0` bytes. Ese objeto, no `[sGameCamera+0xCE4]`, se asigna a:

```text
getViewport(4)
setCamera(third_object)
setDisplay(1)
setVisible(1)
```

Por tanto VIEW_4 es una cámara adicional/debug/free-view y no la salida simultánea del Partner Manager.

# 39. Self View / Partner View seleccionan cuál ocupa VIEW_0

Callbacks:

```text
Self View    0x0203E570 -> usa +0xCE0
Partner View 0x0203E630 -> usa +0xCE4
```

Ambos llaman `0x0203E8A0`.

Si se elige Self:

```text
activar Self
desactivar Partner
bind VIEW_0 <- Self
```

Si se elige Partner:

```text
desactivar Self
activar Partner
bind VIEW_0 <- Partner
```

La unión usa el helper nativo:

```text
0x01C34D5A -> 0x01EBD610
```

que calcula:

```text
viewport = sCamera + 0x30 + index*0x190
```

y asigna su `mpCamera`, seguido de refresco del sistema.

Métodos usados por el selector debug:

```text
activate-like   0x01BF353F -> 0x02069AB0
deactivate-like 0x01C684D4 -> 0x020699B0
```

La estrategia actual de split-screen pasa a ser:

```text
activar Self
activar Partner
Self    -> VIEW_0 -> TOP(2)    -> display 0
Partner -> VIEW_1 -> BOTTOM(3) -> display 0
```

sin tocar VIEW_4.

# 40. mPadNo de uCameraManage se retira del parche nuevo

`uCameraManage::mPadNo` está confirmado por metadata en `+0x88`.

El experimento antiguo escribió Self=0 y Partner=1, pero la revisión posterior no ha encontrado una conexión directa de ese campo con `sGamePad` o `PadData`.

Sus referencias observadas no demuestran que sea el selector de dispositivo físico.

Por tanto el siguiente parche de cámara **no modificará mPadNo** hasta recuperar una ruta que justifique esa semántica.

La escritura antigua se conserva en el historial como experimento, pero queda marcada como no demostrada.


---

# 41. Construcción de CAMERA SPLIT v4

Con la arquitectura Self/Partner ya corregida se construyó un nuevo parche de cámara.

Objetivos:

- no reutilizar VIEW_4;
- no modificar `mPadNo`;
- usar las mismas rutinas de readiness/activación que los callbacks stock;
- usar el helper nativo de binding de viewport.

Hook:

```text
0x0203E3A9 -> 0x0203E3BD
```

El cave ensamblado ocupa:

```text
167 bytes / 179 bytes disponibles
```

y termina antes de la siguiente función en `0x0203E470`.

Secuencia:

```text
Self = [sGameCamera+0xCE0]
Partner = [sGameCamera+0xCE4]

para Self y Partner:
  check readiness 0x01B7BA8A
  si no está listo:
    init mode 0x13 mediante 0x01B8AAEE
  activate 0x01BF353F

bind Self    -> VIEW_0 con 0x01C34D5A
bind Partner -> VIEW_1 con 0x01C34D5A

VIEW_0 visible=1, mMode=TOP(2),    display=0
VIEW_1 visible=1, mMode=BOTTOM(3), display=0

refresh 0x01BFD4B8
```

Resultados:

```text
CAMERA SPLIT v4
SHA256 12ca6dae126e3e7354719637e144253d10a72cc6a8739de704f74e8aa0bcf172
172 bytes distintos / 2 rangos

LOCAL COOP v4 NATIVE SPLIT
(input v3 + camera v4)
SHA256 2b0e7b71cbac09cfb7fd5f3904f2e4709b2fdbfcbf372880e79f301cb95718ad
283 bytes distintos / 19 rangos
```

Ambas imágenes conservan exactamente el tamaño del original y siguen siendo PE32 válidos.

Estado: **solo validación estática**.


---

# 38. Self/Partner View y construcción del split nativo v4

La revisión de los callbacks debug confirmó que `Self View` y `Partner View` no son etiquetas decorativas.

Callbacks:

```text
Self View
  0x0203E570
  pasa [sGameCamera+0xCE0]

Partner View
  0x0203E630
  pasa [sGameCamera+0xCE4]
```

Ambos convergen en:

```text
0x01C784F1 -> 0x0203E8A0
```

## Comportamiento nativo de 0x0203E8A0

Si se selecciona Self:

```text
check ready Self       0x01B7BA8A
si hace falta init 13  0x01B8AAEE
activate Self          0x01BF353F
deactivate Partner     0x01C684D4
bind VIEW_0 <- Self    0x01C34D5A
```

Si se selecciona Partner:

```text
check ready Partner
si hace falta init 13
deactivate Self
activate Partner
bind VIEW_0 <- Partner
```

Por tanto Capcom ya implementa dos `uCameraManage` funcionales Self/Partner, pero de stock son alternativos sobre VIEW_0.

## Corrección de VIEW_4

La inicialización stock observada anteriormente se había resumido provisionalmente como si Partner terminara en VIEW_4.

La lectura completa muestra que VIEW_4 recibe un tercer objeto de cámara independiente, no `[sGameCamera+0xCE4]`.

**CORRECCIÓN:** VIEW_4 no se usa como viewport de Partner en el nuevo parche.

## Split nativo v4

Se construyó un nuevo parche de cámara que reutiliza únicamente rutas confirmadas del juego:

```text
Self    [sGameCamera+0xCE0]
Partner [sGameCamera+0xCE4]

activar Self
activar Partner
bind Self    -> VIEW_0
bind Partner -> VIEW_1

VIEW_0.mVisible = 1
VIEW_0.mMode    = TOP (2)
VIEW_0.mDisplay = 0

VIEW_1.mVisible = 1
VIEW_1.mMode    = BOTTOM (3)
VIEW_1.mDisplay = 0
```

No modifica VIEW_4.

No modifica `uCameraManage::mPadNo`.

### Hook

```text
hook VA  0x0203E3A9
cave VA  0x0203E3BD
next fn  0x0203E470
```

El hook sustituye 5 bytes del epílogo por un JMP y la cave reproduce después el epílogo original completo.

### Cámara-only v4

```text
BioRevHD 30-Enero-2013 CAMERA SPLIT v4.exe
SHA-256:
12ca6dae126e3e7354719637e144253d10a72cc6a8739de704f74e8aa0bcf172

same size: yes
different bytes: 172
ranges: 2
```

### Local co-op v4 combinado

Se aplicó el camera split v4 sobre el input v3 corregido:

```text
BioRevHD 30-Enero-2013 LOCAL COOP v4 NATIVE SPLIT.exe
SHA-256:
2b0e7b71cbac09cfb7fd5f3904f2e4709b2fdbfcbf372880e79f301cb95718ad

same size: yes
different bytes vs original: 283
ranges: 19
```

Estado: **STATICALLY VERIFIED ONLY**.

La siguiente validación necesaria es runtime: comprobar que ambos managers permanecen actualizados simultáneamente y que Partner posee una cámara válida durante gameplay.


---

# 41. SubPlayer0+0x44 identifica el uNpc vivo del compañero

La rutina común de binding de `uPcsPlayer*` en `0x02DF4F30` usa:

```text
uPcsPlayer* + 0x40 = ID PCS esperado
uPcsPlayer* + 0x44 = actor encontrado
```

El scan obtiene el objeto real mediante `0x01BEE332` y lee su ID con `0x01BEDBB7 -> 0x01CB7610`, cuya implementación devuelve `[this+0xE3C]`. Cuando ese ID coincide con `uPcsPlayer*+0x40`, el propio objeto se almacena en `uPcsPlayer*+0x44`.

Como `uPcsPlayerSub0` tiene vtable exacta `0x04E1649C`, se puede identificar el compañero Sub0 sin depender del personaje concreto.

El constructor de `uNpc` inicializa:

```text
[uNpc+0xE3C] = -1
[uNpc+0xE40] = 2
```

y el enum ThinkMode reconstruido es:

```text
0 Invalid
1 Pad
2 Cpu
3 Network
```

Por tanto el compañero normal/offline nace como `ThinkMode::Cpu`.

---

# 42. Experimento v5 — filtro exacto Sub0 manteniendo Cpu

Outputs:

```text
BioRevHD 30-Enero-2013 LOCAL COOP P2 SUB0 INPUT v5.exe
SHA-256 d294508a5fe4919eb9cb46c446f23ba7a79eb1da8664dac3d7cf9ed3437068b7

BioRevHD 30-Enero-2013 LOCAL COOP v5 SUB0 NATIVE SPLIT.exe
SHA-256 e83207b2c64172468930eacbe0460fb7847061b56ac3d4e3a3b7f3ee009bfe9c
```

v5 amplía solo 4 bytes el `VirtualSize` de `.data`:

```text
0x003B4184 -> 0x003B4188
```

y usa `0x057D9184` como `gLocalCoopSub0Npc`.

Regla:

```text
Pad(1)       -> stock
Cpu(2) Sub0  -> ruta local + selector 1
Cpu(2) otros -> CPU/AI stock
Network(3)   -> stock
```

Verificación estática:

```text
input-only: 210 bytes distintos / 21 rangos
combined:   382 bytes distintos / 23 rangos
same file size: sí
```

Limitación: Sub0 sigue teniendo formalmente `ThinkMode::Cpu` fuera de la rutina desviada, así que otros subsistemas de IA podrían seguir considerándolo CPU.

---

# 43. Setter oficial de ThinkMode

Se identificó la ruta virtual completa:

```text
0x01BB8B60 -> 0x0278CC40
```

El wrapper llama al setter real `0x01BE3257 -> 0x027F1290` y después refresca el controlador asociado mediante `0x01C03223 -> 0x01DF4C60` y `0x01C338A1 -> 0x02080240`.

El setter real escribe finalmente:

```text
[uNpc+0xE40] = new ThinkMode
```

y solo ejecuta el teardown específico de red cuando el modo anterior era `Network=3` y se sale de Network. La transición `Cpu(2) -> Pad(1)` no entra en esa rama.

---

# 44. Experimento v6 — Sub0 pasa nativamente a ThinkMode::Pad

v6 sustituye el desvío de la rama CPU de v5 por una transición nativa:

```text
si actor == SubPlayer0 y ThinkMode == Cpu(2):
    SetThinkMode(Pad=1) mediante 0x01BB8B60

si ya es Pad(1):
    no cambiar

si es Network(3):
    no cambiar
```

Después, la ruta stock `ThinkMode::Pad` se ejecuta normalmente. El selector devuelve PadData 1 solo para el actor Sub0 exacto, y los getters PC de `sGamePad` respetan el selector restaurado.

Outputs:

```text
BioRevHD 30-Enero-2013 LOCAL COOP P2 SUB0 PADMODE v6.exe
SHA-256 83554753d1d6a86bf256627830f509a313871a847c87338b61da0e3c0a2c0914

BioRevHD 30-Enero-2013 LOCAL COOP v6 SUB0 PADMODE NATIVE SPLIT.exe
SHA-256 18a429daa0855a7e6de6af91358d867970beee9a99671913303166f8b39a540d
```

Verificación:

```text
v6 input: 201 bytes distintos / 19 rangos / mismo tamaño
v6+split: 373 bytes distintos / 21 rangos / mismo tamaño
```

v6 ya no modifica el branch de ThinkMode en `0x027A0CA0`.

**Estado:** STATICALLY VERIFIED ONLY.
