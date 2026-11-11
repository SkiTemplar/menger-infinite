#include "MengerGameMode.h"

#include "MengerFreeCameraPawn.h"
#include "MengerNavigator.h"

AMengerGameMode::AMengerGameMode()
{
	DefaultPawnClass = AMengerFreeCameraPawn::StaticClass();
}

void AMengerGameMode::RestartPlayer(AController* NewPlayer)
{
	// El pawn se queda en el origen de UE: su posición en el fractal vive en su FCameraState.
	RestartPlayerAtTransform(NewPlayer, FTransform(Menger::MakeOverviewPose().Rotation));
}
