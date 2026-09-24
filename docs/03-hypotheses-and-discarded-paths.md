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
| H13 | `mMovePcs` / `mMoveSubPcs` pueden participar en la separación de control | los setters registrados son `ret 4`; los getters solo exponen campos internos `+0x08/+0x0C` al debug | DESCARTADA como interruptor de movimiento |
| H14 | P2 debe implementarse desde cero | ya existe infraestructura Main/Sub y varios viewports | DESCARTADA como punto de partida |
| H15 | El mejor orden es spawn+input+cámara todo a la vez | dificulta aislar fallos | DESCARTADA metodológicamente |

| H16 | Febrero debía ser el target práctico porque importa XInput | import `XINPUT1_3.dll` frente a enero/DInput | CORREGIDA: enero conserva lógica que febrero stubbea |
| H17 | Un split-screen estático prueba cooperativo local | se pueden configurar dos CameraManage/Viewports | DESCARTADA: falta validar actor, input, cámara activa, HUD y escenas |
| H18 | La solución de Revelations 2 puede trasplantarse directamente | misma familia MT Framework y antecedentes de local co-op | DESCARTADA como evidencia/trasplante; solo referencia conceptual |
| H19 | Strings `Pad2`/`PS2Pad2`/`X(2P)` bastan para identificar P2 | nombres muy sugerentes | DESCARTADA como prueba; necesitan owner/xref/flujo ejecutable |

| H20 | `mMovePcs/mMoveSubPcs` son rutas de movimiento/input | nombre inglés parecía "Move PCS" | DESCARTADA: etiquetas japonesas indican número de PCS/sub-PCS |
| H21 | El experimento P2 input resolvía la selección de mando sustituyendo getPadNo | getPadNo retorna 0 y se sustituyó por 0/1 | INCOMPLETA: sGamePad además ignora el argumento y usa mStartPadNo |
| H22 | La build PC solo mantiene un pad internamente | gameplay usa siempre pad 0 | DESCARTADA: sGamePad construye y actualiza dos PadData con índices 0/1 |
| H23 | Main/Sub0/Sub1 son roles nativos con tipos separados | vtables y overrides fijos 0/1/2; uPcsActor<Player0/1/2> | CONFIRMADA |

| H24 | Modos internos 2/3 podían representar P2 local | el primer experimento los usó como señal secundaria | DESCARTADA: 2=Network y 3=Cpu; P2 local debe usar ThinkMode::Pad(1) |
| H25 | La rama 2 era otro tipo de input local | consumía analog/aim/run | DESCARTADA: consume cPlayerPadSyncData y corresponde a ThinkMode::Network |
| H26 | uPlayer no conserva ownership/control mode explícito | el modo parecía residir solo en gestor de estado | DESCARTADA: uPlayer+0xE40 cachea ThinkMode y existe setter dedicado |
| H27 | Para P2 local hay que secuestrar Network/Cpu | ramas 2/3 ya transportan un segundo estado | DESCARTADA: solución nativa es Sub0 ThinkMode::Pad + pad 1 |

| H30 | Un selector basado en Self/Partner puede separar los pads sin heurística de modo | getter stock siempre 0 + predicado nativo Self/Partner | CONFIRMADA ESTÁTICAMENTE en v3 |
| H31 | Partner+Cpu puede convertirse a Pad usando la API interna sin tocar Network | wrapper 0x01BB8B60 / setter 0x027F1290 | CONFIRMADA ESTÁTICAMENTE en v3; falta runtime |
| H32 | Convertir siempre Partner Cpu->Pad es seguro en todas las escenas | v3 fuerza la conversión durante update de input | ABIERTA/RIESGO: scripts/cutscenes pueden requerir CPU temporal |

| H28 | El selector restaurado puede alcanzar dos entradas de PadData | `0x02DBE720` valida index<2 y usa stride `0xC0` | CONFIRMADA estáticamente |
| H29 | PadData[1] corresponde al segundo mando físico conectado | existen dos slots internos y la API permite 0/1 | ABIERTA hasta prueba runtime |

| H34 | VIEW_4 stock contiene Partner uCameraManage | el índice 4 aparece tras crear los managers | DESCARTADA: VIEW_4 recibe un tercer objeto de 0xB0 bytes |
| H35 | Self/Partner son dos cámaras alternativas que Capcom enlaza a VIEW_0 | callbacks 0x0203E570/630 -> 0x0203E8A0 | CONFIRMADA |
| H36 | El split más limpio es Self->VIEW_0 y Partner->VIEW_1 activados simultáneamente | helper nativo 0x01EBD610 admite cualquier viewport y VIEW_1 existe | ACTIVA / PRIORITARIA |
| H37 | uCameraManage::mPadNo debe ponerse 0/1 para controlar dos mandos | el nombre del campo lo sugería | NO DEMOSTRADA; retirada del parche nuevo hasta nueva evidencia |

| H39 | El compañero offline relevante puede filtrarse por uPcsPlayerSub0 en vez de redirigir todos los Cpu | uPcsPlayerSub0 vtable 0x04E1649C y binder +0x44 | CONFIRMADA |
| H40 | uPcsPlayerSub0+0x44 es el actor vivo cuyo ID está en uNpc+0xE3C | scan común compara el ID y guarda el objeto real | CONFIRMADA |
| H41 | Redirigir globalmente ThinkMode::Cpu sería seguro | muchos NPC comparten la ruta Cpu | DESCARTADA |
| H42 | Forzar Sub0 a ThinkMode::Network es la mejor forma de control local | implicaría efectos de red innecesarios | DESCARTADA frente a PadMode |
| H43 | Mantener Sub0 como Cpu y desviar solo su control principal basta | v5 lo implementa | POSIBLE, pero superseded por v6 por riesgo de otros subsistemas AI |
| H44 | Cambiar solo Sub0 Cpu->Pad con el setter oficial es la ruta más nativa | wrapper 0x0278CC40 + setter 0x027F1290 | ACTIVA / IMPLEMENTADA EN v6 |
| H45 | v6 preserva online al no tocar Sub0 Network=3 | el hook solo cambia Cpu=2 | CONFIRMADA estáticamente |

| H38 | v6 cubría todo el mando P2 | cubría analógicos y algunos booleanos | CORREGIDA: había 63 cargas adicionales relevantes; v7 las restaura |
| H39 | Activar Partner solo en sGameCamera::init basta | v4 dejaba ambos managers activos | DESCARTADA: setup posterior vuelve a desactivar/limpiar Partner |
| H40 | 0x027A1500 era padding seguro para v8 | parecía continuación del cave anterior | DESCARTADA: verificación de bytes encontró código real |
| H41 | 0x01C94FC0 es code cave apto | run continuo de 0xCC de 64 KiB | CONFIRMADA para helper v8 de 314 bytes |
| H42 | El uNpc no-Self usa Partner CameraManage nativamente | 0x027A2FA0 devuelve Self o Partner según identidad | CONFIRMADA |
| H43 | Un flag local separado puede preservar online y hacer persistente la cámara | solo se activa tras Cpu->Pad; Network no lo activa | IMPLEMENTADA en v8 |

| H46 | `cSystemData<Game>::mGameMode` gobierna ItemBox normal/coop | property metadata `mGameMode`; Campaign=0, Coop=1 | CONFIRMADA |
| H47 | El slot 1 de ItemBox puede ser nulo en Campaign | podía parecer creado solo en Coop | DESCARTADA: cGameSystemDouble crea sItemBoxCoop automáticamente |
| H48 | Para v9 hay que crear un inventario P2 desde cero | existe sItemBoxCoop completo en slot 1 | DESCARTADA |
| H49 | El parche mínimo debe reutilizar el selector de ItemBox existente | GameMode ya conmuta slot 0/1 | ACTIVA / PRIORITARIA |

| H50 | Cambiar globalmente `mGameMode` a Coop es necesario para usar sItemBoxCoop | stock selector usa mGameMode | DESCARTADA: basta ensanchar solo el selector ItemBox con gLocalCoopActive |
| H51 | El selector local puede preservar Campaign para el resto del juego | wrapper stock_isCoop OR local flag en 0x01D067AF | IMPLEMENTADA EN v10 |
| H52 | v10 deriva limpiamente de v9 | revertir los 2 rangos nuevos reproduce exactamente SHA v9 | CONFIRMADA |

| H53 | v10 ya permite a Sub0 recoger cualquier uItem normal | ItemBox/pack ya son actor-specific | DESCARTADA: uItem finder y processor siguen exigiendo categoría pl |
| H54 | Parchear globalmente isPlayer para aceptar np resolvería pickup | permitiría pasar los guards | DESCARTADA: afectaría todos los NPC del juego |
| H55 | uItem puede aceptar solo el Sub0 exacto sin alterar categorías globales | gLocalCoopActive + gSub0Npc ya existen | ACTIVA / DISEÑO v11 |
| H56 | Elegir Main/Sub0 por proximidad preserva P1 y habilita P2 con un solo mCandidate | uItem+0xF48 solo admite un actor y ya calcula distancia | ACTIVA / DISEÑO v11 |

| H57 | La v11 canónica de la cadena v12/v13 es la variante nearest-P1/Sub0 | SHA 2c69c5f... y el builder v12 exige exactamente ese base SHA | CONFIRMADA |
| H58 | El uItem ActionCommand puede usar Pad 2 para Sub0 | v12 devuelve member 1 solo para target exacto Sub0 y restaura selector en 0x02DB2B50 | IMPLEMENTADA / ESTÁTICAMENTE VERIFICADA |
| H59 | Pickup de Sub0 termina en el inventario de P1 por estar en Campaign | la entrega stock usa actor admitido -> uNpc+0x1524 -> cBioItemPack propio | DESCARTADA |
| H60 | La regla Coop de ammo relief puede replicarse sin cambiar mGameMode | v13 envuelve gate, selección del otro actor, validación y multiplicador | IMPLEMENTADA / ESTÁTICAMENTE VERIFICADA |
| H61 | uDoor2pBase ya contiene estado independiente para dos participantes | mReadyFlag, mGuestStatusFlag y mLocalFlag [0/1], además de setter actor-específico | CONFIRMADA |
| H62 | El índice local global usado en puertas es necesariamente un bloqueo para local co-op | existen lecturas de 0x02DA3050, pero aún no se separa gameplay de red/feedback | ABIERTA; NO TRATAR COMO BLOQUEO TODAVÍA |
| H63 | El setter 0x02588D20 puede aceptar Sub0 estructuralmente | deriva índice 0/1 del actor y no filtra pl/np | CONFIRMADA ESTRUCTURALMENTE; falta demostrar caller real |
| H64 | Hace falta v14 para puertas | todavía no hay bloqueo concreto demostrado | NO DEMOSTRADA; NO CREAR v14 AÚN |

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


### `mMovePcs` / `mMoveSubPcs`

Parecían candidatos directos para habilitar/deshabilitar movimiento del jugador principal y subjugadores.

Al seguir los callbacks registrados:

- los setters son funciones vacías que retornan inmediatamente (`ret 4`);
- los getters consultan únicamente dos DWORD/campos internos alrededor de `+0x08` y `+0x0C`;
- su función observable es exponer estado al sistema de debug/propiedades.

**DESCARTADOS como mecanismo para activar el control local de Sub0.**
