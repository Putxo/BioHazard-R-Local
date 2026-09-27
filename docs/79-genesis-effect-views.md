# Efectos del Génesis por vista en sUnit

27/09/2026. Continúa PR50. No se ha ejecutado el juego; activación pendiente.

Los tres auxiliares del Scanner se dibujan fuera del recorrido de Cockpit.
La máscara View1 del clon no basta: los auxiliares originales todavía pueden
incluir View1. Los dos enganches existentes de sUnit, `03268708` y `03268918`,
ahora pasan por `Runtime::unit_mask`, que compone el filtro de efectos y el de
ActionIcon. Siguen siendo 131 enganches y se conserva el getter nativo inicial.

Para las seis raíces certificadas, el runtime comprueba generación observada,
sesión/actor/frame, grafo privado completo ya admitido y máscara del clon.
J2 recibe `original & 2`; J1 recibe `original & ~2`, conservando sus vistas
auxiliares y sin escribir sus flags. Una máscara cero nunca se amplía.
Una identidad nueva en una dirección reciclada recibe el comportamiento stock.
Si falla la validación, se oculta la raíz antigua de J2 y se conserva la máscara
original de J1. La reentrada no vuelve a inspeccionar el grafo.

Fuera del hilo propietario no se accede a los certificados de los efectos.
El selector integrado en PR29 fuerza el dibujo inmediato nativo para View0/1
admitidas. Esto no da permiso para ejecutar dibujos locales desde un worker ni
demuestra que todos los productores asíncronos estén cubiertos.

Las cuatro fases del callback embebido del Scanner (`04DE4564`, slots6–9)
resuelven a funciones sin efectos que solo regresan con `ret 4`. El FilterSet
sí ejecuta las fases de sus recursos hijos y reenvía el contexto al dibujarlos.
Esta evidencia descarta un supuesto acceso al actor en esos callbacks; no
certifica el recorte de píxeles ni los estados globales de los filtros hijos.

Validación: 159 escenarios Runtime / 40.279 comprobaciones, incluidos los 1024
valores de máscara para las seis raíces, vida reciclada, recursos compartidos,
fallo de lectura, retirada del actor durante la captura y reentrada. ABI x86
sintética; 69 testigos estáticos SHA-pinned; enlace freestanding; cuatro pruebas
del instalador y reversión exacta de los 131 enganches de la copia independiente.

Módulo: `a15ca30bebadec5dcc06f68cf6708d01283c37a633b10c9bbaea1c313577a44f`.
Copia: `34eba3eecba2213b4970f18b948f047a79b72812a227343de6320d29580765c7`.
Ningún binario propietario se publica. Siguen pendientes el recorte nativo,
los cambios globales de activación y la cobertura de carga asíncrona.
