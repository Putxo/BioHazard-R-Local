# Corrección de la cadena de construcción del control J2

08/10/2026. Continúa PR60, `d3034bb97066ee3d6e3c42150da0ee650a876ca4`.
El propietario informa de que J2 sigue controlado por IA. Todavía no se ha
identificado la copia exacta usada en esa prueba. No se ha abierto el juego.

## Dos defectos distintos

PR60 cambió el comparador del binder activo de 2 a 3. En January el modo
Pad es 1, Network es 2 y Cpu es 3. El código antiguo dejaba fuera al compañero
Cpu normal y podía convertir al actor Network. El setter nativo sigue siendo
`0x01BB8B60`; no basta con escribir el campo de modo directamente.

Sin embargo, `tools/runtime_image.py` en PR60 solo aceptaba los dos hashes de
bases anteriores a esa corrección. Rechazaba la base GNU corregida, a pesar de
lo que afirmaba el documento 88. Además, el constructor intermedio no reconocía
el hash LLVM corregido. La reconstrucción desde el original reprodujo ese
rechazo después de compilar el binder nuevo.

Ahora el paso de scripts reconoce el perfil LLVM corregido y el instalador
acumulativo admite únicamente las bases Cpu3 GNU/LLVM conocidas. Una base Cpu2
anterior produce un error explícito que pide reconstruirla. Los constructores
históricos se conservan para reproducir la cadena, pero sus intermedios no son
entregas nuevas: el módulo local-routing reemplaza su binder al final.

## Comprobación del archivo final

`tools/check_local_input.py RUTA.exe` lee el archivo, sin abrir procesos ni
ejecutarlo. Sigue el salto del bind site `02DF5015` y compara los 108 bytes
del binder conocido, incluidos identidad de Sub0, comparación de modo, ramas,
setter nativo, activación y continuación. Exige sección `.lcfix` de lectura y
ejecución, sin escritura. No busca una coincidencia suelta en todo el EXE.

- `CPU3_BINDER_PRESENT`: contiene el puente corregido; salida 0.
- `LEGACY_CPU2_BUG`: contiene el puente anterior; salida 1.
- `UNKNOWN_IMAGE`: no se reconoce el contrato; salida 2.

La presencia del puente no certifica conexión del mando, acciones, scheduling
del motor ni movimiento jugando. La herramienta no cambia ni repara archivos.
El instalador hace la misma comprobación antes y después de insertar el runtime
y la incluye en su informe JSON.

## Evidencia local

- Puentes compilados: 23 escenarios, 312 comprobaciones; llamadas del motor
  simuladas, incluye Cpu3, Network2 y reenlaces Pad1.
- Inspector: 96 comprobaciones sobre el módulo compilado; mutaciones de guardas,
  saltos y destinos nativos, detección del error anterior y permisos de sección.
  También se ejecuta en los jobs GNU existentes de CI.
- Instalador: cinco tests, incluido rechazo de bases Cpu2 antes de cargar ELF.
- Reconstrucción desde el original January hasta v13, owner fix, módulo LLVM y
  scripts; se reanudó la última etapa tras añadir el perfil que faltaba. Su base
  final coincide byte a byte con una construcción independiente desde owner fix.
- Candidato acumulativo con 184 hooks, reversión exacta y binder Cpu3 conservado.

SHA256 reproducidos con LLVM 22.1.8:

| Artefacto local | SHA256 |
|---|---|
| Local routing Cpu3 | `933aba845d32a4015d7c2aa084ef17514b833c9c467e4651c66abd209f32d659` |
| Base acumulativa Cpu3 | `152bce5dba9eb1a270d2fd392921883e772bfc42682b23497ae72452747814c7` |
| Runtime PR60 | `41df6259f1ab0adf04880c7285c641412a18dffd85d3381f768ec765cd314385` |
| Candidato acumulativo | `9a34b5dbe8eb1199f6545f5ffb37ce39bc97ec14a6d0f9940c4988030024c9c3` |

La copia de trabajo anterior `d64a7648…` fue identificada como `LEGACY_CPU2_BUG`;
la nueva `9a34b5db…`, como `CPU3_BINDER_PRESENT`. Esto no identifica por sí solo
la copia que probó el propietario. Los binarios permanecen locales, no se han
instalado en Steam ni ejecutado. Genesis y los restantes requisitos pendientes
siguen abiertos; no es una entrega de cooperativo completo.
