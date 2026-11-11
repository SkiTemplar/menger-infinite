#pragma once

#include "CoreMinimal.h"

/** Lógica numérica pura de la esponja, sin dependencias del mundo ni de actores. */
namespace Menger
{
	/** Periodo de la repetición de la esponja raíz en el marco raíz. */
	inline constexpr double TilePeriod = 4.0;

	/**
	 * Distancia con signo a la esponja raíz (cubo [-1, 1]^3) con Iterations pliegues.
	 * Es el espejo en double de MengerDE en Shaders/MengerSponge.usf.
	 */
	double SpongeDistance(const FVector3d& Point, int32 Iterations);

	/** Esponja raíz repetida con periodo TilePeriod (marco raíz del shader). */
	double TiledSpongeDistance(const FVector3d& Point, int32 Iterations);

	/**
	 * Bloque de 3x3x3 celdas de lado 2 centrado en el origen; cada celda sólida de OccupancyMask es una
	 * copia de la esponja. Solo evalúa las 8 celdas del octante más cercano: el resto está a 1 o más,
	 * así que la distancia se satura en 1. Espejo de FrameDE en el shader.
	 */
	double BlockSpongeDistance(const FVector3d& Point, uint32 OccupancyMask, int32 Iterations);

	/** Factor de la rueda del ratón sobre la velocidad y la escala de vista. */
	inline constexpr double WheelFactor = 1.25;

	/**
	 * Escala de vista en log3: una unidad de vista mide 3^-Scale unidades raíz. Cada paso de rueda
	 * positivo (hacia delante) la agranda un factor WheelFactor.
	 */
	double StepScale(double Scale, float WheelSteps);
	inline constexpr double MinScale = -2.0;
}
