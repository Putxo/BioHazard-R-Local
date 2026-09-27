# Genesis: copia privada del recurso de filtros

27/09/2026. Continúa PR46. No se ha ejecutado ni instalado el juego.

## Problema y cambio

La inicialización del Scanner crea tres unidades auxiliares, registradas en
el grupo6 de sUnit: uFilterSet en +2F0 (48 bytes, VT04DB80CC), uOutlineFilter
en +338 (150 bytes, VT04F9A7F4) y uGameTVNoiseFilter en +33C (D0 bytes,
VT04DB833C). El grafo GUI certificado anteriormente no incluía estas unidades.
La independencia de paneles y animaciones no implicaba independencia de efectos.

El uFilterSet conserva en +40 un rFilterSet de 80 bytes (VT04DB7A34). Su array
en +68 contiene unidades modificables, no simples parámetros de solo lectura.
El setter 01B94BF7/02933C20 conserva el puntero recibido, añade una referencia
y llama a initialize virtual+14 de cada unidad contenida. No hace una copia.
El cargador sResource virtual+30/0325F440 busca primero en caché y devuelve el
recurso existente incrementando sus referencias; la opción1 no evita compartirlo.
Por tanto, inicializar el clon podía reinicializar filtros ya utilizados por J1.

Dos gateways en 02B295C6 y 02B295FE conservan la llamada nativa del cargador
(thiscall, tres argumentos, ret12). Tras ella, solo durante la inicialización
admitida del Scanner local, el runtime sustituye el recurso por una copia privada.
La referencia obtenida de la caché se libera exactamente una vez. J1 conserva
el recurso que devolvió el cargador y no pasa por la copia.

## Copia y propiedad

Se usa la fábrica nativa 02929680 y su constructor 01C5E8BC/02929AC0. El nuevo
recurso tiene una referencia, clave de caché cero y tamaño contable cero. No se
inserta en la caché global. El método save 01C4B4F1/0292A050 serializa el array
de filtros; load 01C12E2B/02929F50 lo carga sobre el recurso preconstruido.

MtMemoryStream 02FD8B90 usa un buffer privado fijo de256 KiB con flags3:
lectura/escritura, sin propiedad ni crecimiento del buffer. El adaptador comprueba
error de escritura y longitud; rebobina y limita la lectura a los bytes escritos.
El destructor 02FD8C20 no libera el buffer prestado. La copia ocurre en el punto
estructural, antes de que el setter inicialice los nuevos filtros.

La captura doble verifica rFilterSet, VT del array04CC7278, count+6C,
capacity+70, flag propietario+74, table+78, direcciones y VT de cada filtro.
Admite entradas nulas como los bucles nativos. El límite64 es del adaptador,
no una afirmación sobre el máximo del motor. Se rechazan alias entre recurso,
tabla y bases de unidades, y cualquier cambio de identidad durante la copia.
El grafo resultante conserva count y tipos, con direcciones independientes.

El certificado cubre identidades y bases de40 bytes de las unidades; **no es
un certificado recursivo de todas sus asignaciones internas ni una prueba de
vida contra destrucción concurrente**. La reconstrucción de propiedades depende
del serializador nativo. Su comportamiento con los assets reales queda sin
validación en ejecución por la prohibición de abrir el juego.

Un fallo bloquea la admisión del Scanner local. Recursos de identidad ambigua
se retienen sin destruirlos: el destructor del array elimina sus elementos y
no debe recibir posibles punteros prestados. Una copia independiente pero
incompleta o de tipos distintos sí se libera. Si desaparece la admisión tras
copiar, la copia no se publica; queda retenida con el clon en cuarentena.
Los reintentos y esa retirada requieren la gestión de auxiliares que sigue pendiente.

La liberación nativa decrementa las referencias; con cero elimina entradas de
caché comparando el puntero concreto, y destruye el recurso. La clave y tamaño
contable cero del recurso privado evitan atribuirle la identidad o contabilidad
del recurso cacheado. No se copian su nombre, hash ni cabecera por memcpy.

## Validación y siguiente frontera

- 137 comprobaciones sintéticas de captura, copia, alias, lecturas fallidas,
  cambios de identidad y fallos parciales.
- 126 escenarios/26.367 comprobaciones del runtime, incluidos el paso stock,
  sustitución durante init, fallo de fábrica, alias, cancelación y fallo de carga.
- ABI x86 sintética: tres argumentos originales, retorno stock/copia/cero,
  ESP y registros no volátiles. Solo se emula código de test, nunca del juego.
- 53 testigos SHA fijados al EXE de enero, compilación freestanding y4 pruebas
  del instalador. Copia acumulativa125 hooks, reversión exacta.

SHA256 módulo: `6d8d05d49f6dcdcd90a03b94558f17d6ac63a72a0f0e4f2c30945eeb977134e5`.
SHA256 copia: `c54a7744cd6694f5e5495f29248ed74c56979394e895dd6f982a201d9a675bc1`.

**Genesis sigue inactivo.** Faltan máscaras/recorte por vista, pertenencia y
retirada de los tres auxiliares, certificación de su grafo interno, la activación
y el cierre con recompensa, además de las fronteras de vida ya documentadas.
En particular, el destructor de Scanner llama a02B29AF0, que retira +2F0 pero
no demuestra la retirada de +338/+33C. No atribuir al destructor una cobertura
que no tiene ni a este cambio un cooperativo terminado.
