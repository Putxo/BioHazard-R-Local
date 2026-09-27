# Composición del runtime e inventario thiscall

27/09/2026. Base: PR23, `42eed08a2c675c7aceae27c3a923307ca969df3c`.

`patches/runtime/runtime.cpp` construye en un único grafo los módulos de HUD,
admisión, recursos, lifetimes, ventanas estructurales, fases, máscara P1 y menú.
Los permisos de asignación vienen de StructuralWindow y AllocationLedger. Los
callbacks diferidos resuelven las dependencias circulares sin acceder a objetos
antes de que estén construidos. El registro empleado por los bridges es el mismo
que usa Lifecycle.

Faltaba seleccionar gestores en producción: ninguna ruta llamaba a
`LifetimeSource::select_managers`. Ahora el END lee `sIDCockpit[0]`, selecciona
sus gestores observados y retira la selección si desaparece el owner o cambian
sus campos. No inventa nacimientos a partir de punteros encontrados.

Evidencia estática en el original January con SHA
`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`:

- `01F3EF26` llama `01BE32C5` con índice 0 antes de comprobar state 2.
- `01BE32C5 -> 01CB57A0` obtiene el slot mediante `01C11AEE` y lo desreferencia.
- `01C11AEE -> 01CB6A80`; `01CB6ADE` devuelve `055623C4 + index*4`.
- Los gestores pertenecen al owner en `+28` y `+2C`.

Se encontró también un fallo de ABI anterior: `rev_menu_gate_submenu_actor`
llamaba al finder sin reenviar el predicado y terminaba con `ret`. El finder
stock `01C2C7F4 -> 01CB7400` usa `[ebp+8]` en `01CB747D` y termina con `ret 4`
en `01CB74B9`. Ahora el bridge reenvía el argumento conservando ECX y limpia
exactamente el argumento original. La prueba anterior usaba un stub `ret` y
restauraba ESP antes de comprobarlo; ahora modela el contrato real.

El host x86 usa VirtualQuery y GetCurrentThreadId a través de los imports
existentes (`057DB034`, `057DB214`), valida permisos de cada lectura/escritura,
arranca antes del entry original y ejecuta los inicializadores C++ de su módulo.
Un fallo de arranque detiene el entry antes de alcanzar gateways incompletos.
`tools/build_runtime.py` enlaza el módulo completo sin dependencias externas
sin resolver; no abre, modifica ni ejecuta un juego.

Validación local: integración Win32 11 escenarios/711 assertions PASS;
ocho gateways de menú PASS ejecutados como test sintético x86 en emulador;
enlace ELF32 freestanding completo PASS con LLVM 22.1.8.

Esto no completa el cooperativo. Falta el instalador acumulativo verificado,
ampliar el HUD más allá de los tres tipos existentes, Genesis y restantes
acciones/scripts, muerte/checkpoints/cutscenes y escenas sin partner. El END
observado es una frontera de CPU; no se ha demostrado un fence GPU adicional.
El juego no se ha abierto ni ejecutado y la comprobación jugando corresponde
al propietario por petición expresa.
