# Encendido privado de los efectos del Génesis de J2

27/09/2026. Continúa PR54; juego no ejecutado.

El control nativo `02B23120` cambia tres flags de actividad en cada uno de los
auxiliares del Scanner y, además, escribe en `sGameFilters` y `sHunterManager`.
El nuevo gateway conserva la función original para stock y sustituye la ruta
local por un cambio limitado a los tres auxiliares privados admitidos.

Los setters nativos `01C5EFA1`, `01B8D636` y `01B7B5E4` modifican respectivamente
`0400`, `0800` y `4000`; los nueve setters equivalen a aplicar la máscara `4C00`
sobre los tres roots. El resto de bits (estado, grupo, keep-alive, vista) se
conserva. La ruta J2 no invoca los dos setters de gestores compartidos.

Antes del cambio se comprueban frame, propietario, grafo disjunto, máscara View1
y vidas observadas. Para encender se exige además el arma/emisor de sonido admitidos.
Para apagar no se exige mantener equipada el arma anterior: solo se desactivan
los efectos privados, sin dereferenciarla. Después se comprueba el grafo, los
valores escritos y el mismo propietario/frame. Un fallo revoca el conjunto;
una escritura parcial queda en cuarentena, sin liberar recursos en esa llamada.
Nunca se cae al código stock tras rechazar un Scanner local.

El gateway acepta el booleano nativo en el byte bajo, conserva el contrato
thiscall `ret 4` y reproduce el prólogo stock en su trampolín. Se añade un
enganche: **179** acumulativos.

Esto conecta el control privado de actividad; **no habilita aún la entrada de
Génesis desde el mando 2**. Falta reproducir por vista el efecto de los dos
consumidores compartidos, además de los límites de callbacks ya documentados.
Se ha identificado `sGameFilters+6A0` como `uGameColorCorrectFilter` (VT
`04DB848C`), y su dibujo heredado `037D13F0` recibe un contexto `cDraw` y pasa
sus parámetros `+440` al renderer. El código de Hunter altera además vectores
persistentes en la propia instancia; requiere otro tratamiento por vista.

Validación: 201 escenarios Runtime / 47.805 comprobaciones, 535 comprobaciones
de grafos/retirada/encendido, ABI sintética local y stock, 27 testigos estáticos,
enlace freestanding, cuatro pruebas ELF/instalador y reversión exacta de la
copia de 179 enganches. No se ha ejecutado ni instalado el juego.

Módulo: `ad0425b46a299dfaf75367090d46c580d10a98a77a05b91cf5b4981cc61b1c62`.
Copia: `0a115f83eb27b6ca1158b26dc0bea1b3e5321a27c53fd1142de7a48d06e47f2f`.
