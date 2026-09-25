# 17 — Corrección de owner de ActionCommand y alcance real de v12/v14

Fecha: 2026-09-25. Evidencia obtenida del EXE original de enero y del v13 adjunto, identificados por sus hashes completos. No se ha ejecutado gameplay.

## 1. Resincronización conceptual con la rama paralela

Durante esta auditoría, `research/door2p-routing` avanzó hasta `6fd8942886532e373665a941ad45c36a71b5eb89` y declaró v14 DOOR GIMMICK PAD2 (`73fe1255697c47a624025ba40b33bab8df5812e76021bdc2d9acdeec3daf56ea`) como base estática.

Ese cambio se conserva. El helper usa correctamente `[ebp-0x20]`, el actor resuelto de MoveState, no `[ebp-0x14]`, que contiene la puerta. La restauración de su selector no cambia los filtros anteriores documentados en `docs/16-door-gimmick-wait-entry.md`: Self en WaitState y el finder pl-only. No debe interpretarse como prueba de que un np/Sub0 alcanza esa lectura.

Las menciones a v13 como base en los checkpoints 15/16 describen el preflight anterior, no una orden de revertir el trabajo paralelo. La comprobación de 92 aserciones verifica bytes/estructuras, no valida las conclusiones históricas de pickup end-to-end.

## 2. La rutina común no es uGimmick: es uObjModel

Rectificación del nombre provisional usado al final de los checkpoints 15/16: `0x026D4350` construye **uObjModel**.

Cadena literal:

```
constructor store 0x026D437E -> vtable 0x04D926DC
vtable[-1] -> COL 0x0534CF00
COL+0x0C -> TypeDescriptor 0x0547C7DC
TypeDescriptor+8 -> .?AVuObjModel@obj_model@chara@game@app@@
```

Su constructor inicializa `[this+0xF48] = 6` en `0x026D440A`. No es lícito interpretar ese campo como el puntero candidato de uItem.

La inicialización de ActionCommand `0x026D7050`, mediante thunk `0x01C3C5F0`, tiene 18 callsites directos localizados; entre ellos `0x02597D94`, en la familia de puertas gimmick.

En `0x026D7216` se hace `push 0x01BA99B7`, el thunk de `0x026D7AD0`, para construir el delegate. Este se instala por `0x026D7303 -> 0x01C7F26A -> 0x02335940` en el subobjeto `ActionCommand+0x34`. El comando se guarda mediante la estructura de `uObjModel+0xF5C`.

La única aparición del puntero crudo `0x01BA99B7` está en `0x026D7217`, **pero su binding único pertenece a la rutina común de uObjModel**, no al constructor de uItem. Un único binding no demuestra un único tipo concreto de caller.

## 3. El uItem real tiene otra vtable y otro ActionCommand

Cadena RTTI de uItem:

```
constructor store 0x0245E920 -> vtable 0x04D2FEE4
vtable[-1] -> COL 0x05325A60
COL+0x0C -> TypeDescriptor 0x0545E070
TypeDescriptor+8 -> .?AVuItem@chara@game@app@@
```

La dirección `0x04D30844` citada antiguamente como vtable de uItem corresponde en realidad a `uItem::MyDTI`. Esto no invalida la identificación de la string/DTI global de uItem, pero sí la atribución de esa vtable como objeto de gameplay.

uItem inicializa `[this+0xF48] = 0` en `0x0245EA1E`. La ruta de pickup guarda allí al candidato, tal como se había identificado.

Su **ActionCommand es un subobjeto en `uItem+0xFD0`**, distinto del almacenamiento de uObjModel. La configuración de uItem hace:

```asm
0245F3F6 push 01C60DD3h       ; delegate de member/selector
...
0245F414 call 01C1ECDFh      ; construye delegate en [ebp-58h]
...
0245F50C lea eax,[ebp-58h]
0245F50F push eax
0245F510 mov ecx,[ebp-8]
0245F513 add ecx,0FD0h      ; ActionCommand de este uItem
0245F519 call 01C7F26Ah     ; mismo setter +34h
```

El thunk correcto es:

```
0x01C60DD3 -> 0x024605D0
```

La implementación stock de `0x024605D0` termina en `xor eax,eax; ...; ret 4`. Su cuerpo de 46 bytes sigue íntegro en el v13 adjunto.

## 4. Qué hizo realmente v12

v12 sustituyó los 46 bytes de `0x026D7AD0` por un selector que interpreta `[ecx+0xF48]` como actor de pickup.

Pero:

- `0x026D7AD0` se instala desde uObjModel;
- allí `+0xF48` tiene layout distinto, inicializado a 6;
- el selector de uItem instalado realmente es `0x024605D0`;
- ese selector de uItem **sigue devolviendo 0** en v13;
- la v14 paralela modifica únicamente el hook `0x02591118` y su helper `0x01C95300`, por lo que no cambia este diagnóstico.

**Conclusión:** el hash reproducible de v12/v13 no prueba que su supuesto selector de pickup se haya aplicado al owner correcto. Se retira la conclusión histórica de que el botón de pickup de Sub0 estaba cerrado por ese cambio.

## 5. Corrección acotada a implementar y probar

Sin tocar la v14 de puertas ni la entrega/selección de candidatos de v11:

1. restaurar el selector común `0x026D7AD0` a su cuerpo stock; no leer el layout de uItem desde uObjModel;
2. instalar la regla exact-Sub0 en el selector real `0x024605D0`;
3. devolver 1 únicamente con flag local activo, `this` válido, tracker Sub0 no nulo y candidato `uItem+0xF48` igual a ese tracker;
4. devolver 0 en las demás rutas; conservar el `ret 4`;
5. preservar la corrección del getter `0x02DB2B7D` y los demás cambios válidos;
6. fijar hashes de entrada, verificar todas las escrituras, demostrar reversión exacta y probar los casos de guard/retorno antes de ofrecer un candidato.

No ampliar globalmente `isPlayer` ni modificar el índice local de red. El trabajo restante de puertas sigue siendo coherencia entre candidato, propietario del ActionCommand, serial de WaitState y finder de MoveState; no queda resuelto por esta corrección de pickup.
