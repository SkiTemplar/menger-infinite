#pragma once

#include "CoreMinimal.h"
#include "MengerPath.h"

/**
 * Estado de la cámara en el fractal y su avance por tick, como funciones puras.
 *
 * Escalas en log3: una unidad de vista mide 3^-Scale unidades raíz. La niebla, la sombra y la AO del
 * shader van en unidades de vista, así que la imagen solo depende de la geometría vista a esa escala:
 * es la misma en el nivel 10 que en el 1000.
 */
namespace Menger
{
	/** Si la geometría más cercana está a más de estas unidades de vista, la escala de vista crece. */
	inline constexpr double GeometryViewRatio = 30.0;

	/** Niveles por debajo de la cámara que resuelve el DE de la cámara en CPU. */
	inline constexpr int32 CameraDistanceIterations = 6;

	struct FCameraState
	{
		FFractalPosition Position;
		/** Escala pedida con la rueda: la más pequeña a la que puede bajar la vista. */
		double UserScale = 0.0;
		/** Escala efectiva: UserScale, o más grande si no hay geometría cerca. Fija el nivel de Position. */
		double ViewScale = 0.0;
	};

	/** Unidades locales (del nivel de Position) por unidad de vista. */
	double ViewToLocal(const FCameraState& State);

	/** Calcula ViewScale para Position y UserScale y lleva Position al nivel floor(ViewScale). */
	FCameraState MakeCameraState(const FFractalPosition& Position, double UserScale);

	/** Desplaza la cámara DeltaView unidades de vista y renormaliza. */
	FCameraState MoveCamera(const FCameraState& State, const FVector3d& DeltaView);

	/** Aplica pasos de rueda a UserScale y renormaliza. */
	FCameraState StepCameraScale(const FCameraState& State, float WheelSteps);

	struct FCameraPose
	{
		FString Name;
		FFractalPosition Position;
		double Scale = 0.0;
		FRotator Rotation;
	};

	/** Vista exterior equivalente a la posición inicial del proyecto OpenGL. */
	FCameraPose MakeOverviewPose();

	/**
	 * Cámara en el túnel X, a 0,5 unidades locales del nivel Level de la pared y = 1/3, sobre un punto
	 * de la alfombra de Sierpinski (sólido en todos los niveles), mirándola en rasante. Se construye por
	 * dígitos, así que es exacta a cualquier nivel: x = -0,5 y z = 1/6 alternan dígitos -1/+1 desde el
	 * nivel 2 e y = 1/3 - 0,5·3^-Level tiene todos los dígitos +1. Escala de vista = Level.
	 */
	FCameraPose MakeWallPose(int32 Level);
}
