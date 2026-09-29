# 88 — Corrección runtime de Sub0: Cpu(3) → Pad(1)

29/09/2026. Corrección motivada por prueba real del propietario en Windows.

## Resultado observado

El candidato PR59 arrancaba y entraba en partida, pero:

- J2 seguía completamente controlado por IA;
- el mando 2 no producía movimiento ni acciones.

Esto invalida la afirmación anterior de que la conversión básica Sub0 → Pad estaba resuelta en gameplay.

## Causa

La investigación canónica de `uPlayer::ThinkMode` ya había establecido:

```text
0 = Invalid
1 = Pad
2 = Network
3 = Cpu
```

Sin embargo, `patches/local_routing/hooks.S` heredó la comparación histórica:

```asm
cmp dword ptr [eax+0xE40],2
je  .convert_cpu
```

Por tanto el compañero normal de campaña, que llega como `Cpu(3)`, no entraba en
`.convert_cpu`. El hook guardaba el puntero Sub0, pero dejaba su `ThinkMode`
en CPU. La rutina de input `0x027A0CA0` seguía entonces la rama de IA y nunca
consultaba `sGamePad` para ese actor.

El valor 2 corresponde a Network y debe preservarse.

## Corrección

El hook activo pasa a:

```asm
cmp dword ptr [eax+0xE40],3
je  .convert_cpu
cmp dword ptr [eax+0xE40],1
jne .bind_done
```

La conversión sigue usando el setter nativo existente:

```text
0x01BB8B60 -> wrapper de cambio ThinkMode
Cpu(3) -> Pad(1)
```

El selector de pad ya existente continúa devolviendo:

```text
Self / resto -> PadData[0]
Sub0 exacto  -> PadData[1]
```

No se cambia `Network(2)`.

## Regresión añadida

`tests/test_bridges.py` ahora exige explícitamente:

- Sub0 + Cpu(3) -> una llamada al setter, local activo;
- Sub0 + Network(2) -> cero llamadas al setter, local inactivo;
- Sub0 + Pad(1) previamente convertido -> restauración del estado local solo si
  coinciden actor anterior, tracker y binding.

Los hashes de la cadena corregida calculados sobre la build GNU son:

```text
LOCAL ROUTING corregido:
da2d48b79366874155fdf2a115217bbdfac0263a4c59f392fc5416d68adda04f

January script base corregida:
31cbebd180bcc66da2afbdc57c928b22a47f956c53bb5ed12411a220206490ec
```

`build_script_serial_guard.py` y `tools/runtime_image.py` admiten ahora estas
bases corregidas para que una reconstrucción completa no vuelva a introducir
el valor histórico equivocado.

## Estado

La corrección está verificada por código/tests, pero necesita la nueva prueba
real del propietario:

1. entrar en una escena con partner;
2. confirmar que J2 deja de actuar como IA;
3. mover J2 con el mando físico 2;
4. comprobar simultáneamente P1 con PadData[0] y J2 con PadData[1].

Hasta esa prueba no se declara input local validado.
