# Gestores compartidos alterados al activar el Génesis

27/09/2026. Hallazgo estático; no se ha ejecutado el juego ni cambiado todavía
este comportamiento. No tratar un único booleano global como estado de J2.

`02B23120` activa las fases de FilterSet, TVNoise y Outline, pero además realiza
dos cambios que sobreviven a la llamada y afectan al motor compartido:

* `02B2319C -> 01C8007A -> 02B23310` recibe la negación de la activación. Escribe
  `sGameFilters+62B0` y cambia el flag de fase9 del filtro embebido `+6A0`.
  La instancia procede de `05563C04`, slot0 de la tabla de sistemas de zonas.
  El tipo concreto es `sGameFilters` (VT `04DB7C9C`); el constructor invoca la
  base de zonas y sustituye la vtable. Identificar solo la base como
  `sZoneManager` ocultaba la función real de esta instancia.
* `02B2324D -> 01C6D114 -> 02B232C0` escribe `sHunterManager+65`. La instancia
  procede de `0556DD74` y su VT concreta es `04D0939C`.

El primer estado se consulta en `02930678`: impide que la actualización del
gestor vuelva a habilitar la fase9 de `+6A0`. Por tanto, restaurar solamente un
flag de dibujo inmediatamente después de activar el Scanner no reproduce el
estado persistente esperado.

El segundo tiene getter `01C333C9 -> 021FEE80`. Su llamada directa localizada
en `021FE05D`, dentro de `021FD970`, cambia un valor usado posteriormente en
parámetros de materiales del Hunter. Un OR entre los dos jugadores mantendría
el modo activo al cerrar uno, pero seguiría compartiendo los materiales entre
las dos vistas. No se ha implementado esa simplificación como aislamiento.

Próximo trabajo: conservar el estado de activación por propietario y separar
los consumidores de renderizado sin alterar el comportamiento de la otra vista.
La existencia de estas dependencias es una de las razones por las que la
activación de Genesis J2 sigue deshabilitada en el candidato acumulativo.
