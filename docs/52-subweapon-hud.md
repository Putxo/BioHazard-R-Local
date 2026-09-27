# Panel de subarmas por jugador

27/09/2026. Base integrada: PR25, `6a7144f513f93b9128af7606c45a016973cf833d`.

El panel `uGUI_SubEquipWin` del original consulta Self incluso cuando se dibuja
en View1. Se añade una cuarta instancia con recursos mutables propios y actor
Sub0, conservando el original de J1. Main/SubEquip identifican clases de armas,
no el número de jugador.

## Implementación

- Registro, lifecycle, backend, ledger, coordinador y máscara admiten cuatro
  widgets. Los harness antiguos conservan el perfil de tres; el entry nativo
  selecciona explícitamente cuatro. Se rechazan cantidades no admitidas.
- Cockpit `+0x64` proporciona el original prestado. Se comprueban vtable,
  recursos, separación de árboles y ausencia de alias antes de publicar.
- Clase `0x04DE57B4`, tamaño `0x370`, allocator `0x01B8BCE6`, constructor
  `0x01C39C88`; fases y destructor se contrastan con la vtable antes de llamar.
- En `0x02B42FA6`, el bridge consulta el registro usando el widget de `[EBP-8]`.
  J1 continúa por el finder original; J2 recibe Sub0. El estado revocado devuelve
  cero, que esta función comprueba antes de acceder al pack. Se conserva ret 4.
- La máscara temporal quita solamente View1 del original de subarmas durante
  su dibujo nativo y restaura los bits previos. Se corrige también el rollback
  de los paneles ya modificados si falla una lectura del siguiente panel.
- El instalador ahora enlaza y verifica 36 enganches.

## Evidencia

`python scripts/audit_subweapon_hud.py ORIGINAL.exe` comprueba 24 testigos y la
herencia de Cockpit en el original January fijado por SHA. Auditor local PASS.
Integración sintética Win32: 15 escenarios, 1124 assertions. Máscaras: 13/46.
El test i386 incluye los cuatro bridges, registros, predicado y equilibrio de
pila. Las pruebas de ELF/instalador pasan; se construyó una copia de trabajo y
su reversión exacta al input fue verificada sin ejecutarla.

Módulo LLVM 22.1.8: `0ae5aa1455e989c34d3b17d62403f1e5780fc216dc489ee407ddf110e697c5d7`.
Copia local: `f80cfe9ee2141d7ca689c8d6d90fde0ba2ec313b09ccbaa7d865425591371651`.
No se suben ejecutables ni datos del juego.

## Pendientes

Esto amplía el panel de subarmas; no acredita todavía todos sus efectos, blur,
animaciones, selección ni arbitraje. El draw mantiene comprobaciones globales
del original que necesitan auditoría adicional. Scope, ActionIcon, Damage,
Heal y GadgetScanner están localizados pero no incorporados: algunos reciben
eventos, por lo que duplicarlos sin enrutar sus productores sería insuficiente.
HP, prompt Y, Genesis y las restantes áreas descritas en CURRENT_STATUS siguen
pendientes. No se ha abierto el juego y no se declara cooperativo completo.

