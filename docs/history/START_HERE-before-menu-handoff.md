# Continuar sin perder el estado real

Leer completo [CURRENT_STATUS.md](CURRENT_STATUS.md), después [docs/20-local-routing-built-and-tested.md](docs/20-local-routing-built-and-tested.md) y [research/current_state.json](research/current_state.json).

El candidato más reciente es **LOCAL ROUTING EXPERIMENTAL**, SHA-256 `0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378`. Hay código construido y pruebas de componentes, pero no cooperativo completo certificado jugando.

La base inmediata es el owner-fix `e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a`. No empezar de nuevo por v13/v14 ni por `0x049A8023`.

El handoff largo anterior se ha conservado sin cambiar sus bytes en [docs/history/START_HERE-before-local-routing.md](docs/history/START_HERE-before-local-routing.md). Es histórico: contiene afirmaciones de alcance y un selector de pickup rectificados en documentos posteriores.

No subir binarios propietarios, no pisar trabajo paralelo y no marcar runtime como validado a partir de tests con mocks.
