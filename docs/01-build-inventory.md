# 01 — Inventario de builds analizadas

Fecha: 2026-09-23.

Este inventario incluye **solo los cuatro ejecutables aportados en esta conversación**.

Los ejecutables no se suben al repositorio. Los hashes sirven únicamente para identificar exactamente las copias analizadas.

| Build | Tamaño | SHA-256 | PE | Secciones | Observaciones |
|---|---:|---|---|---:|---|
| BioRevHD 30-Enero-2013.exe | 60,748,800 B | `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69` | PE32 i386 | 8 | `FullDebugWin32`; principal candidata |
| BioRevHD 23-Feb-2013 prototipo(2).exe | 16,100,864 B | `f67742e6f8d0f6ade1defc2dcdc34ba06e9e95d2abdc630a605a7da9b437a041` | PE32 i386 | 5 | `ROMReleaseWin32`; conserva interfaz debug pero tiene lógica eliminada |
| rerev May 17, 2013 prototipo.exe | 16,736,600 B | `a015f15a92dd4d242404d83d012a5319d677c15195a0fe862091ebb827dafe34` | PE32 i386 | 6 | `ROMReleaseWin32` |
| rerev Feb 7, 2024 retail.exe | 14,260,576 B | `1573b79eb921b1e7f571e6deb71c39961c7f1940fad6d19283fc3aea7fca48aa` | PE32 i386 | 5 | `MasterReleaseWin32` |

## CodeView / PDB embebido

### 30-Enero-2013

```text
E:\BHR\Source\BioRevHD\buildout\FullDebugWin32\BioRevHD.pdb
```

RSDS age observado: 113.

La build contiene una gran sección `.textbss` y una `.text` de gran tamaño, consistente con una build de desarrollo/depuración rica en RTTI, menús internos y funciones que luego desaparecieron.

### 23-Feb-2013

```text
E:\bhr\pc\BioRevHD\buildout\ROMReleaseWin32\BioRevHD.pdb
```

### 17-May-2013

```text
E:\bhr\pc_qa\BioRevHD\buildout\ROMReleaseWin32\rerev.pdb
```

### 7-Feb-2024 retail

```text
D:\KATARIBE\bhr\BioRevHD\buildout\MasterReleaseWin32\rerev.pdb
```

## Imports relevantes

- Enero 2013 importa `DINPUT8.dll`.
- Enero no muestra `XINPUT1_3.dll` entre sus imports.
- Febrero, mayo y retail sí importan `XINPUT1_3.dll`.

Esto hace útil a enero para reconstruir la arquitectura lógica, pero obliga a separar cuidadosamente la identidad lógica del pad del backend concreto de entrada.

## Secciones de la build de enero

```text
.textbss  RVA 0x00401000
.text     RVA 0x01B79000   raw 0x00000400
.rdata    RVA 0x04C27000   raw 0x030AE000
.data     RVA 0x05425000   raw 0x038ABE00
.idata    RVA 0x057DA000
.didat    RVA 0x057DE000
.tls      RVA 0x057DF000
.rsrc     RVA 0x057E1000
```

Para strings en `.rdata`, la relación raw→VA usada durante este análisis es:

```text
VA = 0x04C27000 + (raw - 0x030AE000)
```
