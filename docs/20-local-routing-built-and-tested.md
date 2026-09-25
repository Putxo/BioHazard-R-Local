# 20 — Continuación implementada y comprobada por componentes

Fecha: 2026-09-25. Este documento sustituye las afirmaciones de alcance de los checkpoints anteriores. No acredita cooperativo completo ni una campaña jugable validada.

## Candidato construido realmente

Base exacta: owner-fix sobre v14, SHA-256 `e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a`.

Salida **LOCAL ROUTING EXPERIMENTAL**:

```text
SHA-256 0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378
Tamaño 60.755.456 bytes
159 bytes existentes modificados
6.656 bytes añadidos
```

El módulo original ocupa `.lcfix` RX, VA `0x057E2000`, y su tabla de bindings `.lcdata` RW, VA `0x057E3000`. No se añade una sección simultáneamente escribible y ejecutable. Se conserva el ImageBase fijo y el punto de entrada del EXE de enero. No se modifica el ejecutable original ni se sube a GitHub.

El constructor es `patches/build_local_routing.py`. Sus argumentos son una base exacta, una salida nueva y `--report` con un informe nuevo. Compila `core.cpp` y `hooks.S` mediante GNU g++/as/ld. Los cambios de compilador pueden cambiar los bytes del módulo; la identidad binaria documentada corresponde al compilador del manifiesto generado, no a cualquier compilador imaginable.

## Cambios de código incorporados

### 1. Reenlace de Sub0 sin perder la sesión local

La rutina `0x02DF4F30` limpia el binding antes de reencontrar al actor. La línea anterior borraba el flag local y después podía encontrar a ese mismo actor ya en Pad, sin restaurarlo.

El nuevo código conserva el flag/tracker anteriores y el actor previo. Solo restaura el modo local cuando coinciden el mismo actor, su binding previo y el tracker previo. Un actor nuevo en Cpu pasa por el setter nativo Cpu→Pad. Network no se convierte en local.

Para los dos temporales nuevos se ajustan las cuatro partes del frame: reserva, comienzo y longitud del relleno debug, y liberación. No se cambia solo el tamaño de reserva.

### 2. Cadena completa de selección de door_gimmick

La modificación ya no se limita al botón final de v14. Une:

```text
sensor original -> candidato elegible exacto Self/Sub0
 -> serial de WaitState
 -> cinco búsquedas posteriores del actor
 -> callback real de uObjModel
 -> consulta del mando del actor
```

Los hooks están en la selección `0x025902C4/0x0259035D/0x025903EE`, en la entrada de WaitState `0x02590072/0x02590097/0x025900B4`, y en cinco callsites de lookup `0x02590DD3/0x02590FA0/0x0259681D/0x02596C02/0x02596F3F`.

Se conserva el sensor del motor: no se amplía el radio ni se admite cualquier NPC. Se valida elegibilidad, coordenadas finitas y seriales distintos dentro del rango que puede transportar el estado. Los índices globales de red y el finder global no se sustituyen.

El callback común de uObjModel solo deriva miembro 1 para una puerta registrada, de tipo verificado, cuyo actor elegido sea el Sub0 local exacto. Nunca interpreta su `+0xF48` como el layout de uItem.

La tabla de 512 entradas y el orden sensor/callback siguen necesitando pruebas de ciclo de vida dentro de escenas reales. Que los mocks pasen no prueba que el motor invoque todo en el orden esperado.

### 3. Recogida y selector del actor con guards

Se conserva la rectificación de owner: el callback de uItem es `0x024605D0`, no `0x026D7AD0`. El segundo es el de uObjModel.

El selector ahora exige flag local y ThinkMode Pad del Sub0. No basta con que el tracker exista. El segundo montaje de ActionCommand de uItem en `0x024614C7` también enlaza `0x01C60DD3 -> 0x024605D0`: queda cubierto por el mismo callback corregido.

No se ha eliminado la limitación anterior de arbitraje de pickups por distancia antes de toda la elegibilidad del motor.

### 4. Ayuda entre los dos actores

`cVitalControl` conserva a su actor en `+0x0C`. Los dos lookups en `0x0276D1BD` y `0x0276D38E` deben elegir el mismo otro actor para la disponibilidad y el mando que confirma la ayuda.

El callback exitoso pasa por `0x0276D480 -> 0x02798F00`. Sus lookups `0x02798F72/0x02798F9A` se ajustan también, de forma recíproca, sin sustituir Self globalmente.

La ruta nativa obtiene el pack del otro actor, comprueba hierbas con `0x0243A0A0` y consume una en la ruta normal de `0x0243B1B0`, sobre `cBioItemPack+0xC8`. La ayuda no debe confirmar con el mando de uno y consumir el inventario de otro distinto.

Esto corrige selección de actor/mando/inventario en esa ruta de ayuda. **No demuestra que toda muerte, game-over, reanimación o checkpoint de campaña sea correcta.**

### 5. Cuatro rutas de ActionCommand ligadas al actor

Se preservan cuatro de los cinco delegates y se cambia únicamente el selector de miembro en los montajes auditados `0x02706B37`, `0x027088E9`, `0x026E9B53` y `0x026EC5B3`.

En las dos inicializaciones de componentes se conserva el actor antes de su posible conversión a Pad; la decisión de local/P2 se toma cuando se invoca el callback. Se mantienen los mecanismos originales de copia y vida del delegate, que no liberan el puntero de contexto.

## Pruebas ejecutadas

| Prueba | Resultado y alcance |
|---|---|
| Lógica C++ | 204 aserciones por ejecución, con servicios del motor simulados |
| Sanitizadores | La misma lógica pasa ASan/UBSan |
| Puentes assembly | 23 escenarios; 312 aserciones desde el PE generado, 311 sin el hash inicial del PE |
| Integridad del EXE | 153 aserciones; cambios declarados, destinos, cabeceras, directorios y código preservado |
| Rechazos CLI | 7 escenarios: aliases, archivos existentes y build incorrecta, sin sobrescribir entradas |
| Reconstrucción | Compilación local independiente reproduce el mismo EXE; reversión exacta a la base |
| Probe | 12 pruebas en Windows, incluida lectura real de un buffer del propio proceso de pruebas |

CI inicial de tres plataformas: run `36121464808`, commit `0b30942ae8d3175fc7ef9727a6832a679d05f527`, correcto.

CI ampliada: run **`36122601817`**, commit **`342c3485115178a5278af15c0ee994236306633e`**, **cuatro jobs correctos**: native, sanitizadores, Linux i386, y Windows x86 con mocks más el probe de solo lectura.

En Windows se compiló y ejecutó el código de pruebas con `clang-cl --target=i686-pc-windows-msvc`; pasó las 204 aserciones. El campo histórico `win32_abi_executed:false` de la salida del test no es una prueba de ejecución del motor: este continuó simulado. El log del job muestra la ejecución del programa de prueba de Windows, no la del EXE del juego.

**Ninguna de estas ejecuciones arrancó Resident Evil Revelations ni utilizó assets propietarios en GitHub Actions.**

## Auditoría de selectores restantes: no confundir guion con actor

Se comprobaron los otros montajes directos del setter de ActionGroup. La entrada de jugador `0x02787160` usa `0x01C49B24 -> 0x027A2390`, que ya llama al selector del actor; no necesita un segundo parche.

Tres callbacks devuelven todavía cero, pero pertenecen a capas genéricas:

| Callback | Owner demostrado por RTTI | Binding |
|---|---|---|
| `0x02976C30` | cFsmAction; vtable `0x04DBD47C` | seis comandos desde `owner+0xA0`, montaje `0x029708CE` |
| `0x029FF600` | cFsmActionPcs; vtable `0x04DC7AD8` | seis comandos desde `owner+0x860`, montaje `0x029FD3F5` |
| `0x02DEE8D0` | uPcsInput; vtable `0x04E15EF4` | ActionCommand `+0x50`, montaje `0x02DEE629` |

No se ha demostrado qué actor debe controlar cada invocación de esos comandos de escena. Cambiarlos indiscriminadamente a P2 o aceptar ambos mandos podría hacer avanzar el mismo guion dos veces. **Son una investigación pendiente, no una compatibilidad certificada.**

## Cámaras, interfaz, cambios de escena y ausencia de partner

El getter de sGameCamera resuelve el singleton en `0x05799D3C`. El getter de sGamePad `0x01C8B7F4 -> 0x01CB2EF0 -> 0x01CB2F40` lee `0x057A7480`. Estas rutas se han verificado para el probe.

En el bloque de cámaras, `0x020413DD` obtiene un selector del actor antes de consultar el pad; no es un cero universal. Otras ramas de cámara debug están deshabilitadas por una escritura previa y no deben parchearse por una búsqueda ciega de `push 0`.

No se ha implementado un HUD P2 completo ni menús/inventarios simultáneos independientes. MainEquipWin/SubEquipWin significan arma principal/secundaria, no jugador 1/2. Los tipos de pausa, checkpoint y cámara existen, pero su existencia no prueba soporte local completo.

El nuevo reenlace ayuda a conservar la sesión cuando vuelve el mismo actor. No crea un partner en una escena que carece de él, no garantiza guardar/restaurar su inventario en checkpoints y no resuelve por sí solo todas las cámaras forzadas/cutscenes.

## Siguiente estado de trabajo

No volver a v12 como prueba de pickup correcto, ni a v14 como prueba de toda la selección de puertas. La base de esta continuación es el candidato de SHA `0c019d43...`, no una versión marcada como terminada.

Pendientes: recepción de observaciones dentro del juego con assets compatibles; ownership de comandos genéricos FSM/PCS; HUD/menús por jugador; lógica completa de muerte y checkpoints; cutscenes y creación/persistencia de P2 en escenas sin partner. Los cambios adicionales deben apoyarse en una ruta demostrada, no en convertir todos los ceros o todas las búsquedas Self globalmente.
