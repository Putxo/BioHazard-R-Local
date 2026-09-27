# Genesis: pertenencia de efectos y máscaras de sus unidades principales

27/09/2026. Continúa PR48. No se ha abierto, ejecutado ni instalado el juego.

El backend ya incluye los tres auxiliares del Scanner en la admisión posterior
a initialize y en sus comprobaciones de fases, productores y retirada. Antes
solo comprobaba los recursos GUI y las listas; un efecto destruido o un recurso
de filtros sustituido podía conservar un HUD aparentemente válido.

La captura requiere una generación observada del tipo correcto antes de leer
cada uFilterSet, uGameTVNoiseFilter y uOutlineFilter. Comprueba VT, grupo 6,
estado 1 o 2 de cUnit, callback de FilterSet en Scanner+2F4 y padre del callback.
Captura también rFilterSet, tabla y bases de sus hijos mediante el certificado
introducido en PR47. Realiza dos capturas y conserva las identidades admitidas.
Una muerte seguida de reconstrucción en la misma dirección cambia la generación
y no puede recuperar la admisión anterior.

Se excluyen solapamientos entre las dos asignaciones Scanner, los tamaños
concretos de sus tres auxiliares, recursos de filtros, tablas y bases de hijos.
En cada uso se comparan tanto el grafo original como el del clon con sus
identidades iniciales; cambios de puntero, callback o generación bloquean la
llamada nativa al Scanner. No se usan punteros encontrados como prueba de vida.

Durante la preparación estructural, el runtime establece la máscara 2 (View1)
en las tres unidades principales de J2. Solo cambia los diez bits de vista en
+C, preservando grupo, estado, actividad y demás flags. No escribe en los efectos
de J1. Una escritura fallida o una captura incompleta deja el clon en cuarentena.
Las máscaras se vuelven a comprobar junto con las identidades.

La ventana estructural del backend sigue siendo una condición necesaria: el
observador no convierte las capturas en bloqueos de memoria. Los eventos desde
otro hilo invalidan conservadoramente el observador. Debe resolverse la posible
construcción de filtros ajenos en hilos de carga antes de declarar cobertura de
carga asíncrona. El certificado no enumera todos los render targets, texturas o
asignaciones internas de cada filtro.

## Validación

- 151 comprobaciones del grafo: identidades ausentes, ABA, callback prestado,
  recursos o unidades compartidas, alias interiores, estado muerto, cambios
  durante lectura y muerte durante captura. Una raíz no observada no se lee.
- 138 escenarios / 30.194 comprobaciones del runtime. Nueve fallos durante uso
  impiden la fase nativa; la preparación rechaza identidades ausentes o una
  escritura fallida sin modificar los efectos de J1.
- 47 testigos estáticos SHA fijados, compilación freestanding, cuatro pruebas
  del instalador y reversión exacta de la copia acumulativa de 131 hooks.

SHA256 módulo: `f2c2b53a8ce4cce38e3f3bbf75f42e4717bf1b9371c9febfadb56dd8215f58c7`.
SHA256 copia local: `e7d5c5acd37bca0e0a13a51e6fe65aa1f607884516459788285a8d117c500310`.

## Continúa pendiente

**Genesis sigue inactivo.** Las máscaras de las unidades principales no prueban
recorte de sus render targets ni aislamiento de los estados globales de
activación. Los auxiliares siguen programados por sUnit fuera del HUD; falta
controlar sus fases/dibujo, evitar que los efectos de J1 entren en View1 y
completar su retirada. El destructor nativo del Scanner solo retira +2F0;
no atribuirle una limpieza de +338/+33C que no realiza. La activación y el cierre
con recompensa se conectarán después de resolver estas fronteras.
