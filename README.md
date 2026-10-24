# MengerInfinite

Esponja de Menger con profundidad **infinita real** en Unreal Engine 5.6 (C++). Se recorre con
una nave cuya velocidad se regula con la rueda del ratón. Es una prueba de rendering y de
precisión numérica, no un juego completo.

## Antecedente

Proyecto OpenGL previo: `C:\Users\Rodrigo\CARRERA\ASIGNATURAS\PRGR-ProgGrafica\codigo\OpenGL\mengersponge`
(raymarching en el fragment shader, pliegues `abs + sort`, *pre-folds* y repetición de dominio;
ver `FRACTALS.md` y `src/main.cpp`).

Allí el detalle se degradaba hacia el nivel 15. Hipótesis, pendiente de confirmar: precisión
de `float32`. `3⁻¹⁵ ≈ 7·10⁻⁸` queda por debajo de la épsilon relativa de `float` en torno a 1
(`1,19·10⁻⁷`), y el umbral de impacto del shader era `5e-8`. Pasado ese nivel, la posición de la
cámara y las del rayo no se pueden representar a la escala del detalle. Añadir iteraciones o
usar `double` solo retrasa el límite (`double` llega hacia el nivel 33). Para que sea infinito
hay que cambiar de enfoque.

## Idea central: renormalización por autosimilitud

La cámara nunca se guarda en coordenadas absolutas. Su estado es:

- `Path`: pila de dígitos (índice del subcubo, de 0 a 26, en cada nivel) que lleva desde la
  esponja raíz hasta la celda actual de nivel `k`;
- `LocalPos` (`double`): posición dentro de esa celda, normalizada a `[-1, 1]³`;
- la velocidad y la orientación en el marco local.

Operaciones (funciones puras con test):

- **Bajar un nivel** (zoom hacia dentro): se elige el subcubo que contiene `LocalPos`, se apila
  su índice y `LocalPos = 3·LocalPos − centro(subcubo)`. La velocidad se multiplica por 3.
- **Subir un nivel**: `LocalPos = (LocalPos + centro(dígito)) / 3`, se desapila y la velocidad
  se divide por 3.
- **Salir de la celda lateralmente**: acarreo en base 3, como un cuentakilómetros, hasta el
  ancestro común y bajando otra vez a la celda vecina.

Para que la escena vista desde el marco local tenga el contexto correcto (los vecinos de un
subcubo no son los de la esponja raíz), el shader recibe los últimos `M` dígitos de `Path`.
Evalúa el DE en el marco del ancestro `k − M`. Con `M ≈ 6` (`3⁶ = 729`), el error de `float`
en el marco local es de unos `10⁻⁴` y todo lo que está a más de `3^M` unidades locales queda
fuera de `MAX_DIST` (con niebla). El coste no depende de `k`, así que `k` puede crecer sin límite.

Caso límite: la cámara cerca del borde de la celda ancestro. La esponja restringida a cualquier
celda de nivel `k − M` es o vacía o una copia exacta escalada de la esponja entera. Por eso el
DE exacto en un bloque `3×3×3` alrededor del ancestro es el mínimo de los DE de sus celdas
sólidas. La CPU calcula esa máscara de 27 bits de ocupación a partir de `Path` (con acarreo) y
se la pasa al shader. Solo se evalúan las celdas que caen a menos de `MAX_DIST`.

La escala de vista (cuántos niveles se baja) la marca el usuario con la rueda o la velocidad,
no la posición. Se baja solo cuando el detalle visible de la celda actual es menor que un
umbral en píxeles.

## Rendering en UE

- **Fase 1**: material de post-proceso con un nodo *Custom* HLSL que recibe cámara local,
  orientación, FOV y dígitos por `Material Parameter Collection` o parámetros. Portar el DE
  y la iluminación del shader OpenGL (normales por tetraedro, AO por número de pasos, niebla).
- La nave se renderiza con normalidad. El post-proceso compara `SceneDepth` con la distancia
  `t` del rayo y deja la nave delante cuando está más cerca. La cámara de UE renderiza la nave
  en un marco fijo cerca del origen; el fractal se dibuja desde `LocalPos`.
- Elegir la ubicación del blendable y comprobar su interacción con TSR y la exposición. Si el
  post-proceso se queda corto, pasar a un *global shader* con `FSceneViewExtension` en C++.

## Nave y control

- `AMengerShipPawn` con movimiento propio (sin física de motor): el ratón orienta, WASD
  traslada, Q/E alabeo y Shift turbo.
- **Rueda del ratón**: multiplica o divide la velocidad (pasos de `×1,25`). Velocidad efectiva
  proporcional a la escala del nivel actual. Opcional: modo automático con velocidad
  proporcional al DE en la posición de la nave, para frenar cerca de las paredes.
- Cámara en tercera persona que sigue a la nave.
- Colisión: el mismo DE portado a C++ (`double`) evalúa la nave en CPU; empuja o desliza al
  tocar la superficie. Un test compara el DE de CPU con valores de referencia del HLSL.

## Fases y criterios de aceptación

| Fase | Entregable | Aceptación |
|---|---|---|
| 1 | Esponja por raymarching en post-proceso, cámara libre | 10 niveles a ≥ 60 fps a 1080p (`stat unit`); imagen equivalente a la versión OpenGL |
| 2 | Renormalización (`Path` + `LocalPos`) | Tests de ida y vuelta bajar/subir y de acarreo lateral; descenso a nivel 50 sin degradación visible (captura en nivel 5 frente a nivel 50) |
| 3 | Nave, rueda y colisión por DE | La velocidad cambia con la rueda; la nave no atraviesa paredes en 100 choques seguidos (test con posiciones fijas) |
| 4 | Pulido | Color por profundidad o nivel, HUD con nivel `k` y velocidad, lámina final de capturas |

## Fuera de alcance

Multijugador, generación de malla real del fractal (todo es raymarching), Lumen sobre el fractal.
Si sobra tiempo, añadir otros fractales de `FRACTALS.md` reutilizando el mismo sistema.

## Compilar y probar

```powershell
powershell -File Tools/build.ps1                  # editor (Development)
powershell -File Tools/build.ps1 -Target Game     # juego
powershell -File Tools/test.ps1                   # Automation Tests sin interfaz
```

Con el editor abierto, la compilación por línea de comandos falla por Live Coding: cerrarlo antes.
La primera vez, clic derecho en `MengerInfinite.uproject` → *Generate Visual Studio project files*.
