# 98 — Identificación de los cuatro ejecutables de Revelations 1

13/10/2026. Se analizaron los cuatro ejecutables aportados por el propietario,
sin ejecutarlos, parchearlos ni redistribuirlos. Son imágenes x86 PE32
diferentes, aunque los cuatro emplean ImageBase 0x00400000.

| Identificador | SHA-256 completo | Bytes | EP VA | Secciones | Compatibilidad |
| --- | --- | ---: | --- | --- | --- |
| BioRevHD 30/01/2013 | 9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69 | 60748800 | 0x01C897E7 | 8 | Base January del mod |
| BioRevHD 23/02/2013 | f67742e6f8d0f6ade1defc2dcdc34ba06e9e95d2abdc630a605a7da9b437a041 | 16100864 | 0x010E982F | 5 | Pendiente portar |
| rerev 17/05/2013 | a015f15a92dd4d242404d83d012a5319d677c15195a0fe862091ebb827dafe34 | 16736600 | 0x013FE2EE | 6 | Pendiente portar |
| rerev retail 07/02/2024 | 1573b79eb921b1e7f571e6deb71c39961c7f1940fad6d19283fc3aea7fca48aa | 14260576 | 0x011F0310 | 5 | Pendiente portar |

El January original **coincide exactamente** con la entrada verificada por
`tools/build_january_base.py`. Los otros tres binarios no pueden recibir de
forma segura los mismos RVA/hook sites. January incluye una sección
`.textbss` y un mapa de secciones diferente; May y retail incorporan `.bind`.

## Comprobador reproducible

Nuevo `tools/identify_revelations_build.py`: lee y calcula SHA-256, valida las
cabeceras PE32 y el mapa de secciones y solo identifica versiones por el hash
**exacto** y los metadatos esperados. No utiliza el nombre de archivo para
clasificar ni identifica versiones por firmas sueltas. Admite varios archivos y
detecta copias idénticas:

~~~sh
python tools/identify_revelations_build.py "BioRevHD 30-Enero-2013.exe" "rerev.exe" --report build_matrix.json
~~~

Para prevalidar un origen antes de llamar a los constructores actuales:

~~~sh
python tools/identify_revelations_build.py "BioRevHD 30-Enero-2013.exe" --require-mod-target
~~~

Código 0: ficheros PE32 leídos correctamente y, si se usa
`--require-mod-target`, todos cumplen el perfil January.
Código 2: inválido/no legible.
Código 3: al menos un origen no es la base January solicitada.
`--report` nunca sobrescribe un archivo preexistente.

El comprobador complementa, **no sustituye**, las comprobaciones exactas de
hash/bytes incluidas en los constructores e instalador. La coincidencia del
SHA no acredita que el mod funcione jugando.

## Verificaciones

- 36 comprobaciones de parser, contratos SHA/tamaño/EP/secciones, casos
  inválidos, duplicados, salida JSON y negativa a sobrescribir.
- Los cuatro ficheros reales se identificaron con los estados previstos:
  `JANUARY_SOURCE_SUPPORTED`, y tres `PORT_REQUIRED`.
- La lista de uploads incluía una segunda copia binaria idéntica de cada
  versión (SHA-256 iguales).

El estado del cooperativo no cambia: J2 sigue pendiente de retest gameplay
con el binder Cpu(3)->Pad(1) y el reenlace suspendido. Genesis, scripts/QTE,
muerte/checkpoints, cutscenes y escenas sin acompañante siguen incompletos.
Este bloque es un **preflight de compatibilidad**, no un port ni el mod final.
No se ha lanzado ningún juego ni subido EXE, DLL, assets o PDB.
