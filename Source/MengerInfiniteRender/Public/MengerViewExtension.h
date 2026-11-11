#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"
#include "SceneViewExtension.h"

struct FScreenPassTexture;
struct FPostProcessMaterialInputs;

/** Un marco de la cascada, ya reducido a float (lo calcula el juego en double con Menger::MakeRenderCascade). */
struct FMengerFrame
{
	/** Posición de la cámara en coordenadas del marco. */
	FVector3f CameraInFrame = FVector3f::ZeroVector;
	/** Unidades de marco por unidad de vista. */
	float ViewToFrame = 1.0f;
	/** Celdas sólidas del bloque 3x3x3 del marco; se ignora si bIsTiled. */
	uint32 OccupancyMask = 0;
	/** Marco raíz con la esponja repetida: cubre cualquier distancia. */
	bool bIsTiled = true;
};

/**
 * Cámara del fractal para un frame: cascada de marcos del más fino al más grueso. El shader usa en
 * cada muestra del rayo el marco más fino que la cubre.
 */
struct FMengerFrameParams
{
	static constexpr int32 MaxFrames = 4;

	TStaticArray<FMengerFrame, MaxFrames> Frames;
	int32 NumFrames = 0;
};

/**
 * Dibuja la esponja de Menger por raymarching en un pase de pantalla completa tras el tonemapper.
 * La orientación y el FOV salen de la vista de UE; la posición, de los marcos que entrega el juego
 * con SetFrameParams. Solo actúa en el mundo con el que se registra.
 */
class MENGERINFINITERENDER_API FMengerViewExtension final : public FWorldSceneViewExtension
{
public:
	FMengerViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld);

	/** Hilo de juego: parámetros que usará el siguiente frame que se renderice. */
	void SetFrameParams(const FMengerFrameParams& Params);

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& View,
		FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	FScreenPassTexture RenderSponge_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
		const FPostProcessMaterialInputs& Inputs);

	TOptional<FMengerFrameParams> GameThreadParams;
	TOptional<FMengerFrameParams> RenderThreadParams;
};
