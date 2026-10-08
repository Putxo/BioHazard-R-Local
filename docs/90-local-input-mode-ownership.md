# Mantener el control local de J2 ante nuevas peticiones de CPU

08/10/2026. Continúa PR61. El propietario informa de J2 controlado por IA;
la copia exacta que probó sigue sin identificarse. Este bloque protege una
transición adicional; no demuestra que sea la causa de esa prueba concreta.

## Contrato nativo

La conversión Cpu3 a Pad1 del binder ocurre al resolver la asociación PCS.
No es una conversión repetida cada frame. El motor también puede pedir cambios
mediante el setter virtual de ThinkMode, slot `+110`: hay solicitudes de 3 en
`01F37EC3` y `01EAA6EF`, con dispatch en `01F37ED7` y `01EAA6F9`.

El wrapper `01BB8B60 -> 0278CC40` llama al setter base y después propaga el modo
a un segundo gestor. El setter base `01BE3257 -> 027F1290` limpia el controlador
CPU al abandonar modo 3, actualiza el gestor principal y escribe `actor+E40`.
Escribir solo ese campo dejaría otros consumidores desincronizados.

## Política conectada

Un gateway en `0278CC40` ajusta el argumento y continúa el wrapper nativo completo.
Solo cambia una solicitud Cpu3 a Pad1 cuando:

- el actor es el Sub0 que ya se enlazó en modo Pad mediante eventos observados;
- Main y Sub conservan actores vivos distintos, generaciones, seriales, tipos,
  PCS y punteros `+44` correspondientes;
- el tracker y el indicador local siguen activos;
- ambos actores están en Pad1 o temporalmente en Invalid0;
- la lectura ocurre en el hilo propietario, sin binder abierto ni captura anidada.

La identidad y el modo se vuelven a leer. Una muerte/reasignación durante la
consulta invalida la revisión y conserva el argumento original. Nunca convierte
un actor que ya esté en Cpu3/Network2 mediante esta política: la conversión
inicial sigue perteneciendo al binder. No depende de los gestores del HUD ni de
un frame de dibujo para conservar el control.

Una petición Invalid0 se conserva, por lo que no se impide la suspensión de
control. Tras esa suspensión, una solicitud Cpu3 del mismo actor vuelve a Pad1.
Una petición Network2 se conserva y revoca la propiedad local observada de Sub0;
no puede recuperarla sin otro enlace PCS observado. J1, otras unidades, objetos
no observados, punteros reutilizados, seriales cambiados y llamadas desde otro
hilo conservan su modo solicitado.

La función no escribe memoria del actor ni llama por sí misma al setter. El
gateway conserva registros y estado FP/SIMD, reemplaza el único argumento,
reproduce el prólogo de nueve bytes y salta a `0278CC49`; el motor conserva su
limpieza de pila `ret 4` y sus actualizaciones internas.

## Verificación

- `audit_local_think_mode.py ORIGINAL.exe`: 18 testigos fijados al SHA January.
- LifetimeSource: 77 escenarios / 1.566 comprobaciones con memoria simulada,
  incluidos suspensión/reanudación, red, ausencia/retirada del HUD, muerte,
  reutilización de dirección, reenlace, cambio de serial y reentrada en lecturas.
- ABI i386 del gateway: cinco solicitudes, argumentos, alineación, registros,
  dirección de continuación, tamaño del frame, FP/SIMD y limpieza de pila.
  Continuación nativa sustituida por un stub; no se ejecuta código del juego.
- Runtime acumulativo: 217 escenarios / 49.858 comprobaciones con host simulado.
  Compilado localmente con clang en modo MSVC; el MSVC puro no admite el
  `__builtin_memcpy` preexistente de Hunter, sin modificar ese componente.
- Cinco pruebas del instalador/ELF; candidato separado con 185 hooks y reversión
  exacta. La comprobación Cpu3 de PR61 sigue pasando.

SHA del módulo: `8cf96b582c419b7e235b21d23b6876f84206a4c51d9343c8bbecce297f3306e5`.
SHA del candidato local: `4ef8ca438494e9c76ed379c223ca2a32d536dec0cb453f1a84b3fd6f991c30af`.

## Límites

Esta protección se limita al wrapper auditado y a una asociación local ya
observada. No prueba todos los cambios de escena/checkpoint ni completa cámaras,
guiones o escenas sin partner. No recupera un actor que nunca pasó por el binder.
Siguen pendientes la identificación del EXE probado y la validación jugando por
el propietario. El juego no se ha abierto; no se ha instalado ni publicado el EXE.
