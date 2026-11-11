#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MengerGameMode.generated.h"

/** Usa la cámara libre y la coloca en la vista exterior de la esponja. */
UCLASS()
class MENGERINFINITE_API AMengerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMengerGameMode();

	virtual void RestartPlayer(AController* NewPlayer) override;
};
