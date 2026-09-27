# Construcción acumulativa y 35 enganches HUD/menú

27/09/2026. Continúa PR24 (`7f1f3029ea2d63dac8527834eac51c4496381ac3`).

El pipeline reconstruye desde el original January, enlaza el módulo y escribe
una copia independiente. Nunca arranca el juego ni instala archivos en Steam.
No es todavía una entrega de cooperativo completo.

## Fallos corregidos durante la integración

1. `manager_gateways.S` cambiaba a `.note.GNU-stack` antes de emitir las dos
   salidas de máscara P1. El linker eliminaba esa sección y dejaba los símbolos
   de las salidas sin definir, aunque terminase con código 0. Se mueve la nota al
   final. La lectura del ELF ahora rechaza cualquier símbolo sin resolver y exige
   que los 35 destinos y el entry estén dentro de `.revtext`.
2. El builder v11 requería un helper de 248 bytes, pero la ruta de fuente había
   sido sustituida por un experimento de 55 bytes. Se recupera el fuente del
   commit `7f7998fc2ab5e765fec243f3742971a7b8274145` con nombre propio.
3. Los saltos absolutos numéricos de `local_routing/hooks.S` pasan a símbolos de
   linker con los mismos destinos para permitir ensamblado LLVM y GNU.

## Compilación

Python estándar y LLVM; base reconstruida localmente con clang/LLD 22.1.8.
Otra versión que produzca bytes distintos se rechaza en la siguiente etapa;
no hay una opción para ignorar hashes.

```text
python tools/build_january_base.py ORIGINAL.exe BASE_DIR --clang CLANG --linker LLD
python tools/build_runtime.py MODULE_DIR --clang CLANG --linker LLD
python tools/runtime_image.py BASE_DIR/january-script-base.exe MODULE_DIR/runtime.elf NUEVO.exe --report NUEVO.json
```

Los directorios de salida deben ser nuevos. El original aceptado tiene SHA256
`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.
No acepta los ejecutables Feb/May/retail como si compartiesen direcciones.

El adaptador de builders históricos solo cambia rutas y transporte al compilador;
mantiene sus comprobaciones de bytes y SHA. Hasta owner-corrected v14 coincide con
`e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a`.

La nueva variante LLVM de local-routing es
`118c06695e4a4cea8fffcc9ae5e9c8b7f4621a608f436d730d1ea4851b2b2d34`.
Con el mismo helper serial de 72 bytes produce
`506ddc362abfa3f330b6d9d8dc75e4ca81c6b6f31bccb6568427899e26613b7b`.
Son perfiles explícitos adicionales; no se atribuye el hash de GNU a LLVM.

## Instalador

Exige base conocida, PE32 sin relocación/ASLR, entry original, espacio libre en
headers y bytes esperados en todos los sitios. Añade `.revtext` RX y `.revdata` RW,
actualiza entry, tamaños y checksum, y verifica la reversión al input exacto.
Rechaza hooks solapados y destinos fuera de código; no escribe sobre el input.

La copia local de integración tiene hash
`4723a7d028408247d919774ffdf124271a792d1865097ed3c636f0598a037a13`.
El módulo original del mod tiene hash
`e28504118cf3641c2330f31308ba8f0fb280b60ab810dd987e18170a94577d90`.
Ningún EXE, recurso o símbolo propietario se publica en GitHub.

Pruebas locales: 35 destinos enlazados y disjuntos; cuatro tests del instalador
y ELF (incluye la regresión de las salidas descartadas); 23 escenarios/311
assertions de puentes local-routing con LLVM; integración Win32 11/711 de PR24.
Las operaciones del motor están simuladas. No se ejecutó código del juego.

## Límites funcionales

El HUD sigue limitado a Reticle/MainEquipment/MapHerb. Faltan HP, subarma/granadas,
prompts e integración visual completa de Genesis. También faltan cobertura de
acciones/scripts/QTE, muerte/checkpoints/cutscenes y escenas sin partner.
El END observado corresponde al ciclo de CPU; no acredita por sí solo un fence
GPU. El propietario realizará la comprobación jugando; está prohibido abrir el
juego desde esta tarea.
