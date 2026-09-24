# 13 — Línea canónica de versiones v1 → v10

Este archivo evita la ambigüedad entre **builds originales del juego** y **versiones experimentales del parche** creadas durante este chat.

## Regla

- Las builds originales son Enero/Febrero/Mayo/2024.
- v1..v10 son iteraciones del parche.
- Solo **v10** es la base canónica actual.
- Las versiones anteriores se conservan por trazabilidad, no como alternativas equivalentes.

---

## v1 — P2 INPUT EXPERIMENTAL

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT EXPERIMENTAL.exe`

SHA-256:

`6c55444a820d05d0b0a15cafad0e8e7d0ec5bc67c1a548631a47d9ef2e6f0729`

Idea:

- redirigir modos secundarios a la ruta local;
- devolver selector 1.

Problema descubierto después:

- los getters PC de `sGamePad` ignoraban ese selector;
- usaban `mStartPadNo` global.

**SUPERSEDED.**

---

## v2 — SPLITSCREEN EXPERIMENTAL

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP SPLITSCREEN EXPERIMENTAL v2.exe`

SHA-256:

`9952e14daedfa05afceea6e1456f7d18aa1ed071ea900c7aef2cc7c8eeeec089`

Añadía cámara/viewports al experimento anterior.

Hallazgos posteriores:

- parte de la interpretación de cámara necesitó correcciones;
- el input heredado seguía teniendo el problema de `mStartPadNo`;
- `mPadNo` de `uCameraManage` no quedó demostrado como selector físico.

**SUPERSEDED.**

---

## v3 — selector PC restaurado

Input output:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT v3.exe`

SHA-256:

`ee05fc7d6c965aed0661165eb0ead4c6dffa1f67bc32fd758297edb86f3213e1`

Combinado:

`BioRevHD 30-Enero-2013 LOCAL COOP v3 INPUT+SPLITSCREEN.exe`

SHA-256:

`6978dc6d63781718c3728160c0312737424536734c0850b9337e990fb2da677d`

Mejora:

- restaura el uso del selector en getters PC;
- interpreta correctamente ThinkMode Pad/Cpu/Network.

Limitación:

- todavía se basaba en una selección demasiado amplia de la ruta NPC;
- aún no filtraba el actor exacto Sub0.

**SUPERSEDED.**

---

## v4 — native camera split

Camera-only:

`BioRevHD 30-Enero-2013 CAMERA SPLIT v4.exe`

SHA-256:

`12ca6dae126e3e7354719637e144253d10a72cc6a8739de704f74e8aa0bcf172`

Combinado histórico:

`BioRevHD 30-Enero-2013 LOCAL COOP v4 NATIVE SPLIT.exe`

SHA-256:

`2b0e7b71cbac09cfb7fd5f3904f2e4709b2fdbfcbf372880e79f301cb95718ad`

Aporta:

- Self/Partner `uCameraManage`;
- VIEW_0 / VIEW_1;
- TOP/BOTTOM nativo.

Problema:

- el split de esa etapa era incondicional y no estaba todavía ligado de forma limpia al estado local.

**Componente histórico; superseded por cámara persistente/condicional v9.**

---

## v5 — filtro exacto Sub0 manteniendo Cpu

Input output:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 SUB0 INPUT v5.exe`

SHA-256:

`d294508a5fe4919eb9cb46c446f23ba7a79eb1da8664dac3d7cf9ed3437068b7`

Combinado:

`BioRevHD 30-Enero-2013 LOCAL COOP v5 SUB0 NATIVE SPLIT.exe`

SHA-256:

`e83207b2c64172468930eacbe0460fb7847061b56ac3d4e3a3b7f3ee009bfe9c`

Mejora:

- identifica el `uNpc` exacto unido a `uPcsPlayerSub0`;
- solo ese actor devuelve selector 1;
- Network queda intacto.

Limitación:

- Sub0 seguía oficialmente en ThinkMode::Cpu;
- otros subsistemas podían seguir tratándolo como AI.

**SUPERSEDED por v6.**

---

## v6 — canonical Sub0 Cpu → Pad

Input output:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 SUB0 PADMODE v6.exe`

SHA-256:

`83554753d1d6a86bf256627830f509a313871a847c87338b61da0e3c0a2c0914`

Combinado:

`BioRevHD 30-Enero-2013 LOCAL COOP v6 SUB0 PADMODE NATIVE SPLIT.exe`

SHA-256:

`18a429daa0855a7e6de6af91358d867970beee9a99671913303166f8b39a540d`

Mejora:

- exacto Sub0;
- solo si está Cpu(2);
- usa setter/wrapper oficial para Pad(1);
- Network(3) intacto.

**Base conceptual correcta de identidad/ThinkMode.**

---

## v7 — FULLPAD

Input-only:

`BioRevHD 30-Enero-2013 LOCAL COOP P2 FULLPAD v7.exe`

SHA-256:

`25cb559a183156bbb8de312688da86bec300bd78e66f0d6c6f7d776a3cc88af7`

Combinado histórico:

`BioRevHD 30-Enero-2013 LOCAL COOP v7 FULLPAD NATIVE SPLIT.exe`

SHA-256:

`fe12f35c1d08f61961d7bdefba19a09e68939bb8966fc3d6c50e994ab699fa76`

Mejora:

- restaura el selector en todos los sitios `mStartPadNo` identificados de la ruta de input NPC;
- 75 cargas conocidas cubiertas;
- exacto Sub0 -> PadData[1].

**v7 input-only es la base de la que parte v9.**

---

## v8 — persistent split, pero con herencia incorrecta

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP v8 FULLPAD PERSISTENT SPLIT.exe`

SHA-256:

`c9a4c1e9887eb3504905734cb0279b2c8fe61ec55bb69768c55724b3b7ec8356`

Aporta:

- `gLocalCoopActive`;
- target exacto Sub0 para Partner;
- Self/Partner simultáneos;
- persistencia del split.

Problema crítico de diseño:

- se construyó sobre **v7 combinado**;
- heredó el split v4 incondicional;
- por tanto el comportamiento cámara no era completamente stock cuando el flag local estaba apagado.

**SUPERSEDED. NO USAR COMO BASE.**

---

## v9 — CLEAN PERSISTENT SPLIT

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP v9 FULLPAD CLEAN PERSISTENT SPLIT.exe`

SHA-256:

`5dc7a7a413ce5916c2758be43b3107d5adf00beee93533b4acf5d83cec97c4be`

Se reconstruye sobre **v7 input-only**.

Aporta:

- `gLocalCoopActive = 0x057D9188`;
- solo se activa tras exacto Sub0 Cpu→Pad;
- Partner target = exacto Sub0;
- Self VIEW_0 TOP;
- Partner VIEW_1 BOTTOM;
- persistencia;
- cuando flag=0, inicialización de cámara stock vuelve a ser byte-identical al original.

**Base canónica de input+cámara.**

---

## v10 — CLEAN SPLIT + COOP ITEMBOX

Output:

`BioRevHD 30-Enero-2013 LOCAL COOP v10 CLEAN SPLIT COOP ITEMBOX.exe`

SHA-256:

`5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6`

Base:

v9 exacto.

Único cambio nuevo:

```cpp
ItemBox_isCoop_for_selection =
    stock_isCoop() || gLocalCoopActive;
```

Wrapper:

```text
0x01C95100
```

Callsite:

```text
0x01D067AF
```

Diferencia vs v9:

- 23 bytes;
- 2 rangos;
- mismo tamaño.

Linaje comprobado:

- quitar esos 23 bytes funcionales y restaurar llamada original reproduce exactamente SHA de v9.

**BASE CANÓNICA ACTUAL.**

---

# Regla para v11

No crear v11 por “investigación nueva”.

Solo crear v11 si se demuestra un cambio de código necesario.

El candidato más probable, si hace falta, sería un hook mínimo de pickup del partner **solo si** `cPickupItemSyncData` demuestra que la aplicación correcta está condicionada exclusivamente a red.

Hasta entonces:

**seguir analizando v10, no parchear.**
