# Curación de J2: notificaciones por actor

27/09/2026. Continúa PR30. Análisis estático y pruebas sintéticas de January.
El juego no se ha ejecutado. El cooperativo completo sigue en desarrollo.

Heal es el sexto widget independiente: vtable 04DE39FC, tamaño 2B0,
original en Cockpit+44. Conserva su constructor, inicialización de recursos,
animación y fases nativas. Su máscara original solo aparece en View0 durante
el dibujo local. El clon registrado para Sub0 aparece en View1.

Seis llamadas a la activación stock 01C88B6C comparten un gateway que recibe
el actor del llamador [EBP-8]: 0279900E, 0279907A, 027999CF, 027ACE72,
027B211C y 027B21E9. Fuera del modo local continúa la llamada original.
En una sesión local válida Self conserva su efecto; Sub0 envía una notificación
a su clon. Otros actores o snapshots no certificados no activan el efecto.
No se modifica salud, consumo de hierbas ni inventario: siguen siendo nativos.

La notificación guarda epoch y la identidad completa de ambos actores, incluidos
sus lifetimes y seriales. Se consume una sola vez en la fase 8 admitida del clon,
antes de filtrar actividad. Si el productor ocurre después del recorrido HUD,
se admite el frame siguiente. Cambiar de actor/sesión o pasar más de un frame
descarta el aviso. Varias solicitudes del mismo frame se agrupan. Este límite
afecta al aviso visual, no al efecto de curación del gameplay.

El consumidor vuelve a comprobar permiso, registro y separación del árbol al
volver de la función nativa. Mantiene los perfiles anteriores de 3/4/5 tipos;
producción usa seis. No se cambia Self, serial, GameMode ni Network.

## Verificación

- 20 testigos del original fijados a su SHA en audit_heal_hud.py; el auditor de
  tipos HUD incluye Heal, con 115 comprobaciones totales.
- Runtime: 23 escenarios / 2982 comprobaciones, con curación J1/J2, cola para
  el frame siguiente, retiro y cambio de lifetime, preservación de originales.
- Cola: 105 comprobaciones sobre duplicados, expiración, identidad y sesión.
- Máscaras: 15 escenarios / 54 comprobaciones, incluida restauración de Heal.
- Gateway ensamblado y ABI sintética de decisión, argumentos, pila y registros.
- Enlace freestanding; cuatro pruebas del instalador; copia local separada con
  52 enganches y reversión exacta. No instalada, distribuida ni ejecutada.

Módulo: 89fd4af2d91d9605a4e5958d292dfac03dc33ab57a38891a215f6275700cf916.
Copia: d4663e65cb32091608388fc05c30a119e00dded47a2d2edb3e17d181e1d8245d.

Quedan Genesis, asfixia y otros elementos HUD, acciones, scripts/QTE,
muerte/checkpoints, cámaras y escenas sin partner. La prueba visual y jugando
corresponde al propietario; no se presenta este bloque como cooperativo completo.
