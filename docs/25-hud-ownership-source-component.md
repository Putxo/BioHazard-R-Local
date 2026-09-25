# 25 — Asociación por instancia implementada como componente, aún no instalada

Continúa docs/24 y preserva la base consolidada `5118850441bf70ad8d6da2bd7d7c6505ab7bd234`. Entrega exclusivamente fuentes, tests y evidencia en GitHub. No se generó ningún nuevo EXE del juego.

## Qué se ha añadido

`patches/hud_ownership/ownership.cpp` implementa un registro explícito de manager, tipo de widget, actor, serial/lifetime, miembro y vista. Distingue las familias Cockpit/MiniMap según la herencia comprobada. Su capacidad es acotada y rechaza duplicados, clases erróneas, vistas incoherentes y vínculos incompletos en vez de corromper otra instancia.

La invalidación es persistente: si se pierde la sesión o cambia la identidad/lifetime del actor, el widget P2 queda Hidden y no retorna el actor Self. Volver a activar el flag o reutilizar la dirección no resucita un vínculo antiguo. Los tokens de generación impiden retirar una instancia nueva con un token anterior. La integración de lifecycle deberá invalidar antes de destruir; este componente no libera unidades del motor.

Tres puentes de `self_bridges.S` están escritos para las llamadas verificadas `0x02B404C6`, `0x02B3B6C8` y `0x02B61F3C`. Preservan el frame del caller, su ECX y su argumento opaco de Self. Reticle usa slot9, equipo/herbas slot8. Los callers originales comprueban puntero nulo antes de utilizar el actor/pack en las rutas auditadas. Los puentes NO están instalados en ninguna imagen.

El componente calcula mDrawView para cada instancia conservando los otros bits del objeto. No modifica mDrawMode, Self, input, la lista nativa ni las coordenadas globalmente. La aplicación scoped de flags y la transformación de pantalla aún requieren el adaptador.

## Cierre adicional del owner de mini-mapa

La vtable `0x04DE86FC` tiene COL `0x0537D3F4` y descriptor `0x054D0D50`, cuyo nombre es `uMiniMapManager`. Sus fases son `0x02B68430` (slot8), `0x02B68590` (slot9) y `0x02B686A0` (slot11). El draw vuelve a consultar mDrawView en `0x02B687BB` y filtra por el índice de vista, como el cockpit.

`uGUI_MapBaseAndHerb` está en su array `+0x30` y también en el alias `+0x40`. El destructor itera cuatro elementos; el alias no debe liberarse como quinto elemento. La jerarquía RTTI del widget no contiene uBioCockpitGUI. Por eso el registro separa ambos gestores y no usa `+0x290/+0x294` como enlaces cockpit en el HUD de hierbas.

## Pruebas ejecutadas

`scripts/audit_hud_lifetime.py` pasó 94 comprobaciones sobre el original de enero (SHA fijado); son witnesses estructurales de RTTI, instrucciones, slots y calls, no pruebas de gameplay. Su suite pasó cinco tests locales, incluidos rechazos de imagen y protección de salida. El original permaneció intacto.

El registro pasó nueve grupos de pruebas C++ y UBSan, incluidas todas las 1.024 máscaras de vista y su preservación de otros bits. Las 61.709 aserciones proceden mayoritariamente de esa matriz de bits, no de miles de escenas de campaña.

CI de fuentes `0dc413931f21b192fb9f1fe37d5630e693824d53`, run `36176901629`: dos jobs correctos. El job i386 `108209599618` ejecutó 16 escenarios con los bridges y el registro reales, comprobando actor, caller frame, registros y pila. El finder original se sustituyó por un mock; no se ejecutó ninguna función del motor. El contenedor local no ejecuta ELF32 (Exec format error), por lo que no se atribuye a él esa ejecución.

## Qué NO está cerrado

Este es código de asociación probado, **no un segundo HUD funcionando dentro del juego**. Faltan creación de recursos e instancias, adaptador que produzca snapshots fiables y gestione su ciclo de vida, instalación selectiva de puentes, dispatch y transformación/clipping para cada mitad. Pausa, inventario, scheduler y escenas sin partner conservan sus pendientes.

Siguiente paso preciso: seguir la inicialización de recursos de Reticle `0x02B402F0`, MainEquipWin `0x02B3B280` y MapBaseAndHerb `0x02B61A70`, más el registro de unidades/grupos de los gestores. Resolver colisiones de identificadores de GUI y callbacks antes de publicar las nuevas instancias al render. No tratar la lista fija de 21 widgets como almacenamiento expandible ni copiar el layout cockpit al mini-mapa.
