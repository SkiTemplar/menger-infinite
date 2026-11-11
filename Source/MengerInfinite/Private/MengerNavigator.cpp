#include "MengerNavigator.h"

#include "MengerMath.h"

namespace Menger
{
	namespace
	{
		/** Escala a la que la geometría más cercana queda a GeometryViewRatio unidades de vista. */
		double GeometryScale(const FFractalPosition& Position)
		{
			const double Distance = DistanceToSponge(Position, CameraDistanceIterations);
			if (Distance <= 0.0)
			{
				return TNumericLimits<double>::Max();
			}
			return Position.GetLevel() - FMath::LogX(3.0, Distance / GeometryViewRatio);
		}
	}

	double ViewToLocal(const FCameraState& State)
	{
		return FMath::Pow(3.0, State.Position.GetLevel() - State.ViewScale);
	}

	FCameraState MakeCameraState(const FFractalPosition& Position, double UserScale)
	{
		FCameraState State;
		State.UserScale = FMath::Max(MinScale, UserScale);
		State.ViewScale = FMath::Max(MinScale, FMath::Min(State.UserScale, GeometryScale(Position)));
		State.Position = WithLevel(Position, FMath::Max(0, FMath::FloorToInt32(State.ViewScale)));
		return State;
	}

	FCameraState MoveCamera(const FCameraState& State, const FVector3d& DeltaView)
	{
		const FFractalPosition Moved = MoveLocal(State.Position, DeltaView * ViewToLocal(State));
		return MakeCameraState(Moved, State.UserScale);
	}

	FCameraState StepCameraScale(const FCameraState& State, float WheelSteps)
	{
		return MakeCameraState(State.Position, StepScale(State.UserScale, WheelSteps));
	}

	FCameraPose MakeOverviewPose()
	{
		return { TEXT("Overview"), FromRootPosition(FVector3d(-2.5, 0.0, 0.0), 0), 0.0, FRotator::ZeroRotator };
	}

	FCameraPose MakeWallPose(int32 Level)
	{
		const int32 SafeLevel = FMath::Max(Level, 1);
		FCameraPose Pose;
		Pose.Name = FString::Printf(TEXT("Wall%02d"), SafeLevel);
		Pose.Scale = SafeLevel;
		Pose.Rotation = FRotator(0.0, 20.0, 0.0);

		FFractalPosition& Position = Pose.Position;
		Position.Digits.Reserve(SafeLevel);
		Position.Digits.Add(FIntVector(-1, 0, 0));
		for (int32 DigitLevel = 2; DigitLevel <= SafeLevel; ++DigitLevel)
		{
			const int32 Alternating = DigitLevel % 2 == 0 ? 1 : -1;
			Position.Digits.Add(FIntVector(Alternating, 1, Alternating));
		}
		const double AlternatingLocal = SafeLevel % 2 == 1 ? 0.5 : -0.5;
		Position.Local = FVector3d(AlternatingLocal, 0.5, AlternatingLocal);
		return Pose;
	}
}
