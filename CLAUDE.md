# CLAUDE.md — MengerInfinite

## Qué es

Proyecto de Unreal Engine 5.6 con C++20 (módulo único `MengerInfinite`). Esponja de Menger con profundidad infinita por raymarching en post-proceso, con renormalización de cámara por autosimilitud (pila de dígitos + posición local) y una nave cuya velocidad se regula con la rueda.

El diseño, la arquitectura, los riesgos y las fases con criterios de aceptación están en `README.md`: leerlo antes de empezar y mantenerlo al día cuando cambie una decisión.

## Stack

- Unreal Engine 5.6 (`C:/Program Files/Epic Games/UE_5.6`; variable `UE_ROOT` para cambiarlo).
- Módulo `MengerInfinite` en `Source/MengerInfinite/`; categoría de log `LogMengerInfinite`.

## Cómo compilar / ejecutar

```powershell
powershell -File Tools/build.ps1                  # editor (Development)
powershell -File Tools/build.ps1 -Target Game     # juego
```

Con el editor abierto, `Build.bat` falla por Live Coding: cerrarlo antes de compilar.

## Tests

```powershell
powershell -File Tools/test.ps1 [-Filter <prefijo>]
```

Automation Tests en `Source/MengerInfinite/Tests/*.cpp`; el script falla si algún test no pasa
o si no se ejecuta ninguno. Regla: la lógica que mueve números va en funciones puras con su
test, sin depender del mundo ni de actores.
