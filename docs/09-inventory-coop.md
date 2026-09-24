# 09 — HUD e inventario cooperativo

Build de referencia: **BioRevHD 30-Enero-2013.exe**.

Este documento continúa la única línea de investigación actual, cuya base experimental vigente es **v9 CLEAN PERSISTENT SPLIT**.

## 1. Pista descartada: uGUI_SubEquipWin no es el HUD de Sub0

Se localizaron RTTI/clases:

```text
uGUI_MainEquipWin
uGUI_SubEquipWin
```

A primera vista podían parecer ventanas separadas para Main Player y Sub Player.

Al seguir sus recursos y campos, la interpretación correcta es otra:

```text
uGUI_MainEquipWin -> equip_wp_main
uGUI_SubEquipWin  -> equip_wp_sub
```

`uGUI_SubEquipWin` contiene además lógica como:

```text
mSubWeaponNum
6 slots
```

Por tanto **Sub = sub-weapon**, no “Sub Player”.

**DESCARTADO:** no se usará como base para un segundo HUD de jugador.

Esta pista se conserva expresamente porque durante el análisis parecía prometedora.

---

# 2. sItemBoxCoop sí es una clase cooperativa real

RTTI identificados:

```text
app::game::item_box::sItemBox
app::game::item_box::sItemBoxCoop
app::game::item_box::sItemBox::cBag
app::game::item_box::sItemBoxCoop::cBagCoop
```

Type descriptor / vtable:

```text
sItemBox
  TD     0x054D8FAC
  vtable 0x04E02DAC

sItemBoxCoop
  TD     0x054D95A0
  vtable 0x04E03EA4

cBag
  TD     0x054D9194
  vtable 0x04E03480

cBagCoop
  TD     0x054D95D8
  vtable 0x04E03F34
```

RTTI confirma:

```text
sItemBoxCoop : sItemBox
cBagCoop     : sItemBox::cBag
```

No son simples nombres debug.

## Tamaños

```text
sItemBox     0x188
sItemBoxCoop 0x188

cBag         0xB8
cBagCoop     0xC0
```

`cBagCoop` añade exactamente 8 bytes respecto a `cBag`.

---

# 3. Campos extra de cBagCoop

Se confirmaron dos DWORD adicionales:

```text
cBagCoop +0xB8
cBagCoop +0xBC
```

Setters:

```text
0x02D0CBA0 -> +0xB8
0x02D0CBF0 -> +0xBC
```

Getters:

```text
0x02D0CDC0 -> +0xB8
0x02D0CE00 -> +0xBC
```

La inicialización cooperativa `0x02D0CA70` obtiene dos valores mediante la misma estructura, con índices 0 y 1, y los almacena en esos campos.

Flujo:

```text
get game-system object
 -> 0x01C74234 -> subobject +0xDC0
 -> 0x01C5CA9E(index)
 -> real impl 0x0278C7B0
```

`0x0278C7B0` implementa literalmente:

```text
return this->array[index]
```

con layout observado:

```text
[this + 0x04 + index*4]
```

Así:

```text
index 0 -> cBagCoop+0xB8
index 1 -> cBagCoop+0xBC
```

## Importante

Todavía **no** se etiqueta esos DWORD como playerID hasta identificar semánticamente el owner de ese array.

La evidencia actual solo confirma que son dos valores asociados a los dos índices de participante.

---

# 4. Validación posterior de esos dos valores

La función `0x02D0CC40` vuelve a obtener la misma estructura y comprueba:

```text
cBagCoop+0xB8 == array[0]
cBagCoop+0xBC == array[1]
```

Además compara los bags cooperativos correspondientes a:

```text
0x80010000
0x80011000
```

Esto demuestra que los dos valores extra no son temporales: forman parte de la identidad/validez del bag cooperativo.

---

# 5. Bags y claves cooperativas

`sItemBoxCoop` crea/consulta bags con claves constantes observadas:

```text
0x80010000
0x80011000
0x80020000
```

La inicialización alrededor de `0x02D0C410` reserva arrays de `cBagCoop`:

```text
16 * 0xC0 bytes
```

para varias categorías.

El significado exacto de las tres claves está pendiente; no se renombrarán todavía como P1/P2/shared sin evidencia adicional.

---

# 6. Factoría real de sItemBoxCoop

`sItemBoxCoop` es instanciable en runtime.

Constructor aproximado:

```text
0x02D0C160
```

- ejecuta inicialización/base;
- instala vtable `0x04E03EA4`.

Factoría:

```text
0x02D0BD20
```

- reserva `0x188`;
- llama al constructor de `sItemBoxCoop`.

Por tanto el subsistema cooperativo de inventario existe como implementación completa.

---

# 7. Diferencias virtuales sItemBox vs sItemBoxCoop

Ambas clases tienen 29 entradas virtuales analizadas.

Slots distintos:

```text
0, 4, 6,
10, 11,
14, 15, 16, 17, 18, 19, 20, 21, 22,
25, 28
```

Implementaciones destacadas:

```text
slot 20
base  0x02CEC8B0
coop  0x02D0CA70   <- inicializa los dos valores indexados

slot 21
base  0x02CEC8F0
coop  0x02D0CC40   <- valida los dos valores

slot 25
base  0x02CF8610
coop  0x02D0C410   <- construcción de bags coop
```

La cantidad de overrides demuestra que el comportamiento cooperativo no es una simple bandera sobre `sItemBox`.

---

# 8. Estado actual

**CONFIRMADO:**

- existe un sistema específico `sItemBoxCoop`;
- contiene bags derivados `cBagCoop`;
- cada `cBagCoop` guarda dos valores persistentes asociados a índices 0/1;
- esos valores se vuelven a validar contra la fuente global;
- la clase dispone de factoría/constructor real.

**PENDIENTE:**

1. identificar el tipo exacto del subobjeto devuelto por `0x01C74234`;
2. poner nombre semántico a `array[0]` y `array[1]`;
3. demostrar dónde se selecciona `sItemBoxCoop` frente a `sItemBox`;
4. comprobar si la campaña en v9 ya instancia la clase cooperativa;
5. determinar cómo Main/Sub0 eligen su bag;
6. después estudiar pausa/inventario interactivo/HUD de cada jugador.

No se crea una nueva versión de EXE hasta obtener un cambio de código concreto y demostrado.


---

## CORRECCIÓN: +0xB8/+0xBC son cCoopSkill, no IDs de jugador

La interpretación provisional de `cBagCoop+0xB8/+0xBC` como posibles identidades de participantes queda descartada.

El owner de la estructura consultada a través de `uPlayer+0xDC0` fue identificado por RTTI como:

```text
app::game::network::sCoopManager::cCoopSkill
```

Vtable `0x04E0F7CC`; type descriptor `0x054DF2FC`.

Sus métodos alrededor de `0x02D90160..0x02D905B0` inicializan, actualizan, serializan y limpian dos valores indexados en `this+0x04/+0x08`.

Por tanto los dos DWORD copiados a `cBagCoop+0xB8/+0xBC` son estado/skill cooperativo, no `playerID`.

## playerID real de sItemBox

La ruta `sItemBox::saveData()` obtiene el primer valor de su mensaje de error `playerID` desde el objeto jugador mediante:

```text
0x01C53859 -> 0x027F1200
```

`0x027F1200` comprueba un override en `uPlayer+0xE38`; si es distinto del sentinel `0x80000000`, lo devuelve. Si no, deriva el ID desde el objeto asociado.

## Mapeo confirmado de pMyBag y pSharedBag

Una rutina alrededor de `0x02CEFF10` confirma por sus asserts:

```text
[ebp-0xB4] = pMyBag
[ebp-0xC0] = pSharedBag
```

El primero viene del virtual `+0x48` (slot 18), y el segundo del virtual `+0x2C` (slot 11).

### sItemBoxCoop::pSharedBag

```text
slot 11 / +0x2C
base: 0x01C3F51B -> 0x02CF42C0
coop: 0x01C74284 -> 0x02D0C750
```

`0x02D0C750` hace el lookup con `0x80010000`.

**CONFIRMADO:** `sItemBoxCoop::pSharedBag -> 0x80010000`.

### sItemBoxCoop::pMyBag

```text
slot 18 / +0x48
base: 0x01C94AED -> 0x02CF4710
coop: 0x01C744C3 -> 0x02D0C930
```

`0x02D0C930` ignora el `playerID/sceneID` recibido y llama a su lookup virtual con `0x80011000, 0`.

**CONFIRMADO:** `sItemBoxCoop::pMyBag -> 0x80011000`.

La tercera clave `0x80020000` sigue pendiente de nombre hasta seguir sus callers.

## Consecuencia

Cuando se usa `sItemBoxCoop`, el motor ya redirige explícitamente las bolsas normalizadas a claves cooperativas. La siguiente pregunta es cuándo se instancia/selecciona `sItemBoxCoop`, no cómo inventar una bolsa nueva para P2.
