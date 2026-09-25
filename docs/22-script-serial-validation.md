# 22 — Candidato acumulativo: selector de guion con validación de actor y serial

Fecha: 2026-09-25. Continúa el trabajo de `docs/21-pcs-script-input-ownership.md`; conserva el constructor anterior como histórico, no lo sobrescribe.

## Resultado construido

Original de enero, analizado exclusivamente en local:
`9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.

Se reconstruyó independientemente la base LOCAL ROUTING EXPERIMENTAL desde los fuentes publicados en `5bcf15adf2f3e6526c71d5b6d3fdbeb9a841559f`. Coincidió exactamente con `0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378`, 60.755.456 bytes.

Nuevo candidato SCRIPT SERIAL GUARD EXPERIMENTAL:
`71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4`.

Tamaño: 60.755.456 bytes. Cambia 92 bytes efectivos respecto a esa base, incluido el checksum PE. No cambia las secciones existentes ni su tamaño. La reversión reproduce la base byte por byte.

## Implementación

Dos callbacks de miembro, `cFsmAction::0x02976C30` y `cFsmActionPcs::0x029FF600`, se dirigen a un helper de 72 bytes en `0x01C95340`. El helper devuelve 1 únicamente cuando:

- el owner no es nulo y tiene la vtable exacta `cFsmActionPcsSub = 0x04DCF41C`;
- el modo local está activo y existe el Sub0 rastreado;
- `owner+0x1078` coincide con ese actor;
- el actor sigue en ThinkMode::Pad;
- su serial `actor+0xE3C` no es negativo y coincide con `owner+0x1074`.

Todos los demás casos devuelven 0. El helper conserva ECX y los registros no volátiles, usa EAX/EDX como volátiles y mantiene `ret 4`. La comprobación de clase precede a cualquier lectura del layout extendido de PcsSub. No se desreferencia el puntero de contexto sin comprobar su identidad.

El setter `0x02DCFBC0` escribe el serial en `+0x1074` y el contexto en `+0x1078`; el productor obtiene ambos de las tablas de sPcsManager. Esta concordancia se exige explícitamente, no se presupone a partir del nombre Sub.

`uPcsInput::0x02DEE8D0` permanece intacto: su ownership de jugador todavía no está demostrado. Tampoco cambian los módulos `.lcfix/.lcdata`, la selección global Self, GameMode, el serial de red ni las correcciones anteriores de pickup/puertas/cámara.

Fuentes: `patches/build_script_serial_guard.py` y `research/patches/script_member_serial_guard.S`.

## Verificación realmente ejecutada

Pruebas locales: nueve tests sin omisiones, 1.038 escenarios del intérprete limitado del selector y 36 comprobaciones de imágenes reales (hashes, RTTI, delegates, prefijos, integridad, reversión). Informe `research/reports/script-serial-tests.json`.

CI de commit `79e86fc19e8494ff0865665ed1e7fb4d34e0afcd`, run `36127817884`: Linux y Windows x86 correctos. Windows ejecutó los 72 bytes exactos del helper en 596 escenarios y comprobó valores reales de EAX, ECX, EBX/ESI/EDI/EBP y ESP. El código de prueba se reservó dinámicamente; las dos referencias globales mantuvieron sus direcciones originales. No se ejecutó ninguna función del motor. Informe descargado del artifact: `research/reports/script-native-win32-ci.json`.

Los primeros dos intentos de CI fallaron en la infraestructura de prueba (ensamblador COFF frente a ELF, y colisión de direcciones sintéticas con Python); no se ocultan ni se presentan como pasados. El último intento reemplaza la prueba Windows por ejecución nativa del helper, no por una omisión silenciosa.

## Aplicación desde el original sin compilador

`tools/portable_script_patch.py` genera un manifiesto de diferencias a partir del original y candidato exactos. Su modo `apply` aplica ese manifiesto al original de enero usando solo Python estándar. Rechaza hashes distintos, bytes inesperados, cambios solapados, payloads excesivos y destinos existentes. La aplicación exige el hash final anterior y revierte en memoria antes de crear la salida.

La distribución local contiene el manifiesto con SHA-256 `05b6512a394e4bcea4e919fc065bdf45904203494b10ae0b3d87fac9b0f12621`. Se probó la reconstrucción completa original→candidato y ocho rechazos negativos. Ni original ni EXE derivado se suben al repositorio.

## Alcance y continuación

Este resultado implementa la selección de P2 para la subclase de guion identificada, no todos los QTE de la campaña. Los tests no ejecutaron Resident Evil Revelations. Siguen abiertos HUD/menús por jugador, checkpoints/muerte/cutscenes, escenas sin partner y el orden/lifetime de callbacks dentro del motor.

El siguiente análisis de HUD está documentado en `docs/23-herb-hud-owner-audit.md`. No volver a empezar por v12 ni por `0x049A8023`.
