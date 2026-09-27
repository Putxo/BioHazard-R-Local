# Icono Y: productor y arbitraje localizados

27/09/2026. Investigación posterior a PR26. El aislamiento parcial del dibujo
se implementa en [el siguiente bloque](54-action-icon-view-routing.md); el prompt
independiente completo sigue pendiente. El juego no se ha abierto.

El icono no es uno de los 21 widgets del Cockpit. La RTTI identifica el productor
como `uActionIcon2D@action_command`, vtable `0x04CDCA9C`. Su constructor
`0x01EB6F50` guarda un argumento en `+0x40`; no se ha demostrado que ese argumento
sea un actor. Contiene una instancia `uGUI_ActionIcon` en `+0x50` y otro widget
en `+0x2F0`. El destructor libera ambos como miembros embebidos.

`0x01EB7680` obtiene el tipo de comando desde el objeto `+0x40` y llama a
`0x01C10D2E -> 0x02B14150` con ID y booleano. Esta llamada consume ocho bytes de
argumentos. El GUI guarda estado de icono en `+0x290`; ese campo **no es** el
enlace `next` de un BioCockpitGUI. Incorporarlo sin más al backend de Cockpit
sería incorrecto por propiedad y por layout.

El draw `0x01EB73A0` consulta tres veces `0x01C54A3D -> 0x01CFDCB0`, pasando
índice cero. Lee un campo `+0x170` y un indicador `+0x174`, y puede establecer
este último antes de dibujar los dos miembros. Es una ruta de arbitraje
compartido que necesita investigación. Que impida el segundo prompt es una
hipótesis, no un resultado observado jugando. El seguimiento posterior confirma
que el accessor exige índice `<1`: no representa un slot por jugador y no se
debe cambiar cero por uno. El slot único está en `0x0556279C`.

Próximo trabajo concreto: seguir el argumento `+0x40` hasta su propietario,
identificar la creación/reset del contexto de arbitraje y su relación con las
vistas, y separar la selección y publicación de comandos por jugador. Conservar
la propiedad embebida y las rutas de destrucción existentes.

El auditor reproducible `scripts/audit_action_icon.py ORIGINAL.exe` comprueba
36 testigos contra el original January fijado por SHA. No modifica archivos.

Otros campos del Cockpit localizados: Damage `+0x3C`, DamageChoke `+0x40`, Heal
`+0x44`, GadgetScanner `+0x48`, ItemWin `+0x4C`, Scope `+0x54`. Son candidatos
de investigación; el HUD integrado sigue siendo Reticle, MainEquipment,
MapHerb y SubEquipment. No dar estos candidatos por implementados.
