# Progreso publicado

Rama canónica:

```text
research/script-member-serial-validation
```

Handoff actual: **PR #20**, merge `6f19b0c9ccb4fcdabd8d35ada6af69d0bebdc284`.

Bloque funcional anterior: **PR #19**, MenuOwnerRouter:
- J1/J2 identificados al abrir Pause/SubMenu;
- P1 conserva prioridad;
- SubMenu owner J2 usa Pad1;
- inventario SubMenu owner J2 usa Sub0/cBioItemPack propio;
- Self y mStartPadNo globales no se modifican.

Punto exacto pendiente:
1. navegación/opciones/confirmar/cancelar de PauseHD para owner J2;
2. cierre/lifetime del owner al salir de states 5/8;
3. auditar states 6/7 y callers alternativos;
4. Genesis/QTE, muerte/checkpoints, cutscenes y escenas sin partner;
5. instalación y validación completa en campaña.

Documentación canónica:
- [CURRENT_STATUS](https://github.com/Putxo/BioHazard-R-Local/blob/research/script-member-serial-validation/CURRENT_STATUS.md)
- [START_HERE_NEW_CHAT](https://github.com/Putxo/BioHazard-R-Local/blob/research/script-member-serial-validation/START_HERE_NEW_CHAT.md)
- [CONTINUE_IN_NEW_CHAT](https://github.com/Putxo/BioHazard-R-Local/blob/research/script-member-serial-validation/CONTINUE_IN_NEW_CHAT.md)

No generar EXE salvo petición expresa.
