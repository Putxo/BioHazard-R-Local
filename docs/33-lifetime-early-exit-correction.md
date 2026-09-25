# 33 — Corrección del aviso de inicio PCS ante la salida anticipada

Continúa el trabajo recuperado de `research/hud-lifetime-observer` en d18aa28011118d01e6117eb5fd223218cd1b8b37. Solo fuentes; no se genera ni modifica un EXE.

## Defecto reproducido por análisis del original

El punto BEGIN anterior, 0x02DF4F99, no precedía a todos los caminos que llegaban a END 0x02DF504B. El original contiene:

```asm
02DF4F65 mov eax,[ebp-8]
02DF4F68 movzx ecx,byte ptr [eax+50h]
02DF4F6C test ecx,ecx
02DF4F6E jne 02DF5043
...
02DF5043 mov ecx,[ebp-8]
02DF5046 call 01C65419
02DF504B ; aviso END propuesto
```

Cuando el byte +50 no es cero, el aviso final se ejecutaría sin el inicial. LifetimeSource lo trataría correctamente como pareja inválida, pero el hook estaría provocando ese error sobre un camino nativo legítimo. La CI previa verificaba los gateways aislados, no esta propiedad del flujo del caller.

## Corrección

BEGIN pasa a **0x02DF4F65**, antes del test. Desplaza siete bytes `8b45f80fb64850`; reproduce MOV y MOVZX y continúa en **0x02DF4F6C**. El antiguo punto 0x02DF4F99 deja de admitirse. END y el resto de eventos no cambian.

Se mantienen flags, registros no modificados por las instrucciones desplazadas, pila y estado FP. EAX/ECX toman los resultados originales de MOV/MOVZX; no se restaura sobre ellos un valor incorrecto anterior al replay.

Los once puntos propuestos conservan sus bytes en el EXE histórico 71f5e70d... que ya estaba adjunto; se leyó sin modificarlo. BEGIN/END no se superponen con los hooks previos del reenlace local en 0x02DF4FA7 y 0x02DF5015. No se compila ni construye otra imagen.

## Regresiones añadidas

El harness i386 usa un objeto sintético real para comprobar el byte +50 y su extensión a cero con bytes vecinos no nulos. Ejecuta los once gateways en doce invocaciones: BEGIN se prueba con cero y no cero. La suite de LifetimeSource añade rechazo del sitio antiguo y pares de eventos de los caminos normal/salida anticipada, sin interpretar el reenlace como otra vida del actor.

Estas pruebas siguen siendo de componentes. La instalación de todos los hooks antes de los nacimientos observados, el hilo/lifetime real, el reloj de presentación y los scopes/fence de render siguen pendientes. No habilitar el observador a mitad de una escena fingiendo que los actores existentes han sido construidos después de armarlo.
