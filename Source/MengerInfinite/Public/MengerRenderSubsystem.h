#pragma once

#include "CoreMinimal.h"
#include "MengerViewExtension.h"
#include "Subsystems/WorldSubsystem.h"
#include "MengerRenderSubsystem.generated.h"

/** Registra la extensión de vista que dibuja la esponja en los mundos de juego y le pasa la cámara. */
UCLASS()
class MENGERINFINITE_API UMengerRenderSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Cámara del fractal para el siguiente frame. */
	void SetFrameParams(const FMengerFrameParams& Params);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	TSharedPtr<FMengerViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
