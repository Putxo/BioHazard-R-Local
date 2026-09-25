# Observaciones de Windows, solo lectura

`tools/runtime_probe.py` es una herramienta de diagnóstico para el candidato exacto `0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378`. No instala el cooperativo, no parchea memoria y no certifica gameplay.

Requiere Python 3.11 o posterior en Windows. Primero se inicia el EXE experimental usando una instalación con datos compatibles. En el Administrador de tareas, pestaña Detalles, se obtiene el PID de ese proceso.

```powershell
py tools/runtime_probe.py --pid 1234 --output observaciones_nuevas.jsonl --seconds 20
```

Sustituir 1234 por el PID del juego. El archivo de salida debe ser nuevo. El script verifica el SHA-256 del ejecutable que usa realmente el proceso y los bytes del módulo `.lcfix` cargado. No acepta el retail ni una versión histórica distinta.

Registra el flag local, el tracker Sub0, serial/ThinkMode, posición y valores de control del actor, pack/herbas, managers/targets de cámara, configuración de dos viewports y huellas de cambio de los dos slots de pad. No guarda un volcado de memoria ni la ruta absoluta del ejecutable.

`local_split_structure_observed` solo significa que esas estructuras coincidieron en una muestra. No prueba que se rendericen ambas vistas, que el HUD sea correcto, que cada mando controle exclusivamente un personaje ni que funcione una campaña. Las muestras no son atómicas: se detecta un cambio del binding durante la captura, pero otras estructuras pueden cambiar entre lecturas.

Errores al cargar una escena o al desaparecer un actor se registran como errores de lectura, no como memoria válida inventada. El probe no crea un P2 cuando la escena no tiene partner.

## Prueba de la herramienta

`tests/test_runtime_probe.py` pasó 12 casos en Windows. Uno abre exclusivamente el propio proceso del test y lee un buffer conocido usando kernel32. Los otros usan memoria simulada para situaciones de actor/cámara ausentes, Network, binding cambiante y coordenadas inválidas. No se ejecutó el juego durante esos tests.

## API y permisos

Se solicitan exclusivamente `PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION` (0x1010). No se solicitan permisos de escritura, suspensión, inyección o terminación.

Referencias oficiales:

- https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess
- https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-queryfullprocessimagenamew
- https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle

El archivo de observaciones no se sube automáticamente a ningún servicio.
