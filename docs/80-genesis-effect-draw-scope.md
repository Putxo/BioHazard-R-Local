# Contexto de dibujo de los auxiliares del Scanner

27/09/2026. Continúa PR51. Genesis todavía no se activa; juego no ejecutado.

Los tres métodos de dibujo reciben ahora un gateway thiscall que distingue
la raíz privada por dirección, tipo y generación observada. Para J2 valida
el grafo y la sesión, y abre un NativeViewScope independiente del de Cockpit.
Exige cDraw exacto, View1, cámara secundaria activa y rectángulo idéntico al
de esa cámara. No llama al método nativo si falla esta admisión.

Tras el dibujo vuelve a comprobar contexto, frame, actor y vida del efecto.
Un cambio durante la llamada revoca el conjunto de HUD y retiene el Scanner
en cuarentena. La reentrada de la misma raíz no dibuja dos veces. Los efectos
originales, hijos del recurso FilterSet y nuevas generaciones ajenas conservan
su recorrido nativo. Los dibujos locales admitidos usan la ruta inmediata de
sUnit; estos gateways no habilitan acceso al runtime desde otros hilos.

Entradas: FilterSet `02933B20`, TVNoise `02935660`, Outline `037DE560`.
Las dos primeras reponen prologos de nueve bytes. Outline repone seis bytes
antes de continuar su frame alineado con EBX. El test ABI comprueba los tres
casos admitidos/rechazados y la restauración de pila y registros conservados.
El instalador acumulativo contiene **134 enganches**.

La lectura estática confirma que TVNoise obtiene el rectángulo activo de cDraw
para normalizar coordenadas de textura con corrección de medio píxel. Outline
usa la altura de ese rectángulo para calcular el grosor. Se conserva esa lógica;
no se añade una segunda reducción manual del tamaño. Esto no demuestra por sí
solo el comportamiento de todos los shaders ni de sus buffers temporales.

Validación: 175 escenarios Runtime / 42.497 comprobaciones; contexto incorrecto,
rectángulo distinto, cámara desactivada, frame cerrado, muerte de actor o efecto,
reentrada y cambios durante el callback. ABI sintética x86 PASS; 99 testigos
estáticos del original SHA-pinned; cuatro pruebas del instalador y reversión
exacta de la copia independiente. No instalada ni ejecutada.

Módulo: `f355606beccc70b6f0ac5aaf235a404b4cc651b45696147d1110e0f7343baf18`.
Copia: `7fb1965f8bdc23c1a0cf2897958b7b8a31e58d6a70f691a9c53891c5d0d78071`.

Pendiente: globals de activación, parámetros de materiales por vista, recursos
internos y carga asíncrona. Véase el hallazgo de los gestores compartidos en
[81-genesis-activation-globals.md](81-genesis-activation-globals.md).
