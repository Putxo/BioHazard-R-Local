# Prioridad y terminación del bucle de comandos por miembro

27/09/2026. Continúa PR27. Fuente, integración y pruebas sintéticas para January.
**No se ha abierto el juego; no equivale a cooperativo terminado.**

La lista ordenada de `sActionCommand::update` usaba una prioridad local única en
`EBP-0x100` y un flag de parada en el bit 0 de `EBP-0x10C`. Seleccionar una acción
exclusiva o encontrar una prioridad menor terminaba el recorrido completo. Ahora
estos dos estados pertenecen al miembro del comando: saltar una entrada de J1
permite seguir buscando las de J2, y viceversa.

El bit 1 se conserva común. Controla la reconstrucción del historial nativo en
`manager+0x84`; separarlo ingenuamente haría que la primera selección de J2
vaciase lo que J1 acaba de añadir. El contenedor es un anillo de nueve slots
con ocho entradas útiles. No se amplía, copia, ni sustituye su almacenamiento.
Su consulta en `0x01EA7BC0` busca una clave de comando. La política de capacidad
y la idoneidad completa de un historial común siguen pendientes de auditoría.

## Enganches

| Dirección | Efecto |
|---|---|
| `01EA61A7` | Inicia la selección tras inicializar prioridad y flags nativos |
| `01EA6215` | Conserva el getter nativo del comando; carga prioridad/rank/flags de su miembro |
| `01EA6235` | Conserva la comparación signed; una prioridad menor termina solo ese miembro |
| `01EA6679` | Guarda prioridad y parada del miembro; deja continuar el iterador común |
| `01EA66B6` | Publica ambos resultados y conserva el campo nativo `+170` para J1 |
| `01EB7419` | El draw de ActionIcon2D recibe la prioridad publicada de su miembro |

Se reutiliza el mapper nativo `0x01E87DB0`; no se compara directamente el valor
enum de prioridad ni se cambia el mapper global. La prioridad para el draw se
presta mediante una consulta acotada durante la llamada, sin escribir `+170`.
Los otros consumidores del getter global conservan su comportamiento stock.

El adaptador exige el hilo propietario, dos actores Pad vivos, frame/epoch y
manager estables, vtable de comando y delegado legible. Las tres escrituras de
locales guardan su valor previo y revierten las que ya se completaron si falla
otra. Un fallo aborta la selección local y no publica prioridades parciales.
Los resultados no se consumen durante una selección abierta ni en otro frame.
Una selección local incompleta omite el draw del icono en lugar de atribuirle
la prioridad de otro jugador. Fuera de modo local se conservan las ramas stock.

## Evidencia y validación

- `audit_action_priority.py ORIGINAL.exe`: 32 testigos fijados por SHA, incluidos
  los flags, cinco escrituras de prioridad, historial y seis puntos de enlace.
- Modelo: 11394 comprobaciones, con 200 mezclas de 40 comandos contrastadas
  contra la evaluación separada de las secuencias de cada miembro.
- Adaptador de pila: 57 comprobaciones; fallos en cada escritura, restauración,
  sesión cambiante, manager reemplazado, llamadas fuera del hilo y modo stock.
- Router: 53 escenarios / 220 comprobaciones, incluidas prioridades J1/J2,
  falta de resultado y fin del préstamo tras volver del draw.
- Grafo Runtime: 16 escenarios / 1234 comprobaciones.
- ABI x86: seis bridges con callbacks/continuaciones sintéticos; argumentos,
  pila, retorno del getter, comparación signed con overflow y destinos de salto.
- Módulo freestanding y cuatro tests del instalador pasan. Copia acumulativa
  independiente con 45 enganches y reversión exacta al input; no instalada.

Módulo LLVM 22.1.8:
`7cf05e735b67c721ea4c75e4fc97010cf637320dcb52a3b346fbb73567632c29`.
Copia acumulativa local, no distribuida ni ejecutada:
`42606328b055a78b324ac955390261a290db529647cae4847228d158287f51a3`.

## Lo que no resuelve este bloque

Los gates anteriores al bucle, flags del manager como `+176`, productores 3D,
ejecución del dibujo en workers, posicionamiento y todos los eventos simultáneos
requieren continuar la auditoría. El nuevo arbitraje pertenece a este bucle y
al consumidor 2D; no se atribuye a todos los usos de sActionCommand. Quedan además
HUD/HP/Genesis, acciones, scripts/QTE, muerte/checkpoints y cámaras. La validación
jugando sigue reservada al usuario y no se da ninguna función por comprobada
en el juego a partir de estas pruebas sintéticas.
