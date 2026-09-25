# Estado actual — HUD por instancia, continuación de solo fuentes

25 de septiembre de 2026. **No se ha generado un nuevo EXE del juego, instalador ni ZIP.** La entrega vigente es código, pruebas y evidencia en GitHub según AGENTS.md.

## Avance actual

Sobre la base consolidada `5118850441bf70ad8d6da2bd7d7c6505ab7bd234` se ha implementado `patches/hud_ownership/`: asociación explícita de cada widget a su clase, manager, actor vivo, serial/lifetime, miembro y vista; invalidación persistente al perder su vínculo; tokens generacionales para no confundir instancias reutilizadas; cálculo acotado de mDrawView preservando los demás bits.

Hay tres puentes i386 para las consultas Self de mira, equipo y mapa/hierbas. Conservan el frame, ECX y el argumento original del finder; en P2 inválido devuelven nulo en vez de adoptar P1. Son **fuentes no instaladas**. No se ha creado ni mostrado una segunda instancia dentro del juego.

La evidencia nueva está en [docs/24-hud-lifetime-and-view-filter.md](docs/24-hud-lifetime-and-view-filter.md) y [docs/25-hud-ownership-source-component.md](docs/25-hud-ownership-source-component.md). Contrato y precondiciones: [patches/hud_ownership/README.md](patches/hud_ownership/README.md).

## Hechos nuevos que no deben perderse

El cockpit reconstruye una lista fija de 21 slots y libera widgets por slots explícitos. No basta añadir clones a su enlace final. La mira obtiene Self en slot9, no en slot8.

Mapa/hierbas pertenece a uMiniMapManager y NO hereda uBioCockpitGUI. Su pointer +40 es alias del primer elemento del array +30; no es otro objeto a liberar. Sus campos +290/+294 no tienen el layout de next/ForceSkip del cockpit.

Ambos gestores usan el filtro nativo mDrawView, diez bits en cUnit+0C. Se ha demostrado por metadata, getter, setter y consumidores. Esto no prueba transformación, escalado ni clipping correctos de las dos mitades.

## Pruebas

El componente pasó nueve grupos C++ y UBSan; la mayoría de las aserciones cubren exhaustivamente las 1.024 máscaras de vista, no escenas de campaña. La auditoría de solo lectura pasó 94 comprobaciones del original y cinco tests locales. El original sigue intacto.

Los tres bridges se ejecutaron con el registro real en 16 escenarios i386 en GitHub Actions, usando un finder del motor simulado. Run `36176901629`, commit `0dc413931f21b192fb9f1fe37d5630e693824d53`: registry y bridges-i386 correctos. No se ejecutaron funciones del motor ni gameplay. La prueba privada de imagen no se sube a CI.

## Continuación exacta

Seguir la inicialización de recursos `0x02B402F0` (mira), `0x02B3B280` (equipo), `0x02B61A70` (mapa/hierbas) y el registro por grupos. Resolver identificadores compartidos y callbacks antes de crear/publicar las instancias de P2. Después conectar lifecycle y dispatch, aplicar/restaurar el filtro por vista y verificar las transformaciones de GUI. No reemplazar Self globalmente ni introducir mapa/hierbas en una lista que usa otro layout.

El adaptador que suministra Session/Actor todavía debe implementarse: valida vida real y serial, incrementa lifetime/epoch en transiciones y mantiene las llamadas en el hilo del juego. El registro no puede convertir por sí solo un puntero liberado en seguro.

## Trabajo anterior preservado

No se han cambiado los parches previos de input, re-enlace, pickup, puertas, ayuda ni guiones. El último hash de imagen construido anteriormente sigue siendo `71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4`; **esta continuación no genera otra imagen ni instala este módulo sobre ella**.

El estado anterior se conserva íntegro en [docs/history/CURRENT_STATUS-before-hud-ownership.md](docs/history/CURRENT_STATUS-before-hud-ownership.md). `research/current_state.json` conserva los datos de esa implementación previa; para el nuevo componente consultar `research/hud_ownership_state.json`.

El HUD/menú completo por jugador, scheduler compartido, muerte/checkpoints/cutscenes, escenas sin compañero y la validación conjunta en campaña siguen abiertos. No falta únicamente probar; quedan esas integraciones e implementaciones.
