# Genesis: vida observada de efectos auxiliares

27/09/2026. Continúa PR47. Análisis estático y pruebas sintéticas; no se ha
ejecutado ni instalado el juego. Genesis permanece inactivo.

El Scanner conserva tres efectos que el motor programa en el grupo 6 fuera
de las fases del HUD. La copia privada de rFilterSet evita compartir sus hijos,
pero no demuestra que los efectos continúen vivos. Comprobar su dirección o VT
no distingue un objeto destruido de otro creado posteriormente en la misma memoria.

| Tipo | Campo Scanner | Tamaño hexadecimal | VT | Construcción | Destrucción |
|---|---|---|---|---|---|
| uFilterSet | +2F0 | 48 | 04DB80CC | 0293344E | 02933576 |
| uGameTVNoiseFilter | +33C | D0 | 04DB833C | 0293559E | 02935816 |
| uOutlineFilter | +338 | 150 | 04F9A7F4 | 037DD309 | 037DD4A2 |

Los seis enganches sustituyen una escritura de VT de seis bytes. Conservan
registros, flags, pila y la escritura original. La construcción se observa
después de la escritura; la destrucción se revoca antes. Se enganchan los
cuerpos destructores, también alcanzados por los destructores escalares,
antes de liberar recursos o invocar callbacks externos.

EffectLifetime asigna una generación monótona y un tipo a cada nacimiento
observado. No descubre objetos leyendo punteros arbitrarios y no lee memoria
al retirar una identidad. Una muerte de tipo incorrecto, nacimiento duplicado,
reentrada o evento desde otro hilo invalida el observador. La invalidación
entre hilos es atómica; la tabla se consulta solamente desde el hilo propietario.
El límite de 256 entradas es una política del adaptador: al llenarse no se
expulsa un objeto vivo para admitir otro. No se rearman generaciones antiguas.

El runtime arranca y conecta el observador; el instalador contiene los seis
enganches. Esto proporciona identidades a la siguiente integración, pero **no
certifica inicialización completa, pertenencia al Scanner, asignaciones internas,
recorte por vista ni una ventana de uso sin destrucción concurrente**. El VT del
constructor de Outline se escribe antes de inicializar sus campos; no se debe
usar una identidad recién observada como permiso para dibujar.

Otros hallazgos pendientes de conectar:

- 02B29AF0 desconecta el callback y marca para retirada únicamente +2F0.
  El destructor de Scanner no demuestra retirada de +338/+33C.
- El bit de keep-alive comprobado por cUnit es el bit 3 del campo de actividad
  (+C >> 10); desactivarlo y marcar muerto no libera inmediatamente la unidad.
- 02B23120 cambia la actividad de los tres efectos y además escribe un estado
  en un singleton +62B0 y otro en un singleton +65. Estas operaciones globales
  requieren aislamiento antes de conectar activación/cierre de J2.

Validación:

- 895 comprobaciones del observador: tres tipos, muerte sin lectura, ABA,
  tabla llena, tipos erróneos, reentrada y eventos desde otro hilo.
- ABI x86 sintética para los seis enganches: orden del evento respecto de la
  escritura, pila, registros y flags; sin cargar código del juego.
- 127 escenarios y 26.377 comprobaciones del runtime compuesto.
- 40 testigos estáticos sobre el SHA exacto de enero; cuatro pruebas del
  instalador; compilación freestanding y reversión exacta de los 131 hooks.

SHA256 módulo: `09b05cc81a66da3ed9c48524996b25de2d9859b96d5921efb0423e948f64ffe3`.
SHA256 copia local: `6b56573b405a724d9a5540c95e9b008e6b104b93720a9274a23aafaad230fc84`.

La copia es material de trabajo local, no una entrega final ni evidencia jugando.
