#include "MengerFlythrough.h"

#include "Math/RotationMatrix.h"

namespace Menger
{
	FFractalPosition MakeFlythroughTarget(int32 Level)
	{
		FFractalPosition Target;
		Target.Digits.Init(FIntVector(1, 1, 1), FMath::Max(Level, 1));
		Target.Digits[0] = FIntVector(-1, -1, -1);
		Target.Local = FVector3d(1.0, 1.0, 1.0);
		return Target;
	}

	FCameraPose MakeFlythroughPose(const FFlythroughParams& Params, double Phase)
	{
		const double Scale = Params.StartScale + Params.Levels * Phase;
		const int32 Level = FMath::Max(1, FMath::FloorToInt32(Scale));

		// El balanceo usa solo la parte fraccionaria: en Phase = 1 la orientación es exactamente la de Phase = 0.
		const double Angle = 2.0 * UE_DOUBLE_PI * (Phase - FMath::FloorToDouble(Phase));
		const FRotator Sway(Params.PitchAmplitude * FMath::Sin(2.0 * Angle), Params.YawAmplitude * FMath::Sin(Angle), 0.0);

		// Base: mirando a F desde el hueco central, por la diagonal, con Z como referencia de "arriba".
		const FVector3d Diagonal = FVector3d(1.0, 1.0, 1.0).GetSafeNormal();
		const FQuat4d Base = FRotationMatrix::MakeFromXZ(-Diagonal, FVector3d::UpVector).ToQuat();
		const FQuat4d View = Base * FQuat4d(Sway);
		const FVector3d Forward = View.GetForwardVector();

		FCameraPose Pose;
		Pose.Name = FString::Printf(TEXT("Flythrough%.4f"), Phase);
		Pose.Scale = Scale;
		Pose.Rotation = View.Rotator();
		// La cámara mira a F: queda a Distance unidades de vista detrás, en el sentido opuesto a la vista.
		const double ViewToLocalUnits = FMath::Pow(3.0, Level - Scale);
		Pose.Position = MoveLocal(MakeFlythroughTarget(Level), -Forward * (Params.Distance * ViewToLocalUnits));
		return Pose;
	}
}
