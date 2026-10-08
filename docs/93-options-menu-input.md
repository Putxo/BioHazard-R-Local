# Entrada de opciones desde el dueño de la pausa

08/10/2026. Continúa PR64. January 30 2013. No se ha ejecutado el juego.

## Comportamiento corregido

Los cuatro sitios de PauseHD cubiertos previamente no bastaban para sus
pantallas de opciones. Se localizaron 25 llamadas adicionales a los lectores
raw de sGamePad, todas con índice cero fijo, en estas clases nativas:

- uGUI_OptionMenu, vtable 04DF4C34: pantallas y controles de opciones.
- uGUI_OptionTop, vtable 04DF4EE4: selección principal de opciones.
- uOptionManager, vtable 04DF5AE4: consulta del bit Start para cerrar.

Se instalan 14 llamadas de +1A0, nueve de +1AC y dos de +198 mediante los
bridges del dueño de Pause. Se añade el bridge +198. Se conserva el manejo
nativo posterior de máscaras, selección, confirmación y cancelación. Cuando
el dueño local es J2, las lecturas reciben Pad1; no modifican el selector global.
El dueño se conserva en los estados anidados 6/7 ya auditados, sin atribuirles
una identidad de pantalla que no se ha demostrado.

Había además una discrepancia para J1: la apertura local seleccionaba Pad0,
pero las lecturas posteriores con dueño 0 seguían mStartPadNo. Ahora un menú
abierto por J1 en una pareja local válida recibe Pad0 aunque el selector stock
sea 1. Fuera de ese caso se conserva el comportamiento stock. La identidad de
J2, los fallos de lectura y la retirada del dueño usan la política existente.

## Evidencia y pruebas

- `audit_options_input.py`: 31 testigos del original SHA-pinned, incluidos los
  argumentos cero de las 25 llamadas, sus destinos y las tres vtables.
- Mapa de instalación contrastado con esos testigos en pruebas sin EXE.
- MenuOwner: 30 escenarios / 139 comprobaciones PASS; seguridad de lecturas:
  18 escenarios / 89 comprobaciones PASS. Incluyen ambos dueños, ambos valores
  del selector stock, estados 5/6/7, otros mandos activos, cambio de actor y
  desaparición del campo +198, sin escribir memoria del juego.
- ABI i386: nueve gateways con callbacks simulados, argumentos/retorno/pila PASS.
- Runtime acumulativo: 217 escenarios / 49.858 comprobaciones PASS.
- Enlace y cinco pruebas ELF/instalador PASS; 212 hooks y diez reemplazos inline,
  sin solapamientos y con reversión exacta al input.

Módulo SHA256: `b6367321edaaa0a12dc0d9c0ebf5034f2c46824c8ce04dd2467bbeedc86c4489`.
Candidato local: `76e11e9377a5dfada25142fadf5462d5b4e837094ca13df22937916ce52d3984`.
No instalado en Steam ni publicado como EXE.

## Configuración independiente todavía pendiente

Esta corrección transporta entrada del mando; **no separa aún los ajustes**.
uOptionManager carga dos cOptionRevelParameter en +4C/+150, mediante
02C196B0, desde 02C2C641/02C2C64F. El primero es editable y el segundo interviene
en restauración/cancelación. La aplicación en 02C2CF0E llama 02C19D90 con dos
argumentos. Ese método puede guardar la configuración global antes de aplicar
cinco valores de PadData y otros ajustes de teclado, ratón y del juego.

Las consultas/setters de esos valores siguen mStartPadNo. Cambiar únicamente
su índice mezclaría la sesión de J2 con el guardado global de J1. Faltan vincular
la sesión de opciones y ambas copias a su dueño, separar su aplicación y su
guardado, revisar restauración/defaults y los accesos de teclado/ratón que
puedan rodear estas llamadas. Tampoco se presenta la navegación completa como
validada jugando. Continúan los restantes requisitos del cooperativo.
