# Genesis: estado de detección por propietario

27/09/2026. Continúa PR35. El juego sigue sin ejecutarse.

`TargetViews` guarda la posición calculada y el enfoque de J2 sin escribir en
el objetivo nativo. Cada muestra exige un target cuya generación se observó
en el constructor, la sesión completa (epoch y ambos actores/lifetimes/serials),
un scanner registrado de J2 y un frame válido. El Runtime aporta estos datos
mediante LifetimeSource, Registry y PipelineClock en su hilo propietario.

Las 512 entradas usan el slot acreditado por TargetLifetime. Reutilizar el slot
con otro target/generación no hereda una muestra anterior. Cambiar la sesión
vacía las muestras. Se aceptan el frame actual y el inmediatamente anterior
para tolerar el orden normal de actualización; un productor que deja de publicar
no mantiene indefinidamente una posición antigua. Se rechazan reentrada,
retroceso de frame, floats no finitos, enfoque fuera de 0..3 y cambios durante
la consulta. Stock conserva su camino; Hidden entrega una muestra vacía.

El estado persistente nativo `target+20` y las recompensas no se copian aquí.
Este componente separa únicamente los datos derivados de una vista. Está
compuesto dentro de Runtime, pero **los gateways nativos de lectura/escritura
y el recorrido del detector todavía no lo alimentan**. El clon sigue inactivo.

## Precisión adicional del análisis

`0281FB30` utiliza cámara global, pero su `this` es el objeto que contiene la
lista de targets, no un singleton universal. Se llama desde `023CEA44`,
`023D4449` y cuatro ramas de `027EBC10` (`027EBD87`, `027EBE4C`, `027EBEAA`,
`027EBED8`). Repetir el recorrido exige conservar ese `this` concreto.
Dentro de la función también hay un finder Self en `0282006F`; separar solo
el getter de cámara dejaría otra dependencia de J1.

El getter de posición `01C4C0F9 -> 02823220` recibe un destino de copia y
termina con `ret 4`; no devuelve simplemente un puntero a `target+10`.
El constructor/asignación Vector3 copia tres floats y fija el cuarto DWORD
con `04CB6D6C`, cuyo contenido en este original es **cero**. Los futuros
gateways deben reproducir ese ABI y esa cuarta componente.

Validación: 2210 comprobaciones del almacén; Runtime 28 escenarios / 4240
assertions; enlace freestanding y cuatro pruebas del instalador. El módulo
sigue instalando 93 hooks. Copia separada con reversión exacta, no instalada
ni ejecutada. No se sube material propietario.

SHA256 módulo: `71d7bcd286b0ad7a71fd00477635059af1dff1307663e8a39db7f8de74cad59d`.
SHA256 copia: `6061053dc0c61d3ad899f5b9a8378fec2db83a7a850afb511165ca8ccebec3b5`.
