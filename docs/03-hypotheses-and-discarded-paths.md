# 03 — Hipótesis y caminos descartados

Solo se incluyen hipótesis surgidas durante esta conversación.

| ID | Hipótesis / pista | Evidencia | Estado |
|---|---|---|---|
| H01 | Enero 2013 es la mejor base para descubrir la arquitectura | `FullDebugWin32`, ~60.7 MB, RTTI/strings/debug conservados | ACTIVA |
| H02 | Febrero 2013 conserva CreateRaidPlayer funcional | conserva wrapper, pero termina en `0x0079C2D0: ret 4` | DESCARTADA |
| H03 | `CreateRaidPlayer N` son strings muertos | enero tiene callback e implementación parametrizada | DESCARTADA |
| H04 | `CreateRaidPlayer 2` puede crear directamente el P2 local final | función real, pero tipo/controlador resultante aún sin demostrar | ABIERTA |
| H05 | `mCameraList[0]/[1]` son cámaras P1/P2 | los xrefs pertenecen a `uObjModel` | DESCARTADA como hook principal |
| H06 | `mPadViewportNo` es el mapa global pad↔viewport | contexto de `sVibration` | DESCARTADA como hook principal |
| H07 | `mPadNo +0x88` es `uPlayer::mPadNo` | el registro de propiedades lo sitúa en `uCameraManage` | CORREGIDA |
| H08 | `sCamera` soporta varios viewports reales | `VIEW_0..VIEW_7` y creación de múltiples Viewport | ACTIVA / FUERTE |
| H09 | `mStartPadNo` participa en el input real | pertenece a `sGamePad`, se lee en rutinas de pad | CONFIRMADA |
| H10 | Cambiar solo `mStartPadNo` resolvería P2 | es global y parece seleccionar el pad inicial/principal | POCO PROBABLE |
| H11 | `sPcsManager` ya distingue Main/Sub0/Sub1 | enum real, tres IDs, setters/getters y bucle de 3 slots | CONFIRMADA |
| H12 | Main/Sub0/Sub1 son solo etiquetas | RTTI muestra clases `uPcsPlayerMain/Sub0/Sub1` | DESCARTADA |
| H13 | `mMovePcs` / `mMoveSubPcs` pueden participar en la separación de control | nombres y callbacks reales | ACTIVA |
| H14 | P2 debe implementarse desde cero | ya existe infraestructura Main/Sub y varios viewports | DESCARTADA como punto de partida |
| H15 | El mejor orden es spawn+input+cámara todo a la vez | dificulta aislar fallos | DESCARTADA metodológicamente |

| H16 | Febrero debía ser el target práctico porque importa XInput | import `XINPUT1_3.dll` frente a enero/DInput | CORREGIDA: enero conserva lógica que febrero stubbea |
| H17 | Un split-screen estático prueba cooperativo local | se pueden configurar dos CameraManage/Viewports | DESCARTADA: falta validar actor, input, cámara activa, HUD y escenas |
| H18 | La solución de Revelations 2 puede trasplantarse directamente | misma familia MT Framework y antecedentes de local co-op | DESCARTADA como evidencia/trasplante; solo referencia conceptual |
| H19 | Strings `Pad2`/`PS2Pad2`/`X(2P)` bastan para identificar P2 | nombres muy sugerentes | DESCARTADA como prueba; necesitan owner/xref/flujo ejecutable |

## Descartes explicados

### `mCameraList[0]/[1]`

Parecía muy prometedor porque eran dos entradas explícitas. Los xrefs mostraron que pertenecían al sistema de cámaras de `uObjModel`, por lo que no se pueden usar como prueba de dos cámaras de jugador.

### `mPadViewportNo`

El nombre parecía resolver directamente la asociación entre pad y viewport. Al inspeccionar el contexto apareció dentro de `sVibration`, junto con configuración de vibración de cámara y mando.

### `uPlayer::mPadNo @ +0x88`

La primera interpretación del campo `mPadNo` fue demasiado rápida. El constructor y el bloque de propiedades demostraron que el campo estudiado estaba en `uCameraManage`.

### CreateRaidPlayer en febrero

El menú y wrapper todavía están presentes, pero la implementación final fue reemplazada por un stub. Por eso febrero sigue siendo útil como comparación, no como base principal para recuperar esa lógica.

## Regla de validación

Una hipótesis no se considerará funcional porque:

- exista un string;
- exista RTTI;
- aparezca un segundo actor;
- una cámara se pueda crear.

Para validar P2 habrá que demostrar:

```text
Pad 0 -> solo Main
Pad 1 -> solo Sub0
ambos actores simultáneos
```

y después demostrar dos cámaras/viewport independientes.
