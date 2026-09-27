# Genesis: contador independiente y fronteras de actor

27/09/2026. Continúa PR31. Componente fuente y ABI verificados; **todavía no
habilita Genesis para J2**. No se ha ejecutado el juego.

El contador global tiene 19 lecturas desde métodos del scanner, tres escrituras
y cuatro sumas. El resto de callers del getter/setter pertenece a otros sistemas
y se conserva. El auditor audit_genesis_progress.py fija 50 testigos al original
January: esos 26 sitios, prólogos que guardan el widget en EBP-8, operaciones
nativas y doce dependencias adicionales de actor.

El getter lee manager+74. Set (01BFA407 -> 02EB3910) limita unsigned a 100;
Add (01BB2404 -> 02EB3980) suma en 32 bits y después limita unsigned a 100.
Las dos mutaciones devuelven con ret4. Se conserva incluso el orden de wrap y
clamp del original; no se convierte el contador en un float ni en signed.

patches/genesis/progress mantiene el contador de Sub0 fuera del singleton.
Requiere que el adaptador certifique widget, hilo y owner vigente. Los widgets
stock conservan su función original; los retirados se rechazan. Conserva el
valor al recrear la GUI con los mismos actores y lo reinicia al cambiar epoch,
dirección, lifetime o serial. Rechaza reentrada y actores que no sean Sub0.
Los gateways conservan ECX y argumentos en la ruta stock, y respetan ret/ret4.

Este componente aún no se incluye en el instalador ni se activa en producción.
Faltan el clon completo, el adaptador propietario, sus productores de objetivos
y las rutas de actor/recompensa. No se añade una interfaz que aparente estar
funcionando mientras depende de datos compartidos.

## Dependencias adicionales encontradas

- Self: 02B214B8, 02B2203D, 02B2246B, 02B225BB, 02B27905, 02B27C3D.
- Actor por personaje seleccionado: 02B21003, 02B21244, 02B2178B, 02B218AB,
  02B224A5, 02B28C88. Las llamadas reciben un argumento y deben conservar su ABI.
- El update 02B1FCF0 usa un prólogo con alineación de pila; su this sigue en
  EBP-8 (almacenamiento 02B1FD22). No debe confundirse con la inicialización
  02B1F680 al buscar solo prólogos push ebp/mov ebp,esp.
- La notificación de 02823AA0 llama 01BB4079 -> 02B29F60, que busca la referencia
  en las colecciones del scanner. Su retirada necesita alcanzar ambos widgets;
  no se puede tratar como una notificación exclusiva del último mando utilizado.

## Verificación

4.331 comprobaciones del contador: valores 0..100, sumas normales y extremos,
aislamiento stock/hidden, identidad, recreación de GUI, reset y reentrada.
ABI x86: nueve rutas (read/set/add por stock/local/hidden), argumentos completos,
pila, registros preservados y tres llamadas stock exactas. Solo se ejecutó
código sintético. El auditor privado del original pasó 50 comprobaciones.

La copia acumulativa del juego sigue siendo la de PR31, con 52 hooks.
El cooperativo completo continúa en desarrollo y no está validado jugando.
