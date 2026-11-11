#pragma once

#include "CoreMinimal.h"

/**
 * Posición exacta a cualquier profundidad dentro de la esponja y marco de render asociado.
 * Todo son funciones puras: devuelven copias y no dependen del mundo.
 */
namespace Menger
{
	/** Niveles entre la celda de la cámara y el marco en el que se evalúa el DE (3^6 = 729). */
	inline constexpr int32 FrameDepth = 6;

	/**
	 * Celda raíz en la red de lado 2 (la esponja raíz es la celda (0, 0, 0); con la repetición de
	 * periodo 4 hay esponja en las celdas de índices pares), un dígito por nivel y la posición local
	 * en [-1, 1]^3 dentro de la celda del último nivel.
	 */
	struct FFractalPosition
	{
		FIntVector RootCell = FIntVector::ZeroValue;
		/** Digits[i] es el subcubo del nivel i + 1, con componentes en {-1, 0, 1}. */
		TArray<FIntVector> Digits;
		FVector3d Local = FVector3d::ZeroVector;

		int32 GetLevel() const { return Digits.Num(); }
	};

	/** Un dígito es sólido si como mucho una de sus componentes es la central. */
	bool IsSolidDigit(const FIntVector& Digit);

	FFractalPosition FromRootPosition(const FVector3d& RootPosition, int32 Level);

	/** Posición en unidades raíz. Pierde precisión a partir del nivel ~30: solo para tests y depuración. */
	FVector3d ToRootPosition(const FFractalPosition& Position);

	/** Baja un nivel: apila el subcubo que contiene Local y reescala Local a ese subcubo. */
	FFractalPosition Descend(const FFractalPosition& Position);

	/** Sube un nivel. En el nivel 0 devuelve la posición sin cambios. */
	FFractalPosition Ascend(const FFractalPosition& Position);

	FFractalPosition WithLevel(const FFractalPosition& Position, int32 Level);

	/** Suma un desplazamiento en unidades locales y devuelve Local a [-1, 1]^3 con acarreo entre niveles. */
	FFractalPosition MoveLocal(const FFractalPosition& Position, const FVector3d& DeltaLocal);

	/** Celda vecina del mismo nivel, desplazada Offset celdas, con acarreo hasta la raíz. */
	FFractalPosition OffsetCell(const FFractalPosition& Position, const FIntVector& Offset);

	/** La celda del último nivel contiene una copia de la esponja (no está en un hueco). */
	bool IsCellSolid(const FFractalPosition& Position);

	/** Marco en el que el shader evalúa el DE para la cámara en Position. */
	struct FRenderFrame
	{
		/** Marco raíz con repetición de periodo 4; si no, bloque de 3x3x3 celdas con OccupancyMask. */
		bool bIsTiled = true;
		int32 FrameLevel = 0;
		/** Posición de la cámara en coordenadas del marco (celda central en [-1, 1]^3). */
		FVector3d CameraInFrame = FVector3d::ZeroVector;
		/** Unidades de marco por unidad local del nivel de la cámara: 3^-(nivel - FrameLevel). */
		double LocalToFrame = 1.0;
		/** Bit (x + 1) + 3 (y + 1) + 9 (z + 1) a 1 si la celda vecina con ese desplazamiento es sólida. */
		uint32 OccupancyMask = 0;
	};

	FRenderFrame MakeRenderFrame(const FFractalPosition& Position, int32 Depth = FrameDepth);

	/** Marcos que puede usar el shader: cada uno cubre 3^FrameDepth veces más distancia que el anterior. */
	inline constexpr int32 MaxRenderFrames = 4;

	/**
	 * Cascada de marcos a FrameDepth, 2·FrameDepth... niveles por encima de la cámara, hasta MaxFrames
	 * o hasta el marco raíz repetido, que cubre cualquier distancia y es el último.
	 */
	TArray<FRenderFrame> MakeRenderCascade(const FFractalPosition& Position, int32 MaxFrames = MaxRenderFrames);

	/** DE en coordenadas del marco, en double: espejo de FrameDE en el shader. */
	double FrameDistance(const FRenderFrame& Frame, const FVector3d& PointInFrame, int32 Iterations);

	/**
	 * Distancia de la cámara a la esponja en unidades locales de su nivel, resolviendo Iterations niveles
	 * por debajo. Si no hay geometría en el marco, prueba marcos más grandes; el resultado se satura en
	 * 3^MaxSearchLevels.
	 */
	double DistanceToSponge(const FFractalPosition& Position, int32 Iterations);
	inline constexpr int32 MaxSearchLevels = 36;
}
