# 19 — Continuación: reenlace de Sub0 y cadena de interacción

Fecha: 2026-09-25. Rama base integrada: `ae57c34eead9d178ffb1f8bf03402711aa7707a8`.

Se integró PR #3 tras repetir sus 44 comprobaciones y reproducir exactamente el candidato `e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a` desde el v13 adjunto. No equivale a gameplay validado.

## Fuentes locales identificadas otra vez

- Enero original: `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.
- v13: `3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa`.
- Corrección de owner sobre v14: `e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a`.

Las tres copias tienen 60.748.800 bytes. El análisis nuevo se hace sobre sus bytes locales, no sobre un EXE de GitHub.

## Nuevo defecto reproducible de lógica: reenlace del mismo actor

`0x02DF4F30` conserva el actor anterior en `[ebp-0x20]`. Si pasa sus dos controles iniciales, limpia `uPcsPlayer+0x44` en `0x02DF4FA7`, recorre de nuevo la colección y escribe el actor encontrado en `0x02DF501F`.

El parche heredado intercepta la limpieza y pone tanto `gSub0Npc` como `gLocalCoopActive` a cero. Su ruta posterior para un actor que ya está en Pad exige `gLocalCoopActive == 1`. Por tanto una segunda ejecución completa del binder con el mismo Sub0 ya convertido pierde el indicador local. Esto es una contradicción entre los dos hooks, independientemente de la frecuencia real de invocación del binder.

La corrección en desarrollo guarda el indicador y el tracker anteriores en dos locales nuevos del mismo frame; no usa un global compartido para la instantánea. Restaura el indicador solamente si el actor encontrado está en Pad y coincide tanto con el tracker previo como con el binding anterior. Network y actores Pad no convertidos previamente no activan esa restauración.

Se amplía coherentemente el frame de `0x114` a `0x11C`: reserva, inicio del relleno de depuración, número de DWORD y liberación. Los campos originales y el descriptor RTC mantienen sus offsets relativos a EBP.

## Cadena de puerta que se está implementando conjuntamente

La excepción local debe coordinar:

1. Disponibilidad del actor: mismo método stock `0x02797A40`, aplicado por separado a Self y al Sub0 exacto.
2. Candidatos: únicamente actores devueltos por el sensor original, no NPCs arbitrarios ni una ampliación artificial del radio.
3. Propietario del ActionCommand de uObjModel: se comprueba DTI de uDoorGimmick; no se usa el layout `uItem+0xF48`.
4. Actor de WaitState y serial positivo transportado: deben concordar con el candidato elegido.
5. Cinco búsquedas específicas de esta familia: stock tiene prioridad; solo si no devuelve actor puede añadirse el Sub0 exacto cuyo serial coincida.
6. Selector final: Sub0 solo recibe Pad 1 si el modo local está activo y su ThinkMode sigue siendo Pad. Fuera del modo local devuelve el 0 original.

Se han ejecutado 49 comprobaciones nativas de la lógica C++ con servicios del motor simulados. No ejecutan el ABI Win32 ni el motor. La generación del PE y la verificación de los hooks continúan antes de publicar este candidato como artefacto de prueba.

## Límites

No se ha ejecutado el juego. El entorno contiene los EXE, pero no los assets compatibles de enero ni un entorno Windows del juego. La ejecución directa de un ELF de prueba i386 también fue rechazada por este entorno; no se presenta como prueba x86 nativa.

Quedan por validar el orden real de callbacks, el ciclo de vida de la selección, la concurrencia del motor y las transiciones completas. QTE, HUD, menús, muerte/reanimación, checkpoints y cámaras forzadas NO quedan certificados por estas comprobaciones.
