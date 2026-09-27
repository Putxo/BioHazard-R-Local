# Dibujo de ActionIcon separado por miembro y vista

27/09/2026. Implementación parcial para January 30, 2013. **No completa los
prompts independientes ni el cooperativo. No se ha ejecutado el juego.**

El dibujo original de `uActionIcon2D` no filtra el miembro del comando y consulta
el indicador compartido `sActionCommand+0x174`. El nuevo router permite dibujar
un comando del miembro 0 solamente en View0, y del miembro 1 solamente en View1.
Cada vista conserva su propio indicador durante el ciclo CPU de dibujo.

Además, el constructor selecciona un solo bit de vista en `0x01EB6FCF`.
Dos bridges en los filtros de listas de dibujo de sUnit (`0x03268708` y
`0x03268918`) remapean los bits locales 0/1 al miembro del comando. Conservan
una máscara cero o limitada a vistas auxiliares. Solo afectan a la vtable
exacta de ActionIcon2D; otras unidades reciben el resultado stock sin cambios.
No escriben la máscara del objeto ni interceptan globalmente su getter.

Se usa el accessor nativo de miembro `0x01E8CDC0`, que consulta el delegado en
`cActionCommand+0x34`. Un delegado vacío devuelve miembro 0. No se deduce el
miembro desde el tipo de objeto o la proximidad. El router exige las vtables
exactas auditadas de icono, comando y manager, dos actores Pad distintos y vivos,
epoch y frame válidos. Las clases derivadas no auditadas se rechazan en modo local.

El manager es **único**: su accessor limita el índice a `<1` y usa la tabla en
`0x0556279C`. No existe evidencia de un segundo slot para J2. El router guarda
el byte `+0x174`, presta el indicador de la vista durante el draw stock y restaura
el byte original; conserva los otros tres bytes del word tal como quedaron al
volver del draw. No cambia `+0x170`, `Self`, seriales ni propietarios de recursos.

El hook de nueve bytes en `0x01EB73A0` pasa ECX y el contexto al router y vuelve
con `ret 4`. El trampolín reproduce el prólogo y continúa en `0x01EB73A9`.
La vista se obtiene del byte `context+0x158`. El grafo Runtime aporta el frame de
PipelineClock y las vidas de LifetimeSource en su hilo propietario. Se vuelve
a validar la sesión después del delegado. Ante un fallo de restauración se
inhiben llamadas posteriores; nunca se escribe sobre un manager reemplazado.
Los objetos siguen siendo propiedad del motor durante la llamada síncrona:
esto no implementa observación del lifetime propio de ActionCommand/ActionIcon.

Los snapshots locales certifican el hilo propietario antes de acceder al estado
mutable del router. El filtro de listas también cubre la ruta que agenda trabajo,
pero un draw local ejecutado en otro hilo se rechaza: no se ha demostrado ni
implementado soporte de ejecución del icono en workers.

Fuera de cooperativo local se llama al dibujo stock. En modo local sin evidencia
válida se omite el dibujo. Las llamadas anidadas también se rechazan.

## Comprobaciones reproducibles

- `scripts/audit_action_icon.py ORIGINAL.exe`: 45 testigos fijados por SHA.
- `router_tests.cpp`: 51 escenarios y 181 comprobaciones con memoria simulada.
- `runtime_tests.cpp`: 16 escenarios y 1231 comprobaciones; incluye cierre de
  frame, hilo incorrecto y muerte del actor antes de dibujar.
- `gateway_abi.S`: pila thiscall, ECX, argumento, registros preservados y
  prólogo de 0xFC bytes, con continuación sintética. No carga código del juego.
- Módulo freestanding enlazado; cuatro pruebas del instalador pasan; copia
  acumulativa de trabajo con 39 hooks y reversión exacta a la base original.

SHA256 del módulo LLVM 22.1.8:
`8c7953ff9741f825cf65c7156496058787e2c305f8ce79f0ad0011414c75a053`.
SHA256 de la copia local acumulativa, no distribuida ni ejecutada:
`6ef2d856974ea0613b3a3f409bf53c94c113487b258ec29eb654bc96d6ba314c`.

## Pendiente que impide declarar terminado el icono Y

`sActionCommand::update` en `0x01EA5820` sigue arbitrando globalmente. La
comparación en `0x01EA6235` puede salir del bucle por prioridad; el resultado
se publica en `manager+0x170` en `0x01EA66BF`. El filtro de dibujo no recupera
comandos que esta actualización dejó fuera. Debe separarse la selección por
miembro, junto con sus listas/flags, antes de afirmar que ambos prompts aparecen
en todos los casos. Tampoco se ha completado el vínculo con productores 3D,
todos los tipos de evento, posicionamiento, o las interacciones simultáneas.

Estas pruebas demuestran contratos del código nuevo y puntos estáticos de
enlace. La presentación, las acciones y el comportamiento jugando quedan sin
validar, conforme a la prohibición de abrir el juego.
