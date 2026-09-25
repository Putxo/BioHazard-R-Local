# 32 — Vida nativa, reenlace PCS y límites del contador encontrado

Continuación de PR #9 desde `9e8dfec08842d8e199a59932074220cc5581f96c`. Análisis de solo lectura del original local de enero: 60.748.800 bytes, SHA-256 `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`. No se genera/modifica una imagen del juego ni se ejecuta el motor.

## Reenlace no equivale a nacimiento

`0x02DF4F30` guarda la referencia anterior PCS+44, la limpia en `0x02DF4FA7`, busca por serial y escribe la nueva en `0x02DF501F`. La continuación local existente ya modifica la limpieza y la asignación en `0x02DF4FA7/0x02DF5015`; no deben superponerse otros parches a esos sitios ni deshacer su frame ampliado.

Hay dos puntos independientes para observar el intervalo completo: `0x02DF4F99` antes de llamar a `0x01C65414`, y `0x02DF504B` después de `0x01C65419`. El segundo ocurre después de la asignación y conversión local existente. Una notificación aquí puede actualizar el rol, pero no demostrar por sí sola una vida nueva del actor. El mismo puntero puede reenlazarse sin haber sido destruido.

## Construcción y destrucción identificadas

| Evento | Implementación / punto |
|---|---|
| Final de construcción de uNpc | `0x0277D4FB`, dentro de `0x0277C040` |
| Final de construcción de uCockpitManagerMain | `0x02B47907`, dentro de `0x02B477A0` |
| Final de construcción de uMiniMapManager | `0x02B67F51`, dentro de `0x02B67F00` |
| Inicio del destructor real de uNpc | `0x0277DC70` |
| Inicio del destructor real de uPlayer | `0x027B5D00` |
| Inicio del destructor real de cockpit | `0x02B47A30` |
| Inicio del destructor real de minimapa | `0x02B68040` |
| Inicio del destructor real PCS Main | `0x02DF5970` |
| Inicio del destructor real PCS Sub0 | `0x02DF5F90` |

La cadena RTTI de `uPlayer` (`0x04D9CDF4`) incluye `uNpc` (`0x04D9B25C`). El destructor de uPlayer llama al de uNpc mediante `0x027B5D61 -> 0x01C3F4F8 -> 0x0277DC70`. Una observación que reciba ambos avisos debe invalidar una sola vez, no interpretar el aviso de la base como una segunda liberación. Las notificaciones se sitúan antes de la destrucción de recursos de cada cuerpo auditado, no después de liberar la memoria. Esto no demuestra cobertura temprana de todos los tipos derivados; el proveedor no debe admitir clases no cubiertas fingiendo esa prueba.

## El reloj localizado es de simulación, no de dibujo

En `sUnit::0x03268BB0`, el camino que supera una comprobación de pausa incrementa el par de DWORDs `+0x33D848/+0x33D84C` con ADD/ADC en `0x03268CCC..0x03268CEA`. La salida por pausa anterior evita ese incremento. El getter `0x03479BC0` lee la mitad inferior en `0x03479BD1`.

**No se usa este contador como ManagerFrame.frame de dibujo.** Puede detenerse mientras siguen existiendo presentaciones de pantalla; usarlo para deduplicar draw puede eliminar actualizaciones necesarias durante la pausa. Tampoco se usa `sMain+30`: su metadata lo nombra mFrameTimer y el código calcula tiempo por FPS, no una identidad inequívoca de presentación. Los contadores sMain+98/+9C y +A0/+A4 examinados son índices de colas, no el fotograma.

La fuente de vida puede producir Session/Actor y generaciones de gestores sin inventar un frame. El proveedor de presentación y el bloqueo/fence real del render siguen separados y pendientes. Un hook de GUI no se convierte por ello en permiso para construir o destruir.

## Continuación

Implementar avisos de nacimiento/destrucción y de comienzo/final de binding, captura coherente de actores ya observados, e invalidación inmediata del registro anterior. No resucitar por lectura de puntero ni aceptar clases con destrucción no cubierta. Conservar los gateways de managers y las operaciones del backend, sin activarlos con permisos constantes.
