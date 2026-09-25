# 24 — HUD: ciclo de vida, fases y filtro por vista

Continuación de `docs/gui-and-scheduler-ownership.md`, sobre la rama consolidada `5118850441bf70ad8d6da2bd7d7c6505ab7bd234`. El original de enero se ha leído localmente y su SHA-256 calculado coincide con `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`; tamaño 60.748.800 bytes. No se ha ejecutado ni modificado el EXE.

## 1. La lista del cockpit no es una colección de jugadores

`uCockpitManagerMain::0x02B4C470` construye un array temporal de 21 widgets: los slots `+0x3C..+0x88`, cada cuatro bytes, y `+0x90`. Excluye `+0x8C`, que tiene un registro separado al final.

La función busca el siguiente elemento no nulo y lo enlaza mediante `0x01C52F1C -> 0x02B4C6F0`. La instrucción `0x02B4C719` escribe `widget+0x290 = siguiente`. El último enlace se pone explícitamente a cero. El getter `0x01C935B7 -> 0x02B499E0`, usado por los recorridos, lee ese mismo campo en `0x02B49A06`.

La creación del gestor llama a esta construcción de lista en `0x02B48F80`. Añadir un widget solo al final de la lista sería insuficiente: una reconstrucción posterior puede eliminar ese enlace. El destructor `0x02B47A30` libera los slots individualmente mediante su virtual cero con argumento 1, no recorriendo esa lista como una colección genérica de clones. Cualquier widget adicional necesita ownership y teardown propios; no debe liberarse dos veces.

## 2. Tres fases diferentes

La vtable del manager `0x04DE5C14` contiene:

| Slot | Thunk | Implementación |
|---|---|---|
| 8 | `0x01C08CFA` | `0x02B492D0` |
| 9 | `0x01BA3F94` | `0x02B49A60` |
| 11 | `0x01C756DE` | `0x02B49DA0` |

Los recorridos ordinarios llaman respectivamente al virtual `+0x20`, `+0x24` y `+0x2C` de cada widget. Las ramas especiales de modo de interfaz no recorren necesariamente todos los widgets.

**Rectificación de precisión:** la consulta Self de Reticle en `0x02B404C6` está en el virtual 9 (`0x02B40480`), no en su virtual 8. MainEquipWin consulta Self en su virtual 8. No se deben ejecutar ambas rutinas como si fueran una única fase de update duplicada.

## 3. mDrawView está demostrado por metadata y consumidor

`0x01BE2D20 -> 0x0279D1A0` devuelve `(unit[+0x0C] >> 16) & 0x3FF`. La metadata en `0x0347823F..0x0347824D` asocia ese getter a la cadena **mDrawView** (`0x04F145F8`), no a un pad ni a mDrawMode.

El setter `0x01C61814 -> 0x01EB6130` sustituye solo esos diez bits y conserva el resto de `unit+0x0C`:

```
newFlags = (oldFlags & 0xFC00FFFF) | ((viewMask & 0x3FF) << 16)
```

El draw del manager obtiene un índice del contexto con `0x01C322FD -> 0x02B49ED0` (`context+0x158 & 0xFF`), calcula `1 << index`, y en `0x02B49E21..0x02B49E29` exige que intersecte con mDrawView antes de llamar al virtual de dibujo. Esto demuestra una selección por vista ya presente; no demuestra que las coordenadas/proyección de la GUI se ajusten automáticamente a cada mitad de pantalla.

## 4. Mapa/hierbas tiene otro propietario y otro layout

La cadena RTTI de `uGUI_MapBaseAndHerb` (`0x04DE810C`) contiene `uBioGUI`, pero **no uBioCockpitGUI**. Las cadenas de Reticle y MainEquipWin sí contienen ambas bases. Por ello no es válido aplicar a mapa/hierbas los campos cockpit `+0x290` (next) y `+0x294` (Force Skip). El `+0x294` de mapa/hierbas ya se identificó como su contador mHerbHaveNum.

Se ha seguido el constructor `0x01C6392A -> 0x02B61890`. Su caller `0x02B681A7`, dentro de `0x02B68150`, guarda la instancia en `owner+0x40` (`0x02B681C7`) y también en el primer elemento del array `owner+0x30`. La misma función inicializa cuatro unidades. El assert de `0x02B68323` sitúa esta lógica en `mini_map/uminimapmanager.cpp`.

El destructor `0x02B68040` itera cuatro entradas `+0x30 + i*4`, llama a sus destructores y las limpia. La vtable de ese owner es `0x04DE86FC`; se continúa su RTTI y sus fases antes de diseñar el enlace definitivo.

## Estado de este checkpoint

Nuevas evidencias de análisis estático; todavía no se ha instalado un segundo HUD. Se conserva toda la implementación anterior. No se ha generado EXE, instalador ni paquete binario.

Siguiente trabajo: implementar una asociación explícita widget/tipo/owner/actor/vista con invalidación por ciclo de vida, sin escribir el layout de otra clase ni reemplazar Self globalmente. Separar el ownership del mini-mapa del cockpit y mantener la pausa fuera de esa duplicación hasta demostrar sus consumidores.
