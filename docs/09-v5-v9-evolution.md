# 09 — Evolución v5 → v9 (solo esta conversación)

Este documento resume la evolución posterior a v4 sin borrar ninguna versión intermedia.

## v5 — Sub0 identity tracking

Archivos documentados:
- `research/manifests/p2-sub0-input-v5.json`
- `patches/build_v5_sub0_local.py`

Cambio principal: en vez de tratar cualquier NPC/modo secundario como P2, se captura el `uNpc*` exacto enlazado por `uPcsPlayerSub0`.

Se reserva:
```
0x057D9184 = gLocalCoopSub0Npc
```

El selector devuelve 1 únicamente cuando `this == gLocalCoopSub0Npc`.

## v6 — transición nativa Cpu → Pad

Archivos:
- `research/manifests/p2-sub0-padmode-v6.json`
- `patches/build_v6_sub0_padmode.py`

v6 deja de redirigir heurísticamente ThinkMode::Network.

Solo el Sub0 exacto, cuando está en:
```
ThinkMode::Cpu (2)
```
se cambia mediante la ruta virtual/canónica a:
```
ThinkMode::Pad (1)
```

ThinkMode::Network (3) permanece intacto.

Esto es preferible porque el setter completo `0x027F1290` ejecuta también la limpieza/transición propia del motor antes de escribir `uNpc+0xE40`.

## v7 — FULLPAD

Archivos:
- `research/manifests/p2-fullpad-v7.json`
- `patches/build_v7_fullpad.py`

Se comprobó que la versión PC había colapsado muchas APIs de entrada al global:
```
sGamePad::mStartPadNo
```

v7 restaura el argumento selector en todas las APIs identificadas como consumidoras del selector de `uNpc`, no solo movimiento/aim/run.

Cobertura registrada:
```
129 direct selector calls found
75 selector-load sites restored in relevant NPC input path
```

Resultado:
```
Sub0 -> selector 1 -> PadData[1]
otros -> selector 0 -> PadData[0]
```

La build input-only v7 es la base limpia de v9.

SHA-256 input-only v7:
```
25cb559a183156bbb8de312688da86bec300bd78e66f0d6c6f7d776a3cc88af7
```

## v8 — persistent split

Archivos:
- `research/manifests/p2-fullpad-persistent-split-v8.json`
- `patches/build_v8_persistent_split.py`
- `research/patches/camera_persistence_v8.S`

v8 introduce dos mejoras importantes:

1. flag local separado:
```
0x057D9188 = gLocalCoopActive
```
2. persistencia de cámara:
   - target Partner = Sub0 exacto mediante setter stock `0x01C275EC -> 0x02066AB0`;
   - Self/Partner pueden coexistir cuando el flag local está activo;
   - Partner usa VIEW_1;
   - Self VIEW_0 TOP, Partner VIEW_1 BOTTOM;
   - hooks reafirman activación cuando rutas stock intentan apagar Partner.

### Corrección descubierta después

v8 se construyó sobre:
```
v7 FULLPAD NATIVE SPLIT
```
que ya incluía el hook v4 de cámara incondicional en `0x0203E3A9`.

Por tanto la afirmación “cámara completamente stock cuando flag=0” no era estrictamente cierta en v8.

v8 se conserva sin modificar porque forma parte del historial solicitado.

## v9 — CLEAN PERSISTENT SPLIT

Archivos:
- `research/manifests/p2-fullpad-clean-persistent-split-v9.json`
- `patches/build_v9_clean_persistent_split.py`
- reutiliza `research/patches/camera_persistence_v8.S`

v9 toma la lógica de persistencia de v8 pero se construye sobre **v7 input-only**, eliminando la herencia del split v4.

Output local:
```
BioRevHD 30-Enero-2013 LOCAL COOP v9 FULLPAD CLEAN PERSISTENT SPLIT.exe
```

SHA-256:
```
5dc7a7a413ce5916c2758be43b3107d5adf00beee93533b4acf5d83cec97c4be
```

Verificación:
```
same file size: yes
854 bytes different vs original
88 ranges vs original
351 additional bytes vs v7 input-only
11 additional ranges vs v7 input-only
```

Garantía específica de v9:
```
0x0203E3A9..0x0203E463
```
es byte-idéntico al ejecutable original.

Así:
- fuera de una sesión convertida localmente, la inicialización de cámara permanece stock;
- ThinkMode::Network no activa el flag local;
- el split se arma solo tras la transición exacta Sub0 Cpu→Pad.

## Target de Partner verificado

Setter:
```
0x01C275EC -> 0x02066AB0
```

La implementación:
- guarda el actor en `uCameraManage+0x74`;
- deriva un handle/interfaz auxiliar en `+0x78`;
- el update normal de cámara valida el target y llama al mismo setter con null cuando deja de ser válido.

Por tanto v9 usa el mecanismo nativo de target; no inyecta un puntero en un campo inventado.

## mPadNo de uCameraManage

El metadata confirma:
```
uCameraManage + 0x88 = mPadNo
```

pero no se ha demostrado una conexión con `sGamePad`.

Se inicializa a -1 y los usos localizados están relacionados con serialización/condiciones especiales. No hay una ruta normal confirmada Self=0 / Partner=1.

Por ello v9 **no modifica mPadNo**.

## Estado

v9 es la base estática preferida actual.

Todavía requiere prueba real en Windows para:
- asignación física de PadData[1];
- render de ambas cámaras;
- culling/colisiones/cutscenes;
- HUD;
- inventario/pausa;
- interacciones/QTE;
- transición entre escenas.
