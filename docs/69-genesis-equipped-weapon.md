# Genesis: pertenencia del arma retenida

27/09/2026. Continúa PR40. Juego nunca ejecutado.

El scanner guarda el arma recibida en +2FC al activarse y puede usarla en fases
posteriores. El runtime comprueba ahora esa identidad antes y después de las fases
del clon y de su pasada de detección. Un puntero retenido no nulo debe coincidir
con el arma equipada en el inventario del propietario J2. No se desreferencia el
arma para decidir si sigue perteneciendo al jugador. Si falla la admisión previa,
no entra en la fase nativa; si cambia al retornar, se detiene el clon.

La resolución reproduce la ruta original: actor+1524 -> cBioItemPack,
selección pack+D4 -> virtual +14 -> pack+4+index*4, con 15 slots. Un índice
>=15 devuelve cero, igual que el getter nativo. Se validan vtable 04D2D42C,
virtual 01BFECCD, packs distintos y sin solapamiento, y que el arma seleccionada
no aparezca en ningún slot de J1. Dos capturas y la revalidación del propietario
rechazan cambios de sesión, lifetime, pack, selección o contenido.

El valor cero en scanner+2FC conserva el caso nativo sin arma (el constructor lo
inicializa así). Esta comprobación identifica inventario/equipo en el momento de
la llamada: **no mantiene vivo el arma durante callbacks, ni valida su tipo, ni
habilita por sí sola la activación**. La destrucción reentrante sigue pendiente.

## Activación y visibilidad encontradas

0279CF80 recibe actor y dos argumentos, devuelve con ret8. Compara la cámara del
actor con la primaria antes de hacer cambios. Sus cuatro escrituras de visibilidad
limpian/restauran bit0 de la máscara de personaje y arma. El getter 0279D1A0 lee
bits16..25 de object+0C; el setter 01EB6130 preserva los restantes bits. Para J2
será necesario seleccionar View1, además de redirigir el scanner.

La rama no-Genesis no modifica el retículo: 01BFC67B -> 02B4A9A0 obtiene
Cockpit+54 y llama 01BE9C6F -> 02B42270 con arma y un flag. Ese widget de
identificación del arma al apuntar es adicional a los siete tipos ya clonados.
No se cambia el padre Cockpit global ni se trata Main/SubEquip como jugadores.

Las llamadas nativas de activación 02B20D70 y cierre 02B211C0 también manipulan
listas, animaciones, arma y recompensas. Su admisión y efectos por vista siguen
pendientes. El clon continúa inactivo en la inicialización de producción.

## Validación

- Inventario/equipo: 529 comprobaciones, incluyendo fallo de cada lectura,
  cambios entre capturas, slots nulos/fuera de rango, alias y sesión revocada.
- Runtime: 62 escenarios / 10863 comprobaciones; arma retirada, cambio de slot,
  arma de J1, alias de pack y cambio durante detector/fase.
- 92 testigos del ejecutable original fijado por SHA256.
- Enlace freestanding, cuatro pruebas del instalador, 110 hooks y reversión
  exacta en copia separada. No instalada ni ejecutada.

SHA256 módulo: `9198ebe353147362a9ada38f589a6d9654faa3fef04c564d2d9743861064cb44`.
SHA256 copia: `1873a0739bf7295066ee2ae0ede767c176a32adcf092fc25753a6a211e07b571`.
