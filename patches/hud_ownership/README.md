# Asociación HUD -> actor -> vista (solo fuentes)

Este módulo no instala hooks ni crea una segunda interfaz dentro del juego. Implementa y prueba el registro de ownership y tres puentes i386 que utilizará esa integración. No ejecutar constructores de EXE para probarlo.

## Contrato

El adaptador del motor debe registrar cada manager **después** de su construcción, verificar su vtable real, y registrar cada widget con su clase y actor reales. Mira/equipo pertenecen a Cockpit; mapa/hierbas a MiniMap. La clase Blur no es P2.

Los snapshots Session/Actor no se leen de direcciones arbitrarias: los proporciona un adaptador todavía pendiente. Ese adaptador debe garantizar actor vivo, serial correcto, ThinkMode::Pad, epoch de escena/sesión y lifetime incrementado en cada nueva vida del actor. Un serial o puntero por sí solo no detecta reutilización de memoria. Todas las llamadas deben producirse en el hilo del juego; este módulo no es thread-safe.

Solo se activa una asociación si existen dos actores locales válidos y distintos. No se crea un actor ausente. La pérdida de actor/contexto/sesión revoca el binding: una posterior activación con el mismo puntero no lo revive. Se exige retirarlo y registrarlo de nuevo. Los tokens incluyen generación; no pueden retirar otra instancia que haya reutilizado su slot.

Los widgets originales no registrados conservan Stock. Un widget de P2 revocado devuelve Hidden y actor nulo; jamás adopta Self como fallback. Un P1 vivo con modo local inactivo recupera Stock. Tras invalidar el manager ambos miembros se ocultan. El adaptador debe impedir nuevas llamadas, invalidar antes de destruir y retirar las entradas solo cuando ya no haya callbacks; el registro no hace segura la desreferencia de un objeto liberado.

## Dibujo y fases

`flags_for` devuelve una propuesta de mDrawView preservando todos los demás bits. No escribe el flag del objeto. La integración pendiente deberá aplicar/restaurar esa vista de forma acotada, no sobrescribir permanentemente el estado original. No implementa escalado, posición, clipping ni proyección de las dos mitades.

La lista stock de cockpit se reconstruye desde 21 slots conocidos y su destructor libera slots explícitos. No introducir clones ajenos en esa lista. MiniMap mantiene cuatro unidades propias y un alias +40 del primer elemento +30: no liberar ambos como objetos distintos. Los widgets de mapas no admiten el layout de next/ForceSkip del cockpit.

Reticle consulta el actor en slot9; equipo y hierbas lo hacen en slot8. Los puentes sustituyen solo llamadas concretas a Self: no vuelven a invocar las fases ni cambian el finder global. `integration.json` conserva callsites y vtables exactos.

## Puentes

`self_bridges.S` mantiene el EBP del caller, lee su widget desde `[ebp-8]`, conserva ECX y el argumento opaco del finder y devuelve el actor registrado o un nulo comprobado por los guards originales. En modo Stock hace tail-call al símbolo `rev_hud_stock_self`; la integración final deberá resolverlo al thunk original 0x01C2C7F4. **No resolverlo al hook mismo**, ni instalarlo globalmente.

La suite i386 ejecuta los puentes reales con el registro real, pero sustituye el finder del motor por un mock. No prueba recursos GUI, callbacks multihilo ni gameplay. Las direcciones sintéticas de actores son claves, no punteros desreferenciados por el registro.

## Pruebas sin archivos del juego

Desde la raíz del repositorio:

```
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Ipatches/hud_ownership patches/hud_ownership/ownership.cpp tests/hud_ownership/registry_tests.cpp -o /tmp/hud-registry
/tmp/hud-registry
python tests/hud_ownership/test_audit.py
```

La prueba opcional con el EXE original requiere `BHR_ORIGINAL` y solo lee su contenido. La CI no recibe ejecutables del juego ni sube binarios de tests. Los comandos i386 completos están en `.github/workflows/hud-ownership.yml`.

Pendiente antes de usar el módulo en el juego: creación de recursos/instancias duplicadas, adaptador de lifecycle, instalación selectiva de bridges, dispatch por fase, transformaciones del viewport y prueba dentro de la campaña. Pausa/inventario globales y scheduler no se modifican aquí.
