# 49 — Revocar owner ante lecturas inválidas

27 de septiembre de 2026. PR #23, rama `research/pause-owner-lifetime-audit`.
Base integrada: PR21/PR22, `cf472903e0b7b086e8d1260f0d07a26f3d002922`.

## Resultado

Se preservan el routing PauseHD Pad1, la identidad Sub0 puntero+serial y el
retorno nested 5/8 -> 6/7 -> raíz implementados por los avances remotos.
No se añade un segundo hook de estado. Las correcciones nuevas son:

1. Rechazar Sub0 cuando `sub+0xE40+3` excede PE32. Antes, un puntero alto
   podía envolver a memoria baja y admitir un actor ficticio.
2. Rechazar sPad cuando `pad+0x970+3` excede PE32, antes de consultar el
   selector. Con `pad=0xFFFFF800`, el selector envuelve pero el bitfield
   aún puede ser legible; el código anterior aceptaba ese dato inconsistente.
3. Revocar owner1 cuando una lectura de su pad falla. Recuperar la memoria
   no reactiva ownership sin una apertura nueva.
4. Validar owner1 incluso cuando se consulta otra superficie. Antes,
   invalidar Sub0 durante una lectura de otra superficie, restaurarlo y
   volver a su menú permitía conservar el owner que debía haberse descartado.

Se conserva el fallback stock del repositorio, incluido mStartPadNo, sin
modificar globals del juego.

## Pruebas

- Suite heredada Win32 MSVC `/W4 /WX`: 16 escenarios / 59 aserciones PASS.
- Suite nueva Win32 MSVC `/W4 /WX`: 18 escenarios / 89 aserciones PASS.
- Auditor complementario `scripts/audit_pause_owner.py`: 19 comprobaciones
  exactas de las lecturas PauseHD y escritura común de estado. SHA de enero
  `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`.
- El test ABI comprueba ahora el balance de pila antes de que el harness
  restaure ESP, y que EAX/EDX entran intactos en el body de transición.
  Antes `mov esp,ebp` podía ocultar un `ret N` equivocado.
- CI ejecuta ambas suites con GCC nativo y ASan/UBSan, sintaxis i386,
  auditoría de mapa y los ocho gateways sobre motor simulado.

La evidencia no equivale a gameplay. Los cuatro originales se leyeron;
solo el de enero corresponde al mapa de direcciones y al hash admitido.
Ver `research/reports/local-build-inventory-2026-09-27.json`.

## Límite pendiente

Conservar ownership en estados 6/7 no demuestra que todos sus widgets y
opciones ya reciban Pad1. Falta la instalación conjunta y la validación
dentro del juego. Un ciclo de sesión que reutilice puntero+serial y no
pase por una transición observada tampoco queda demostrado por mocks.
Continuar por esas rutas o por Genesis según el checkpoint canónico más
reciente; no volver a sustituir los bloques ya fusionados.
