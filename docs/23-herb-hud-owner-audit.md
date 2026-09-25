# 23 — HUD de hierbas: propietario y contador de la instancia

Fecha: 2026-09-25. Auditoría de solo lectura del EXE de enero con SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.

## Identificación concreta

`uGUI_MapBaseAndHerb` tiene vtable `0x04DE810C`. La cadena RTTI es vtable[-1] → COL `0x0537D144` → descriptor `0x054D09E0` → `.?AVuGUI_MapBaseAndHerb@gui@game@app@@`. El registro DTI usa global `0x05595D8C`, nombre en `0x04DE830C` y tamaño `0x2C0`.

Su virtual de update (slot 8) es `0x01C28CAD → 0x02B61D60`.

## La ruta examinada obtiene Self

En `0x02B61F3C` llama a `0x01C2C7F4 → 0x01CB7400`, el buscador Self. Guarda el actor en `[ebp-0x44]`.

Posteriormente, la misma variable alimenta:

```
0x02B62195 → 0x01BCC327       pack del actor
0x02B621A0 → 0x01C2A51C       cantidad de hierbas
0x02B621A9 → 0x01BC6251       actualizar contador de este HUD
```

El getter de hierbas acaba en `0x0243CE90`, cuya instrucción en `0x0243CEB6` lee `[pack+0xC8]`.

El setter visual real `0x02B62880` compara el argumento con `[this+0x294]`; si cambia, lo guarda en `0x02B628B7` y llama a la actualización siguiente. La metadata en `0x02B6269A/0x02B626A0` asocia literalmente `+0x294` a `mHerbHaveNum`.

## No confundir Blur con jugador 2

Existe otra clase `uGUI_MapBaseAndHerbBlur`, con descriptor `0x054D0B10`. No es una etiqueta de jugador. Su setter emparejado `0x01B96FDD → 0x02B63DE0` no contiene la actualización del contador de la clase regular: acaba tras el prólogo/epílogo en `ret 4`. Esa ausencia no debe atribuirse al setter regular ni usarse para afirmar que existe un segundo HUD funcional.

## Consecuencia para la implementación

En el método auditado, cada instancia obtiene Self y actualiza un solo contador. Cambiar ese buscador a Sub0 sustituiría los datos de P1 en esa instancia; duplicar el dibujo sin separar el owner podría mostrar los datos de P1 dos veces. Esto es una conclusión sobre la ruta auditada, no un recuento probado de todas las instancias vivas del juego.

Antes de parchear faltan la creación/destrucción de instancias, el enlace a actor/viewport, la colocación del segundo HUD y su restauración en pausa/cambios de escena. No se ha declarado ni implementado aquí un HUD 2P completo.

Verificación reproducible: `tools/audit_herb_hud.py`, 25 comprobaciones de bytes/RTTI y preservación de esos mismos puntos en el candidato de hash `71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4`. Cuatro entradas negativas rechazadas. Ninguna ejecución de gameplay.
