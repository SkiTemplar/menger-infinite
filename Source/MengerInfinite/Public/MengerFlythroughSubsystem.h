#pragma once

#include "CoreMinimal.h"
#include "MengerFlythrough.h"
#include "Subsystems/WorldSubsystem.h"
#include "MengerFlythroughSubsystem.generated.h"

/**
 * Vídeo del descenso por línea de comandos (-MengerFlythrough): recorre Menger::MakeFlythroughPose frame a
 * frame (la fase depende del número de frame, no del reloj, así que el resultado es determinista), guarda
 * una captura por frame en Saved/Flythrough/frame_%05d.png y cierra el juego.
 *
 * Parámetros opcionales: -FlyFrames=720 (frames por ciclo; se guarda también el frame Frames, que debe
 * repetir el 0), -FlyLevels=2, -FlyStart=20, -FlyDistance=3, -FlyYaw=6, -FlyPitch=2, -FlyWarmup=60,
 * -FlyEvery=1 (guardar uno de cada N, para previsualizar).
 */
UCLASS()
class MENGERINFINITE_API UMengerFlythroughSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void HoldPose(double Phase) const;
	/** Falso (y lo registra) si se pidió -ResX/-ResY y el viewport no tiene ese tamaño. */
	bool HasRequestedViewportSize() const;
	void OnScreenshotCaptured(int32 Width, int32 Height, const TArray<FColor>& Colors);

	Menger::FFlythroughParams Params;
	FIntPoint RequestedSize = FIntPoint::ZeroValue;
	FString OutputDir;
	/** Fichero de la captura pedida en este frame; vacío si no hay ninguna pendiente. */
	FString PendingFile;
	FDelegateHandle ScreenshotHandle;
	int32 Frames = 720;
	int32 WarmupFrames = 60;
	int32 FrameStride = 1;
	int32 TickIndex = 0;
	int32 Requested = 0;
	int32 Saved = 0;
	bool bIsRunning = false;
};
