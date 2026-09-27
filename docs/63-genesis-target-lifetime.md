# Genesis: identidad observada de objetivos

27/09/2026. Continúa PR34. No se ha ejecutado el juego.

Dos enganches del original January alimentan un registro de generaciones:

- `0281AE26`: final de constructor base del objetivo `04DA8570`, después de
  inicializar sus campos. El gateway conserva flags/registros y reproduce
  `mov eax,[ebp-8]; pop edi; pop esi`; vuelve a `0281AE2B`.
- `02823A53`: comienzo del destructor, antes de limpiar el owner o ejecutar
  callbacks. Revoca el registro, reproduce la escritura de vtable y vuelve
  a `02823A5C`. No lee la memoria del objetivo para revocarlo.

El Runtime arranca y vincula el observador antes de alcanzar los enganches.
Una consulta solo devuelve identidad si se ha observado el constructor y no
su destructor. Reutilizar una dirección recibe otra generación. No se inventan
nacimientos leyendo punteros de listas. Las clases derivadas pueden cambiar
su vtable después del constructor base: consultar identidad no vuelve a exigir
la vtable base ni lee el objeto.

La tabla admite 512 objetivos vivos. Si se llena, los objetivos adicionales
quedan sin acreditar; no se sustituyen entradas vivas. La muerte libera el slot.
Una doble alta sin baja, reentrada o evento desde un hilo distinto invalida
la admisión. Los eventos de otro hilo activan un bloqueo atómico para no dejar
una falsa identidad viva si se pierde un destructor. El contador no hace wrap.
El comportamiento del motor original continúa; el registro no modifica objetos.

**Alcance:** esto conecta observación de vida, no completa la retirada de las
referencias internas del scanner. Antes de activar el clon hay que conectar esa
retirada síncrona, los estados derivados de cada cámara, los productores, los
bits de vista y los efectos. El clon sigue inactivo. El estado persistente de
escaneado y las recompensas compartidas no se duplican en este bloque.

Validación: 568 comprobaciones de lifetime; Runtime 27 escenarios / 4110
assertions; ABI x86 sintético de constructor/destructor, incluyendo flags,
registros, pila y bytes reproducidos; 23 testigos del análisis de productores;
enlace freestanding y cuatro pruebas del instalador. Copia separada con 93
hooks y reversión exacta, sin instalar ni ejecutar.

SHA256 módulo: `428dc4870c14d221f96292627365235ecaf9badb27d5747673ec9b54836a4efd`.
SHA256 copia: `3e452a181dc9ec9811583b9a3667dc0703394e63f40a5cc1640b8e8fac7c742e`.
