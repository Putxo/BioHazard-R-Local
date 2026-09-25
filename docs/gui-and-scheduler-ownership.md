# HUD y uPcsInput: propiedad del actor, sin cambiar campos a ciegas

Auditoría local sobre el original de enero SHA `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`. Continúa después del candidato SCRIPT EXPERIMENTAL. **No se creó un parche de HUD en esta auditoría.**

## uPcsInput: el padre es un uScheduler

El scan de la base uPcs en `0x02DEDF30` busca una unidad que contenga a este objeto entre sus unidades dependientes y la almacena en `+0x30`. La conversión usa `0x01C2DE24 → 0x02DF1AF0`.

Ese checked cast empuja DTI `0x0579963C` en `0x02DF1B60`. El registro del DTI en `0x04B00AC0` lo vincula literalmente a `uScheduler` (string `0x04EC5D84`, referencia `0x04B00ACF`). **No es un puntero de jugador demostrado.**

Esto concreta el límite anterior: no basta leer `uPcsInput+0x30` para escoger el mando de un actor. El selector `0x02DEE8D0` queda stock hasta demostrar una asociación no ambigua entre scheduler, comando y participante. La corrección de los dos callbacks cFsmAction usa otra cadena, la clase Sub y su contexto/serial verificado en `+0x1078/+0x1074`.

## El HUD consulta Self explícitamente

Cuatro consumidores comprobados llaman al finder `0x01C2C7F4 → 0x01CB7400`:

| Consumidor | Llamada a Self | Observación |
|---|---|---|
| uGUI_Reticle, update `0x02B40480` | `0x02B404C6` | Luego usa el pack del actor seleccionado en `0x02B4054D` |
| uGUI_MainEquipWin, update `0x02B3B670` | `0x02B3B6C8` | Pack obtenido en `0x02B3B6DE` |
| uGUI_MapBaseAndHerb, update `0x02B61D60` | `0x02B61F3C` | Pack obtenido en `0x02B62195` |
| uGUI_PauseHD, update `0x02C309C0` | `0x02C30A06` | Evalúa estado del actor y puede activar su byte `+0x2AC`; esto no identifica todavía el propietario de todo el input del menú |

No se deduce Self solo por el nombre de una nota antigua: el finder llama al predicado `0x01BB3192 → 0x01CB7560`, que compara el serial candidato con el serial de Self y exige su estado válido. La comparación está en `0x01CB759E`.

Duplicar únicamente los viewports del mundo no modifica esos consumidores. Tampoco debe cambiarse el finder global, porque afectaría al resto del juego.

## Clases y creación localizadas

RTTI de `uCockpitManagerMain`: vtable `0x04DE5C14`, COL `0x0537C8E8`, descriptor `0x054CEFA4`. Su constructor `0x02B477A0` instala esa vtable y pone a cero sus slots de widgets.

La creación de widgets en `0x02B48510` reserva/construye un `uGUI_Reticle` mediante `0x02B48F33 → 0x01C19CCB → 0x02B40140` y lo guarda en **manager+0x90**, en `0x02B48F53`. Este dato es por instancia: no demuestra por sí solo cuántos managers se crean globalmente.

`uGUI_Reticle` y `uGUI_MainEquipWin` derivan de `uBioCockpitGUI` y llaman a su constructor `0x01BEF075 → 0x02B79700`. La base instala vtable `0x04DE98B4`, con RTTI propio, y su metadata expone `Active` y `Force Skip`. No hay evidencia aquí de un selector de jugador oculto en el argumento cero del constructor.

Vtables verificadas adicionales:

- Reticle `0x04DE52C4`.
- MainEquipWin `0x04DE4C3C`.
- MapBaseAndHerb `0x04DE810C`.
- PauseHD `0x04DF5E5C`.

MainEquipWin/SubEquipWin continúan siendo equipo principal/secundario; no se renombran como interfaces P1/P2.

## Evidencia reproducible

`scripts/audit_gui_ownership.py` pasó **29 aserciones de bytes, RTTI y destinos de llamada** sobre el original real. El informe está en `research/reports/gui-ownership-audit.json`. Es una auditoría de solo lectura con hash de entrada obligatorio, no una prueba de render ni una modificación del juego.

## Siguiente trabajo concreto

Seguir el registro, ciclo de vida y composición de los widgets creados por `0x02B48510` y los métodos `0x02B492D0 / 0x02B49A60 / 0x02B49DA0` de uCockpitManagerMain. Determinar cómo asociar cada instancia con un actor y un viewport sin colisionar IDs de interfaz o recursos compartidos. Después separar la propiedad de pausa/inventario del estado global de pausa del motor.

No reemplazar globalmente Self; no modificar el tamaño/coordenadas de toda la interfaz sin comprobar el render y las transformaciones. Muerte, checkpoints, cámaras forzadas y escenas sin partner siguen abiertos: esta auditoría no los convierte en hechos resueltos.
