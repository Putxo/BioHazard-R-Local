# Indicador de daño de J2 alimentado desde su actor

27/09/2026. Continúa PR29. Fuente y verificación estática/sintética para January.
El juego no se ha ejecutado y el cooperativo completo sigue en desarrollo.

El indicador `uGUI_Damage` no consulta Self en su actualización. El gestor
Cockpit lo alimenta antes de recorrer los widgets: obtiene Self en `02B49584`,
consulta su estado y convierte `(1 - health_ratio) * 100` a entero truncado.
Clonar únicamente sus fases dejaría el nuevo indicador sin esa alimentación.

Se añade Damage como quinto tipo, con constructor, recurso GUI, nodos mutables,
registro y retirada propios. El original se obtiene de Cockpit+3C; el objeto
ocupa 2B0 bytes y su vtable es 04DE357C. La fase 8 del clon obtiene el actor de
su registro vigente y solo admite miembro/vista 1. El invocador nativo usa los
mismos getters de estado y salud y el setter del indicador. No consulta ni
sustituye Self. La admisión y el registro se vuelven a comprobar al terminar.

La conversión conserva el resultado x87 del getter y restaura la palabra de
control después de truncar. Rechaza valores negativos, NaN, infinitos y mayores
que uno antes de modificar el widget. En estado 4 oculta el indicador ordinario,
como hace el gestor original. Esto todavía **no añade DamageChoke** ni su
medidor: esa clase stock solo se construye bajo una condición del motor.

La alimentación ocurre antes del filtro de actividad para poder reactivar un
indicador previamente oculto. Fases 9 y 11 mantienen sus llamadas nativas y
recursos propios. La máscara temporal P1 también incluye el widget de daño:
no se dibuja el daño de J1 en View1 y se restaura su máscara al salir. Permanecen
admitidos los perfiles de tres y cuatro widgets para pruebas/regresión; el
runtime de producción habilita los cinco.

## Verificación

- 40 testigos fijados al SHA del original en `audit_damage_hud.py`.
- Grafo Runtime: 20 escenarios / 2103 comprobaciones, incluida alimentación con
  actor J2, originales intactos, llamada única por frame, actor destruido, hilo
  incorrecto y rechazo del proveedor de salud.
- Máscaras: 14 escenarios / 50 comprobaciones, con Damage y restauración exacta.
- ABI sintética: valores 0/25/50/100 y truncación de 0.1f, estado 4, cuatro
  entradas inválidas, ECX/argumentos, pila, registros preservados, balance x87
  y restauración de su palabra de control. No carga funciones del juego.
- Enlace freestanding y cuatro pruebas del instalador. Copia acumulativa local
  separada, 46 enganches, reversión exacta; no instalada ni distribuida.

Módulo: `890c890b6481a258452982b998fad226258e01d61521a9bb06974c90de8b7257`.
Copia: `9f5155048222a7cac3f6881ada2f127c622f4f85ced94aa063627e6e8311e27a`.

Este bloque cubre el indicador ordinario de daño. Curación, asfixia, Genesis,
otros elementos del HUD, acciones/scripts, muerte/checkpoints y cámaras siguen
pendientes. No se atribuye una barra numérica de HP inexistente al código ni se
da por comprobada la presentación sin la prueba posterior del propietario.
