# Genesis: cámara del propietario y productores

27/09/2026. Continúa PR33. Fuente, análisis estático y pruebas sintéticas.
El juego no se ha ejecutado ni se ha instalado la copia de trabajo.

La selección dentro del scanner llamaba siempre al CameraManage primario.
El enganche `02B25D8B` ahora consulta el propietario del widget y devuelve
el CameraManage secundario solo para el clon vivo de J2. Comprueba sesión,
actor/lifetime/serial, hilo, singleton `05799D3C`, slots `+CE0/+CE4` distintos,
vtable `04CF2B1C` y target `camera+74 == Sub0`. Revalida identidad y campos
antes de devolver el puntero. No escribe en las cámaras. El widget stock
conserva el getter nativo; un clon revocado devuelve cero y el caller lo omite.
El gateway mantiene el contrato thiscall sin argumentos de pila (`ret`).

## Productor global: no basta con reenviar sus resultados

La función es `0281FB30`, con prólogo de pila alineada, no `0281F5F0`.
Construye el rayo desde el contexto primario obtenido en `0281FB8A`.
`EBP-1B8` es el candidato; `EBP-17C` es un **entero de estado 0/1/2**,
no un actor. Lo entrega en `028207B9` a `01C70436 -> 02B2A210`, que
selecciona animaciones. El envío del candidato ocurre en `02820716`.
Estos dos envíos y el contexto del productor siguen pendientes de conexión.

El detector además escribe sobre el objetivo común:

- Posición `+10` mediante `01C5A6B8 -> 0281DDE0`.
- Enfoque `+24` mediante `01BC335D -> 0281F510`; getter `01C6341B`.
- Estado persistente `+20` mediante `01C91177 -> 0281B400`; getter `01B9F3A9`.

Por eso repetir el detector con otra cámara exige aislar el estado derivado
de la vista. El estado persistente y la recompensa necesitan una política
explícita de mundo compartido; no se deben duplicar todas las escrituras.

## Retirada de objetivos

El constructor `0281ADA0` instala vtable `04DA8570`, inicializa `+20/+24`
y acaba en `0281AE3C`. El destructor `02823A30` notifica al scanner stock
con `01BB4079 -> 02B29F60` (`ret 4`). Esa notificación es condicional y puede
omitirse si no hay scanner stock. La llamada de limpieza base `02823AA8`
es incondicional: es la frontera candidata para revocar referencias de J2
antes de liberar el objetivo. **No se ha conectado todavía**: requiere
validar contenedores dinámicos y evitar llamadas durante otro callback.

El cierre `02B211C0` puede conceder recompensa; no sirve como sustituto
indiscriminado de la retirada. El clon mantiene la inicialización inactiva
hasta conectar activación, bits de vista, productores, retirada y efectos.

## Validación

- Cámara: 117 comprobaciones, memoria inalterada, fallos de lectura, cambio
  de target/sesión/cámara y aliases rechazados.
- Integración: 26 escenarios / 4103 assertions; hilo ajeno y actor muerto
  no obtienen la cámara del clon.
- Gateway x86 sintético: stock/local/hidden, ECX, pila y registros preservados.
- 22 testigos SHA-pinned; enlace freestanding; cuatro pruebas del instalador.
- Copia separada con 91 hooks y reversión exacta, nunca ejecutada.

SHA256 módulo: `fa5841ee2149a90f291fda5fbb6a366adac2a78e6321c4cd74c2b185376dceb7`.
SHA256 copia: `4839ee5d5cfc69feea36866393076534dc7c4d675ac463c07e59ebb68a36c865`.
