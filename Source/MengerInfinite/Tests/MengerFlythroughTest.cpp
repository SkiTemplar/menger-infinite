#include "MengerFlythrough.h"
#include "MengerNavigator.h"
#include "MengerPath.h"
#include "Misc/AutomationTest.h"

namespace
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerFlythroughTargetTest, "MengerInfinite.Flythrough.Target", TestFlags)

bool FMengerFlythroughTargetTest::RunTest(const FString& Parameters)
{
	for (int32 Level = 1; Level <= 12; ++Level)
	{
		const FVector3d Root = Menger::ToRootPosition(Menger::MakeFlythroughTarget(Level));
		TestTrue(TEXT("La esquina F está en (-1/3, -1/3, -1/3) a cualquier nivel"), Root.Equals(FVector3d(-1.0 / 3.0), 1e-12));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerFlythroughLoopTest, "MengerInfinite.Flythrough.Loop", TestFlags)

bool FMengerFlythroughLoopTest::RunTest(const FString& Parameters)
{
	const Menger::FFlythroughParams Params;
	const Menger::FCameraPose First = Menger::MakeFlythroughPose(Params, 0.0);
	const Menger::FCameraPose Last = Menger::MakeFlythroughPose(Params, 1.0);
	const Menger::FCameraState FirstState = Menger::MakeCameraState(First.Position, First.Scale);
	const Menger::FCameraState LastState = Menger::MakeCameraState(Last.Position, Last.Scale);

	TestEqual(TEXT("La escala sube Levels niveles"), LastState.ViewScale - FirstState.ViewScale, static_cast<double>(Params.Levels));
	TestTrue(TEXT("La orientación del último frame es la del primero"), Last.Rotation.Equals(First.Rotation, 1e-9));
	TestEqual(TEXT("Misma escala de vista en unidades locales"), Menger::ViewToLocal(LastState), Menger::ViewToLocal(FirstState));

	// El shader solo ve la cascada de marcos: si los marcos que alcanza el rayo coinciden, la imagen también.
	const TArray<Menger::FRenderFrame> FirstCascade = Menger::MakeRenderCascade(FirstState.Position);
	const TArray<Menger::FRenderFrame> LastCascade = Menger::MakeRenderCascade(LastState.Position);
	constexpr int32 ComparedFrames = 3;
	TestTrue(TEXT("Hay marcos que comparar"), FirstCascade.Num() >= ComparedFrames && LastCascade.Num() >= ComparedFrames);
	for (int32 Index = 0; Index < FMath::Min3(ComparedFrames, FirstCascade.Num(), LastCascade.Num()); ++Index)
	{
		const Menger::FRenderFrame& A = FirstCascade[Index];
		const Menger::FRenderFrame& B = LastCascade[Index];
		TestEqual(TEXT("Mismo tipo de marco"), B.bIsTiled, A.bIsTiled);
		TestEqual(TEXT("Misma máscara de ocupación"), B.OccupancyMask, A.OccupancyMask);
		TestTrue(TEXT("Misma cámara en el marco"), B.CameraInFrame.Equals(A.CameraInFrame, 1e-12));
		TestEqual(TEXT("Misma escala del marco"), B.LocalToFrame, A.LocalToFrame);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerFlythroughClearanceTest, "MengerInfinite.Flythrough.Clearance", TestFlags)

bool FMengerFlythroughClearanceTest::RunTest(const FString& Parameters)
{
	const Menger::FFlythroughParams Params;
	constexpr int32 Samples = 240;
	for (int32 Sample = 0; Sample <= Samples; ++Sample)
	{
		const double Phase = static_cast<double>(Sample) / Samples;
		const Menger::FCameraPose Pose = Menger::MakeFlythroughPose(Params, Phase);
		const Menger::FCameraState State = Menger::MakeCameraState(Pose.Position, Pose.Scale);
		const double WallView = Menger::DistanceToSponge(State.Position, Menger::CameraDistanceIterations) / Menger::ViewToLocal(State);

		TestTrue(TEXT("La escala de vista es la pedida (hay geometría cerca)"), FMath::IsNearlyEqual(State.ViewScale, Pose.Scale, 1e-9));
		TestTrue(TEXT("La cámara no toca la esponja"), WallView > 0.5);
		TestTrue(TEXT("La cámara no se aleja de la esquina"), WallView < Params.Distance);
	}
	return true;
}
