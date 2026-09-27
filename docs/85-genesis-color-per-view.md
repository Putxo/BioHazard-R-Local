# Corrección de color por vista al usar Génesis

27/09/2026. Continúa PR55. Juego no ejecutado.

El filtro embebido `sGameFilters+6A0` es una sola unidad compartida. Al activar
el Scanner stock se borra su bit `0800`, y `sUnit` rechaza el dibujo **antes**
de consultar la máscara de vista. Filtrar solo la máscara no permitía conservar
la corrección de color de J2 mientras J1 usaba Génesis.

La política nueva identifica exclusivamente esa unidad mediante el singleton,
el offset y las dos vtables. Dentro de la sesión local valida ambos Scanners,
sus auxiliares, vidas, propietario y frame; lee y vuelve a comprobar el estado.
La actividad de FilterSet determina si cada vista está usando Génesis.

* La máscara efectiva conserva los bits originales y elimina View0/View1 según
  el estado de cada Scanner. Nunca activa un bit de vista ausente.
* Los dos predicados de dibujo de `sUnit` (`032686DA`, `032688EE`) conservan un
  resultado nativo verdadero. Solo pueden promover un falso para la unidad
  compartida cuando J1 usa Génesis, J2 no, el control global nativo indica su
  desactivación por Scanner y quedan vivos los flags requeridos.
* Esa excepción exige state2, disponibilidad de actualización `0400`, permiso
  de dibujo `4000`, ausencia de `0800` y View1 original. No rehabilita una unidad
  muerta, invisible, fuera de zona o sin View1.

El getter de actualización `01DEE3F0` comprueba `0400`, mientras el de dibujo
`01DEE460` exige `0800 | 4000`. Por ello la excepción de dibujo no necesita
reescribir la actualización ni el booleano global `+62B0`. El sistema sigue
calculando sus parámetros normales. No se cambia ningún flag, recurso ni
material compartido desde esta política.

Las llamadas ajenas a esta unidad, otros hilos y certificados no admitidos
conservan la decisión stock. La ruta inmediata local ya conectada evita aplicar
esta política de propietario en un worker. El filtro sigue dibujándose con el
contexto nativo de cada vista; no se declara aquí probada la composición final
de píxeles ni el aislamiento de materiales de los Hunter.

Validación: las cuatro combinaciones de uso de Génesis, máscaras 0/1/2/3/7,
flags ausentes, muerte, lectura fallida, frame cerrado y cambio de owner durante
la lectura. Total Runtime: **217 escenarios / 49.858 comprobaciones**. ABI
sintética de tres rutas conserva thiscall y lee solo AL del resultado stock;
24 testigos estáticos, enlace freestanding y cuatro pruebas de instalador.
La copia acumulativa revierte exactamente sus **181 hooks** al input conocido.

Módulo: `0d1c8235fa15000b688458a2a6a68538610d598f95e82f10687663489019d3fc`.
Copia: `ea6347c1125ec64711adb4f01486796ef845c584b9122da17eb2af3794ff24d7`.
Activación desde mando todavía pendiente de los otros consumidores. No se ha
abierto ni instalado el juego ni publicado ningún binario propietario.
