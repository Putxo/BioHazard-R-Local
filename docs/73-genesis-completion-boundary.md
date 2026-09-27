# Genesis: retorno seguro de la finalización del objetivo

27/09/2026. Continúa PR44. Juego nunca ejecutado.

La función de escaneo 02B25A10 llama al virtual+14 del objetivo en 02B26B63.
Después vuelve a consultar la entrada que tenía en la pila. Una retirada del
objetivo dentro de esa llamada podía invalidar tanto ese puntero como el estado
que aún usarían los dos llamadores. Se protegen tres fronteras:

| Sitio | Frontera | Salida cuando se retira un objetivo o falla la validación |
|---|---|---|
| 02B26B60 | Carga y llamada al virtual+14 | Epílogo del escaneo 02B26E7E |
| 02B2F8ED | Estado llama al procesamiento | Epílogo del estado 02B2F9C2 |
| 02B1FD94 | Fase9 llama al estado de escaneo | Cola RTC de fase9 02B201BF |

El Scope del jugador, la fase9 del Scanner y esta notificación son ámbitos
distintos. El nuevo permiso de limpieza existe solo mientras la llamada virtual
de finalización está en curso dentro de la fase9 admitida del clon. No se relaja
la exclusión del backend para productores, otras fases o destrucciones normales.
La guardia anidada restaura el valor anterior de busy al terminar la limpieza.

Antes de invocar se exige un TargetKey vivo, pertenencia a la colección privada,
owner no nulo y una de cinco parejas exactas de vtable/método verificadas en enero.
Las lecturas de objetivo por vista quedan suspendidas durante el callback del
mundo. La ruta del scanner original conserva la llamada virtual original.

El destructor sigue revocando la generación y limpiando J2 en la frontera donde
target+8 ya es cero. Si ocurre durante la notificación protegida, se admite la
retirada nativa y se comprueba que solo desaparezcan sus referencias e iconos.
Una retirada correcta conserva el HUD vivo, pero obliga a abandonar los tres
marcos nativos para que no reutilicen cachés. Un fallo de limpieza, identidad,
recursos o reentrada pone el clon en cuarentena y propaga la misma salida.
El marcador de salida solo pertenece al clon y termina con su fase.

El estado de mundo +20 y la disponibilidad de escaneos +38 de ciertos objetivos
siguen siendo compartidos: no se copian ni se reinician para obtener recompensas
duplicadas. Este bloque conserva la semántica del método virtual y no constituye
todavía una política completa de premios o efectos del J2.

## Alcance pendiente

Se protege esta frontera concreta; **no toda destrucción reentrante**. Siguen por
revisar otros callbacks del escáner, vida de armas/owners dentro de los métodos
nativos, efectos, recompensa al cerrar y activación de Genesis. No se demuestra
seguridad contra destrucción concurrente desde workers. La revocación fuera
del hilo propietario sigue invalidando la admisión. Scanner continúa inactivo
por inicialización: este cambio no anuncia Genesis utilizable ni co-op completo.

## Validación

- Runtime:121 escenarios /25347 comprobaciones. Cinco parejas de clase/método,
  aislamiento del callback, retirada del objetivo actual y de otro objetivo,
  retirada defectuosa, owner revocado, reentrada, muerte sin limpieza, cambio
  de owner y evento de otro hilo. Retirada fuera de la frontera sigue rechazada.
- ABI x86 sintética anidada: tres niveles de pila; marcos alineados mediante
  EBX, marcos EBP ordinarios, RET/RET4, argumento1, registros, retornos normales
  y abandono de todos los consumidores posteriores. Solo código de prueba.
- 22 testigos estáticos en el original fijado por SHA256.
- Enlace freestanding y cuatro pruebas del instalador:115 hooks y reversión exacta.
- El ejecutable sintético MSVC utiliza una reserva de pila de8 MiB para sus
  fixtures de memoria, equivalente al límite usual de las pruebas Linux.

SHA256 módulo: `a86905218df2eb3cdf0c419042962890faaee0375b8473f2638b2526ca1b70f9`.
SHA256 copia separada: `e1245932571cd307796d443e08cce6e74eef0c1c55793cd58d71bdcf7b62ee36`.
Copia sin instalar ni ejecutar; publicación exclusivamente de fuentes y pruebas.
