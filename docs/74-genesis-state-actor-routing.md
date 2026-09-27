# Genesis: propietario del inventario en ocho estados internos

27/09/2026. Continúa PR45. Juego nunca ejecutado.

La auditoría de los llamadores del escaneo encontró ocho consultas adicionales
a Self en los estados internos 02B2F2C0..02B2FFE0. Los doce sitios conectados
anteriormente no cubrían esas funciones. Cada una conservaba el widget en
EBP-8, resolvía Self, obtenía su pack y guardaba el recuento de hierbas del pack
en Scanner+34C. Un clon de J2 podía así conservar datos de inventario del J1.

| Estado nativo | Función | Llamada a Self ahora dirigida al propietario |
|---|---|---|
| 0 | 02B2F2C0 | 02B2F395 |
| 1 | 02B2F4A0 | 02B2F575 |
| 2 | 02B2F680 | 02B2F755 |
| 3 | 02B2F860 | 02B2F935 |
| 7 | 02B2FA40 | 02B2FB15 |
| 4 | 02B2FC20 | 02B2FCF5 |
| 5 | 02B2FE00 | 02B2FED5 |
| 6 | 02B2FFE0 | 02B300B5 |

Los sitios reutilizan rev_hud_scanner_self: el original sigue llamando a su
finder con ECX y predicado intactos; el clon obtiene su actor admitido. La ruta
revocada devuelve cero. Se ha verificado una comprobación nula en los ocho
consumidores antes de consultar el pack. El getter 01C2A51C lee pack+C8 y cada
función escribe el resultado en su propio Scanner+34C. No se modifica Self,
la selección global de personaje ni el inventario para conseguir esta lectura.

El runtime conecta ahora20 consultas de actor del widget:14 al finder y6 al
selector por personaje. Este número no afirma cobertura completa de todos los
subsistemas: otras consultas en widgets vecinos se revisarán por separado.
Genesis permanece inactivo hasta completar activación/cierre, efectos y las
fronteras restantes; los límites documentados de reentrada siguen vigentes.

## Validación

57 testigos estáticos comprueban el frame, número de estado, llamada exacta,
comprobación nula, getters del actor/pack y escritura local de los ocho sitios.
Los nuevos hooks usan el gateway ya probado por la ABI sintética de actor;
no cambia el módulo C++/ensamblador respecto a PR45. Las cuatro pruebas del
instalador pasan con123 sitios sin solapamientos y símbolos ejecutables.
La copia separada verifica cada instrucción de entrada y su reversión exacta.

SHA256 módulo reutilizado: `a86905218df2eb3cdf0c419042962890faaee0375b8473f2638b2526ca1b70f9`.
SHA256 copia: `afbe79c7179d396db7853e4a43d97d2d764fb1517f7736c47dde70dfccde8858`.
No se instala ni se ejecuta la copia, ni se publica código propietario.
