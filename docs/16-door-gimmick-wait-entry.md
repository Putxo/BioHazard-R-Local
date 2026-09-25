# 16 — Door gimmick: la identidad de P1 se fija antes del botón

Fecha: 2026-09-25. Continúa `docs/15-door-gimmick-actor-routing.md`.

**Estado: análisis estático reproducido en el EXE original y en v13. No se ha ejecutado gameplay ni generado un nuevo parche.**

## 1. Dos correcciones de atribución, verificadas mediante RTTI

El documento histórico `docs/13-door-2p.md` debe leerse con estas rectificaciones. Sus conclusiones generales sobre otras clases no se extrapolan a esta familia.

| Dirección de función | Clase propietaria real | Vtable | Slot / thunk |
|---|---|---|---|
| `0x02590040` | `door_gimmick::WaitState` | `0x04D5D61C` | 8: `0x01B7E9EC` |
| `0x02590F60`, contiene `0x02591121` | `door_gimmick::MoveState` | `0x04D5D684` | 8: `0x01C4DF17` |
| `0x02711D70` | implementación compartida del slot 8 de `PlayerDoorGimmickWaitState` | `0x04D5D754` | 8: `0x01C74013` |

En particular, `0x02590040` **no pertenece a uDoorAutoBase**. La proximidad de sus datos a las strings de door_auto no demuestra ownership. Su vtable apunta al COL `0x0533662C`, cuyo TypeDescriptor `0x0548551C` contiene literalmente `.?AVWaitState@door_gimmick@obj_model@chara@game@app@@`.

La comprobación del DTI de `PlayerDoorGimmickWaitState` dentro de `MoveState` tampoco convierte al caller en esa clase.

## 2. Productor del serial de interacción: WaitState

`0x02590040` obtiene Self mediante `0x01C16468` en `0x02590072`. En la rama que acepta la interacción local y valida al actor mediante `0x01BDD159`, hace:

```asm
025900AD call 01B95665h
025900B2 mov ecx,eax
025900B4 call 01C85962h       ; índice/serial local del proceso
025900B9 mov [ebp-29h],al
025900BC mov eax,[ebp-8]      ; WaitState
025900BF mov cl,[ebp-29h]
025900C2 mov [eax+10h],cl
```

El método de transición `0x025901F0` consume ese byte con extensión de signo:

```asm
0259022F push 0
02590231 mov eax,[ebp-8]
02590234 movsx ecx,byte ptr [eax+10h]
02590238 push ecx
02590239 push 1
...
02590245 call 01BF2AC7h       ; -> 02080790h
```

La inicialización de MoveState (`0x02590D00`) guarda su segundo argumento en `MoveState+0x10`.

**Consecuencia estática:** la entrada local ordinaria transporta el serial local del proceso, no una identidad derivada de un segundo mando. No basta con cambiar la lectura posterior del botón.

Existe otra rama en `0x025900DE..0x02590116` que consume campos del objeto puerta; no se elimina ni se interpreta como un selector local de P2 sin reconstruir su productor.

## 3. El filtro de proximidad también exige Self

En `door_gimmick::WaitState`, método `0x02590280`, la colección de candidatos se recorre antes de habilitar la interacción. Para cada candidato relevante:

```asm
0259040C call 01C8C9A1h       ; categoría pl
...
02590419 je 02590429h        ; descarta si no es pl
0259041B call 01C16468h      ; Self
02590420 cmp [ebp-50h],eax   ; candidato debe ser ese Self exacto
02590423 jne 02590429h
02590425 mov byte ptr [ebp-35h],1
```

Por tanto no es únicamente un problema de asignar PadData[1]: esta rama no admite un candidato np/Sub0 como usuario local alternativo. Si ambos están presentes, sus comprobaciones de elegibilidad siguen referidas a Self.

Esto prueba una limitación de esa ruta de código, **no una prueba dentro de una escena concreta**.

## 4. La identidad se vuelve a resolver varias veces

Se verificaron cinco llamadas al finder `0x01C07341 -> 0x02DA3840` en esta familia:

| Callsite | Contexto inspeccionado |
|---|---|
| `0x02590DD3` | Inicialización de MoveState, serial no negativo |
| `0x02590FA0` | Actualización de MoveState |
| `0x0259681D` | Helper de entrada, serial guardado en puerta `+0x11A0` |
| `0x02596C02` | Helper asociado a continuación/salida |
| `0x02596F3F` | Otro helper asociado a salida/restauración |

No se atribuyen nombres C++ finales a los tres helpers solo por su posición.

El finder comprueba categoría `pl` **antes** de comparar el serial. Por ello no encuentra un np aunque su serial coincida con el solicitado.

La inicialización y los tres helpers tienen una alternativa para valores negativos que resuelve al partner mediante `0x01C4C9C3`. La actualización `0x02590F60` no reproduce esa alternativa en su entrada y sale si el finder devuelve null. No se convertirá indiscriminadamente a Sub0 en un valor negativo: podría escoger una ruta de control distinta y dejar al update sin actor.

## 5. El botón es el último de varios puntos que deben concordar

`0x02591118` sigue haciendo `push 0` antes de llamar a `0x02DB0CF0`. v13 ya permite que ese getter respete un argumento selector; este caller no le entrega 1.

Un cambio correcto de esta familia debe mantener coherentes:

```
candidato de proximidad
  -> propietario de ActionCommand
  -> serial que WaitState entrega a la transición
  -> actor que MoveState y los helpers recuperan
  -> selector de pad de ese actor
```

No basta un parche de dos bytes en el botón ni ampliar globalmente `isPlayer` o el índice de red local.

## 6. Verificación efectuada

Herramienta nueva, solo lectura: `scripts/audit_door_gimmick.py`.

Ejecución sobre las dos copias adjuntas:

- original enero: `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`;
- v13: `3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa`.

Resultado: **46 comprobaciones por imagen, 92 en total**, y **6 controles negativos**. Se comprobaron además dos errores de CLI: rechazo de usar el EXE original como v13 y rechazo de escribir el informe sobre el EXE de entrada. Ambos devuelven código 2; el hash del original permanece intacto.

Las comprobaciones cubren SHA, PE32/i386, ImageBase, COL/TypeDescriptor, vtables, destinos de llamadas y saltos, máscaras y constantes, argumentos de serial, salida temprana por actor null y selector del getter. Los controles negativos cubren corrupción, identificación de build equivocada, cabeceras inválidas/truncadas, VA sin bytes de archivo y offset negativo.

**PASS significa coincidencia de las evidencias estructurales, no funcionamiento del cooperativo.** No se ejecutó código de máquina del juego, no se emuló el juego y no se ejecutó gameplay.

Uso:

```text
python scripts/audit_door_gimmick.py "EXE original de enero.exe" --v13 "EXE v13.exe" --report audit.json
```

No requiere paquetes Python externos y rechaza los otros tres ejecutables aportados por no ser las builds fijadas por hash.

## 7. Punto siguiente exacto

Reconstruir el owner/selector del `cActionCommand` instalado por la capa común de uGimmick, empezando por `0x026D7050` y sus consumidores `0x026D75D0`/`0x026D7870`, y conectarlo con el candidato en `0x02590280` y el serial de `WaitState+0x10`.

Solo después diseñar la excepción local por **puntero exacto de Sub0**, preservando el resto de NPC, las ramas de partner negativo, Network y la exclusión de uso simultáneo del mismo objeto. La base canónica continúa siendo v13. El antiguo candidato v14 de WaitState_PL no resuelve los gates de esta familia y no se promociona aquí.
