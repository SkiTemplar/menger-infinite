# MengerInfinite

Esponja de Menger con profundidad **infinita real** en Unreal Engine 5.6 (C++). Se recorre con
una nave cuya velocidad se regula con la rueda del ratón. Es una prueba de rendering y de
precisión numérica, no un juego completo.

## Antecedente


En una primera versión con raymarching en un fragment shader, el detalle se degradaba hacia el nivel 15. Hipótesis, pendiente de confirmar: precisión
de `float32`. `3⁻¹⁵ ≈ 7·10⁻⁸` queda por debajo de la épsilon relativa de `float` en torno a 1
(`1,19·10⁻⁷`), y el umbral de impacto del shader era `5e-8`. Pasado ese nivel, la posición de la
cámara y las del rayo no se pueden representar a la escala del detalle. Añadir iteraciones o
usar `double` solo retrasa el límite (`double` llega hacia el nivel 33). Para que sea infinito
hay que cambiar de enfoque.

## Idea central: renormalización por autosimilitud (implementada en la fase 2)

La cámara nunca se guarda en coordenadas absolutas. Su estado (`Menger::FFractalPosition`,
`Source/MengerInfinite/Public/MengerPath.h`) es:

- `RootCell`: celda entera en la red de lado 2 (la esponja raíz es la celda `(0, 0, 0)`; con la
  repetición de periodo 4 del proyecto OpenGL hay esponja en las celdas de índices pares);
- `Digits`: un dígito por nivel, con componentes en `{-1, 0, 1}` (el subcubo dentro del padre);
- `Local` (`double`): posición dentro de la celda del último nivel, en `[-1, 1]³`.

Operaciones (funciones puras con test):

- **Bajar un nivel**: se apila el subcubo que contiene `Local` y `Local = 3·Local − 2·dígito`.
- **Subir un nivel**: `Local = (Local + 2·dígito) / 3` y se desapila.
- **Salir de la celda lateralmente**: acarreo en base 3 equilibrada, como un cuentakilómetros,
  hasta la celda raíz si hace falta.

**Escala de vista** (`Menger::FCameraState`, `MengerNavigator.h`): se guarda en log₃
(`ViewScale`; una unidad de vista mide `3^-ViewScale` unidades raíz), porque a nivel 1000
`3⁻¹⁰⁰⁰` no cabe en un `double`. La rueda fija `UserScale`, la escala más fina a la que se
puede bajar. La escala efectiva es `min(UserScale, escala de la geometría)`: si la pared más
cercana está a más de 30 unidades de vista, la vista crece. El DE de la cámara se calcula en CPU
en `double`. El nivel de la cámara es `floor(ViewScale)`. La velocidad va en unidades de vista
por segundo, así que la rueda es a la vez zoom y velocidad.

**Marco de render** (`Menger::MakeRenderFrame`): el shader evalúa el DE en el marco del ancestro
`k − 6`. Con `3⁶ = 729`, el error de `float` queda en unos `10⁻⁴` unidades de vista a cualquier
profundidad. La esponja restringida a una celda es o vacía o una copia exacta de la esponja
entera. Por eso, en un bloque `3×3×3` alrededor del ancestro, basta una máscara de ocupación de
27 bits, que calcula la CPU a partir de los dígitos (con acarreo). El shader solo evalúa las 8
celdas del octante más cercano al punto, con poda por caja: la geometría del resto está a 1 unidad
de marco o más. Si `k ≤ 6`, el marco es el raíz con la esponja repetida. El rayo no pasa de
`0,95` unidades de marco (≥ 692 unidades de vista), muy por encima de la niebla.

**Niebla, sombra y AO en unidades de vista**: la imagen solo depende de la geometría a la escala
de vista, así que es autosimilar. La pose de pared se construye por dígitos (exacta a cualquier
nivel) y da la misma captura, píxel a píxel, en los niveles 10, 50 y 1000 (`Tools/bench.ps1` lo
comprueba). Contrapartida: lo que queda a miles de unidades de vista (la pared opuesta de un
túnel grande cuando se está pegado a una pequeña) se funde en la niebla.

## Rendering en UE

- **Global shader + `FSceneViewExtension`** (módulo `MengerInfiniteRender`, carga en
  `PostConfigInit` para registrar `/MengerShaders`), tras el tonemapper y no como material de
  post-proceso: todo es texto versionable (`Shaders/MengerSponge.usf` + C++), sin `.uasset`, y ni
  la exposición ni el bloom de UE tocan la imagen, que lleva su propio tonemap ACES y gamma.
  `r.AntiAliasingMethod=0` para que el FXAA no se aplique encima.
- El actor de la cámara se queda en el origen de UE y solo aporta orientación y FOV. El juego
  entrega el marco con `FMengerViewExtension::SetFrameParams`, que llega al hilo de render en
  `BeginRenderViewFamily`.
- Mejoras sobre el port de OpenGL: sombra y AO con LOD por tamaño de rasgo (el `MapLo` original
  ponía los puntos de los agujeros pequeños dentro de lo sólido y salían manchas y rayas negras),
  penumbra mejorada de Aaltonen, sphere tracing sobrerrelajado (Keinert 2014) y dithering.
- Pendiente (fase 3): la nave se renderiza con normalidad cerca del origen y el pase compara
  `SceneDepth` con la `t` del rayo para dejarla delante cuando está más cerca.
- CVars: `r.Menger.Enable`, `r.Menger.MaxSteps`, `r.Menger.PreFolds`, `r.Menger.MaxDist`.

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

| Fase | Entregable | Aceptación | Estado |
|---|---|---|---|
| 1 | Esponja por raymarching en post-proceso, cámara libre | 10 niveles a ≥ 60 fps a 1080p; imagen equivalente a la versión OpenGL | Hecha (2026-10-24): de 90 a 183 fps con el port; misma vista inicial que OpenGL |
| 2 | Renormalización (`Path` + `LocalPos`) | Tests de ida y vuelta bajar/subir y de acarreo lateral; descenso a nivel 50 sin degradación visible | Hecha (2026-10-24): 12 tests; capturas idénticas en los niveles 10, 50 y 1000 a ~125 fps (RTX 4060 portátil) |
| 3 | Nave, rueda y colisión por DE | La velocidad cambia con la rueda; la nave no atraviesa paredes en 100 choques seguidos (test con posiciones fijas) | Pendiente; la rueda ya escala vista y velocidad |
| 4 | Pulido | Color por profundidad o nivel, HUD con nivel `k` y velocidad, lámina final de capturas | Pendiente; hay texto de depuración con nivel y escala |

## Fuera de alcance

Multijugador, generación de malla real del fractal (todo es raymarching), Lumen sobre el fractal.
Si sobra tiempo, añadir otros fractales de `FRACTALS.md` reutilizando el mismo sistema.

## Compilar y probar

```powershell
powershell -File Tools/build.ps1                  # editor (Development)
powershell -File Tools/build.ps1 -Target Game     # juego
powershell -File Tools/test.ps1                   # Automation Tests sin interfaz
powershell -File Tools/bench.ps1                  # abre el juego: fps por pose, capturas y comparación entre niveles
```

Con el editor abierto, la compilación por línea de comandos falla por Live Coding: cerrarlo antes.
La primera vez, clic derecho en `MengerInfinite.uproject` → *Generate Visual Studio project files*.
