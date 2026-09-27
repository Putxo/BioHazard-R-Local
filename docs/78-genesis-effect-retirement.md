# Genesis: retirada de los tres auxiliares

27/09/2026. Continúa PR49. No se ha ejecutado ni instalado el juego.

El destructor nativo de Scanner desconecta y marca para retirada uFilterSet,
pero no hace esa operación con uGameTVNoiseFilter y uOutlineFilter. Ahora el
backend retira los tres efectos antes de destruir el Scanner local.

La operación exige el permiso estructural de destrucción, los dos grafos
admitidos y sus generaciones observadas. Vuelve a capturar el grafo del clon
y conserva sus flags. Desconecta primero el callback FilterSet+44, que apunta
al objeto embebido Scanner+2F4. Desactiva los seis bits de actividad de cada
auxiliar, incluido keep-alive, y cambia el estado de cUnit de 1/2 a 3, igual
que la transición de retirada nativa. Preserva los bits de grupo y vista.
Finalmente pone a cero los tres punteros del Scanner y verifica el resultado.

Los auxiliares conservan su registro en el grupo 6 y sus recursos: la operación
no los libera directamente ni modifica sus enlaces del planificador. sUnit
conserva la responsabilidad de destruirlos. El destructor posterior de FilterSet
libera su recurso privado y omite el callback desconectado. La destrucción del
Scanner ya no deja ese callback apuntando a su memoria liberada.

Una escritura fallida detiene la operación sin intentar deshacer una retirada
parcial. El backend conserva el Scanner en cuarentena y no llama a su destructor.
Así, incluso si algún auxiliar ya quedó marcado para retirada, el objeto padre
no se libera mientras la desconexión sea incierta. No se modifica el grafo de J1.

Las lecturas de raíces y recurso incorporan comprobaciones de generación antes
y después de cada lectura. Una muerte observada durante la lectura detiene las
lecturas siguientes. Esto complementa la ventana estructural; no sustituye el
requisito de serializar las vidas nativas ni demuestra cobertura de hilos de carga.

## Validación

- 296 comprobaciones de grafo y retirada: siete escrituras, fallo en cada
  posición, escritor que no aplica el valor, muerte durante lectura/escritura,
  ABA, separación de J1 y conservación de identidades hasta la muerte observada.
- 146 escenarios / 32.368 comprobaciones del runtime. El destructor del Scanner
  solo se alcanza tras desconectar sus tres campos; los siete fallos lo impiden.
  La fixture conserva los auxiliares como asignaciones separadas del padre.
- 54 testigos estáticos, compilación freestanding, cuatro pruebas del instalador
  y reversión exacta de la copia acumulativa de 131 hooks.

SHA256 módulo: `e0cffd1b7803f22d9a493f2a61974c222c54ff72bc4a90aef1058cc977db5f39`.
SHA256 copia local: `9be9bd53ff73a1a03f108e3f7f9d1ff4c4047cf4d6072835e28601f09e61290d`.

La liberación posterior por el motor no se ha ejecutado en estas pruebas.
**Genesis permanece inactivo.** Continúan pendientes las fases auxiliares,
recorte/render targets, aislamiento de estados globales y la activación/cierre
con recompensa. Las asignaciones retenidas por fallos de inicialización o
cuarentena tampoco se presentan como una recuperación completa.
