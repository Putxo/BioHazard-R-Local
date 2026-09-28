# 87 — Materiales Hunter por vista para Génesis local

28/09/2026. Continúa el recorrido de PR #58. Análisis y parche fuente sobre el ejecutable January exacto `9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69`. El juego no se ejecuta.

## Por qué no se duplica 021FD970

`021FD970` no es una función pura de dibujo. Lee el alfa base de `uHunter+1A74`, consulta el singleton de `sHunterManager` y su byte `+65`, avanza osciladores y escribe estado persistente del actor. Ejecutarla una segunda vez para View1 alteraría la simulación.

La condición nativa que cambia el aspecto de Génesis es:

- debe existir `sHunterManager[0]` (`0556DD74`);
- `sHunterManager+65` debe estar activo;
- el alfa base debe ser exactamente 1.0;
- entonces el alfa de material pasa a 0.99.

A partir del alfa efectivo, el juego calcula:

```text
inv = 1 - alpha
A = clamp(1 - inv*0.75, 0.25, 1.0)
B = clamp(inv, 0.0, 1.0)
C = clamp(inv*0.8, 0.0, 0.8)
```

## Parámetros realmente dependientes de Génesis

Para cada parte de modelo, `021FD970` resuelve:

- material `6C8011F4`
  - floats 44..46 <- A
  - floats 0..2 <- B
  - floats 40..42 <- C
- material `EFCA322B`
  - float 1 <- alpha

Otros parámetros escritos en la misma función proceden de osciladores distintos y no se sustituyen.

El lookup nativo de parámetros usa grupo 1:

- count: tabla en `part+88`; grupo 1 -> `part+8C`;
- índice ordenado: tabla en `part+78`; grupo 1 -> `part+7C`;
- base de entradas: `part+30`;
- cada entrada mide 12 bytes;
- clave: bits 20..31, comparados con `hash & 0xFFF`;
- `entry+4` contiene el puntero de buffer con flags bajos; el buffer es `& ~0xF`.

Las partes del Hunter se enumeran desde `uModel+F8` y su count `+FC`.

## Transacción por View1

`HunterMaterials` no modifica el estado persistente del Hunter. Antes del primer bind de material del dibujo View1:

1. valida Hunter, contexto y propietario/frame local;
2. obtiene todos los buffers afectados;
3. elimina duplicados por dirección;
4. guarda los nueve floats RGB y el alpha originales;
5. calcula los valores desde `Hunter+1A74` y el estado privado del Scanner J2;
6. escribe solo esos parámetros.

La restauración ocurre **después** del unbind final del Hunter.

Fronteras nativas:

```text
034FA2B4  unbind por cambio de material     -> stock, no restaura
034FA2D2  bind de material                  -> begin View1 + bind stock
034FA464  unbind final                      -> unbind stock + restore
```

El bind `03700980` conserva el material en el contexto de dibujo, por lo que restaurar inmediatamente después del bind sería demasiado pronto.

View0 y cualquier contexto que no sea el View1 local certificado conservan el camino stock.

## Seguridad

La transacción es fail-closed:

- no se ensancha ninguna máscara de vista;
- no se escribe `sHunterManager+65`;
- no se tocan `Hunter+1AE0/1AF0/1B00/1B24`;
- no se modifican osciladores, caches ni recursos;
- cambio de sesión/frame/owner durante el dibujo provoca rollback y cuarentena;
- fallo parcial de escritura intenta restauración y se considera Fault.

El instalador acumulativo añade dos hooks, de 182 a **184**.

## Lo que sigue pendiente

Después de este bloque todavía no se habilita Génesis desde el mando 2.

`021FD970` utiliza también el alfa para transiciones persistentes de estado/visibilidad/animación alrededor de `021FE7F2–021FE971` y `Hunter+1B24`. Esas escrituras no pueden duplicarse por vista sin demostrar primero qué parte es render-only y qué parte pertenece a gameplay.

Siguiente bloque exacto:

1. auditar cada consumidor de las flags/estado afectados por ese tramo;
2. separar únicamente la decisión visual que sea realmente por vista;
3. conservar una sola actualización de simulación por Hunter/frame;
4. después revisar los últimos consumidores compartidos de Génesis;
5. solo entonces habilitar la entrada de Génesis para J2.

## Evidencia

`scripts/audit_genesis_hunter_materials.py` fija 27 testigos del binario January: condición 0.99, fórmulas, hashes, setters, estructura de partes, lookup de parámetros y fronteras bind/unbind.

Los tests de componente cubren aplicación, estado activo/inactivo, manager ausente, buffers compartidos, rollback, objetos inválidos y API incompleta. El gateway tiene ABI i386 sintética separada.

No se ha ejecutado gameplay.
