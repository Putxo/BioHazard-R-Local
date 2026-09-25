# Estado actual — 25 de septiembre de 2026

## Resultado real

Hay un candidato nuevo construido y comprobado por componentes. **El cooperativo completo pedido por el usuario todavía no está terminado ni validado dentro del juego.** La siguiente tarea no debe empezar otra vez en el DTI de uItem, ni confundir pruebas de lógica con una partida de campaña.

```text
Original enero: 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69
Base owner-fix: e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a
Último candidato: 0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378
Tamaño: 60.755.456 bytes
```

Constructor: `patches/build_local_routing.py`. Módulo: `patches/local_routing/{core.cpp,hooks.S,link.ld}`. Informe: `docs/20-local-routing-built-and-tested.md`.

## Implementación añadida y alcance

| Sistema | Estado del candidato |
|---|---|
| Reenlace de Sub0 | Conserva la sesión solo al reencontrar el mismo actor previamente local; conversión Cpu→Pad nativa; Network excluido |
| Door gimmick | Une sensor elegible, serial de WaitState, cinco lookups y callback de miembro; requiere probar orden/lifetime real del motor |
| Pickup | Callback real de uItem `0x024605D0`; no confundirlo con uObjModel `0x026D7AD0`; arbitraje de elegibilidad de ambos jugadores aún limitado |
| Ayuda | Disponibilidad, mando e inventario usan el mismo actor contrario en la ruta auditada; no es una revisión completa de muerte/game-over |
| ActionCommand | Cuatro montajes con actor identificado adaptados; se preservan los demás delegates |
| Cámaras/pantalla partida | Se heredan los hooks anteriores; no se ha demostrado render correcto en toda la campaña |
| HUD/menús por jugador | No implementados de forma completa |
| Checkpoints, cutscenes y escenas sin partner | No resueltos de forma completa; no se crea un P2 donde no existe un partner |

## Pruebas reales, no simulación de resultados

La lógica C++ pasó 204 aserciones por ejecución con funciones del motor simuladas. Se ejecutó en host local, con ASan/UBSan, en Linux i386 y en Windows x86.

Los nuevos puentes x86 pasaron 23 escenarios en un intérprete limitado a sus instrucciones: 312 aserciones desde el PE, 311 sin la comprobación inicial del hash del PE. Las llamadas al motor y al C++ están simuladas en ese intérprete; no es emulación del juego.

La integridad del archivo pasó 153 aserciones y siete casos negativos de CLI. La recompilación local reproduce los mismos bytes; revertir las modificaciones y quitar las dos secciones nuevas reconstruye exactamente la base owner-fix.

CI ampliada verificada: `342c3485115178a5278af15c0ee994236306633e`, run `36122601817`, cuatro jobs correctos. Windows pasó además las 12 pruebas del probe, incluida una lectura real mediante kernel32 del propio proceso de pruebas. **No se ejecutó Resident Evil Revelations en esos jobs.**

El JSON histórico del test C++ conserva `win32_abi_executed:false`: no certifica la ABI del motor real. El log de Windows acredita el binario de pruebas x86 con mocks, no el EXE del juego.

## Continuación exacta

Tres selectores de miembro cero adicionales están en capas genéricas: `cFsmAction::0x02976C30`, `cFsmActionPcs::0x029FF600` y `uPcsInput::0x02DEE8D0`. Primero determinar el actor/ownership del comando de guion; no sustituir todos los ceros por P2 ni aceptar ambos mandos indiscriminadamente.

Faltan también ownership del HUD y pausa/inventario, flujo completo de muerte y checkpoint, cámaras forzadas y escenas sin partner. La mera presencia de clases/strings de esos sistemas no es una implementación.

Para validación dentro de la build se necesita una instalación que ya arranque con los datos compatibles de enero. Los EXE aportados no contienen los escenarios, personajes, interfaces y demás datos de una instalación completa. El [probe](docs/runtime-probe.md) sirve para registrar actor/mandos/cámaras sin modificar memoria; todavía no existen resultados de una partida con ese probe.

## Rectificaciones que no deben perderse

- v12 alteró inicialmente el callback de uObjModel, no el de uItem. La rectificación está en documentos 17/18 y en el candidato owner-fix.
- v14 corrigió el botón final de door_gimmick, pero no bastaba para resolver selección/serial; esta continuación añade esa cadena.
- MainEquipWin/SubEquipWin no identifican P1/P2: son equipo principal/secundario.
- El experimento histórico DOOR WAIT PAD2 no es la v14 canónica de la cadena anterior.
- No activar control local de un actor Network; no cambiar globalmente GameMode, Self o el serial de red.
- Un hash correcto acredita identidad/reconstrucción, no jugabilidad.

Los anteriores handoffs se conservan byte por byte en `docs/history`. Leer este estado antes de seguir indicaciones históricas.
