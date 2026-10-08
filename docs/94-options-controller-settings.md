# Ajustes de mando del dueño de la pausa

08/10/2026. Continúa PR65, `d71eae57f0e89bdcad05e29d90a5285a4517d590`.
Implementación para January 30 2013. No se ha ejecutado el juego.

Las lecturas de botones de PR65 permitían dirigir el menú de opciones al mando
que abrió Pause, pero sus lecturas y setters de configuración seguían usando el
mando principal. Ahora los cinco campos de control de esa pantalla se leen del
PadData del dueño y los setters nativos se ejecutan sobre ese mismo slot.
No se modifica `mStartPadNo` para prestar el mando principal a J2.

## Recorrido auditado

`uOptionManager` tiene vtable `04DF5AE4`. Sus llamadas de lectura `02C2C641` y
`02C2C64F` reciben respectivamente los parámetros en `manager+4C` y `+150`.
Se conserva la lectura nativa de los ajustes comunes y se sustituyen cinco words:

| Campo del parámetro | Campo de PadData (`sGamePad+668+slot*C0`) |
|---|---|
| `+4` | `+10`, o `+14` cuando el layout actual es 6 |
| `+8` | `+C` |
| `+C` | `+18` |
| `+10` | `+20` |
| `+14` | `+1C` |

La segunda copia es una referencia de comparación. Esto no demuestra todavía
todas las rutas de cancelar, previsualizar o restaurar valores predeterminados.

La llamada de aplicación `02C2CF0E -> 01C1A0D6 -> 02C19D90` conserva su contrato
thiscall, argumentos y `ret 8`. Su resultado AL indica el cambio detectado por
el código nativo, no una confirmación general de éxito. Se conserva ese resultado.

Doce cargas de `mStartPadNo` dentro de los cinco setters conservan su valor
original fuera de la aplicación síncrona de este menú. Dentro de ella seleccionan
el dueño capturado. Así se conservan las operaciones nativas del setter de layout,
incluidos su layout anterior y sus flags. Las llamadas de arranque y de cambio de
dispositivo no pasan a obedecer ciegamente sus argumentos de índice.

Antes de aplicar, el original puede copiar `104` bytes al objeto global en
`055926F4`, campo `+3C`. Se reproduce esa publicación en memoria con los cinco
valores globales de J1 conservados cuando aplica J2. El resto de ajustes comunes
sí procede de la pantalla. El predicado nativo exige que el byte bajo del primer
argumento sea exactamente 1. Se evita una segunda publicación al invocar la
aplicación nativa con ese argumento a cero.

## Propiedad y cancelación del acceso

Se comprueban dueño de Pause, actor Sub0, serial, modo Pad, hilo, manager y
singleton del mando antes de leer o aplicar. La aplicación local exige haber
leído también la referencia. Una sesión invalidada no cae en la aplicación stock
del mando principal ni recupera permiso al reaparecer el mismo puntero.
Abrir otra pausa o salir de ella invalida el acceso anterior. La reentrada en
lectura/aplicación nativas se rechaza.

Si el acceso se revoca durante la aplicación síncrona, sus setters restantes
mantienen el slot inicial: no terminan escribiendo parte de los valores de J2
en J1. La revocación bloquea la siguiente transacción. Esto no proporciona
rollback general de efectos nativos ya realizados ni soporte para carreras
arbitrarias entre hilos.

## Validación

- 40 testigos estáticos fijados por SHA del original; 15 hooks contrastados
  con el instalador. El ejecutable privado no se publica ni ejecuta.
- Opciones: 122 escenarios / 1016 comprobaciones con callbacks simulados.
  Ambos dueños y ambos valores de mando principal, fallos de lectura/escritura,
  cambio de serial/actor, reentrada, flags, copia global y revocación.
- Menú existente: 30/139; seguridad: 18/89.
- Grafo Runtime: 218 escenarios / 49871 comprobaciones; incluye transmisión
  real de `OptionsAccess` al router a través del constructor de Runtime.
- ABI: 14 gateways del mod en test x86 sintético. Los dos nuevos selectores
  conservan pila, registros, flags, x87 y XMM; callback con dirección normal y
  pila alineada. Ninguna función del juego se carga en estas pruebas.
- Enlace freestanding completo y cinco pruebas del instalador. Copia separada
  con 227 hooks y diez reemplazos inline; reversión exacta a la base aceptada.

Módulo LLVM: `1d004982fe828e4090d7a4de822a82071196a66530ed5c835b5be839611f960d`.
Copia local: `c40489b3e05208d9f203e70e62de2e6b5f71e581f3f761addd1cfb7b882bb6f5`.
No instalada en Steam ni distribuida.

## Pendiente

Los valores de J2 viven en su PadData durante la sesión; no se ha implementado
su persistencia independiente entre arranques. Teclado/ratón, vídeo, audio y
otros ajustes comunes siguen compartidos. El cambio automático de dispositivo,
las rutas completas de cancelar/defaults y la validación jugando siguen pendientes.
Este bloque no demuestra que el informe de J2 inmóvil haya quedado resuelto en
el ejecutable probado por el usuario. Tampoco completa las acciones, Genesis,
HUD, QTE, muerte/checkpoints, cutscenes o escenas sin compañero.
