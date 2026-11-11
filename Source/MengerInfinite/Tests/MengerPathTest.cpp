#include "MengerMath.h"
#include "MengerNavigator.h"
#include "MengerPath.h"
#include "Misc/AutomationTest.h"

namespace
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	bool IsLocalInRange(const Menger::FFractalPosition& Position)
	{
		return Position.Local.GetAbs().GetMax() <= 1.0;
	}

	/** Posición de la pared del nivel Level en coordenadas raíz (solo exacta hasta el nivel ~30). */
	FVector3d WallRootPosition(int32 Level)
	{
		return FVector3d(-0.5, 1.0 / 3.0 - 0.5 * FMath::Pow(3.0, -static_cast<double>(Level)), 1.0 / 6.0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerPathRoundTripTest, "MengerInfinite.Path.RoundTrip", TestFlags)

bool FMengerPathRoundTripTest::RunTest(const FString& Parameters)
{
	FRandomStream Random(1234);
	for (int32 Sample = 0; Sample < 200; ++Sample)
	{
		const FVector3d Root(Random.FRandRange(-5.0, 5.0), Random.FRandRange(-5.0, 5.0), Random.FRandRange(-5.0, 5.0));
		const int32 Level = Random.RandRange(0, 12);
		const Menger::FFractalPosition Position = Menger::FromRootPosition(Root, Level);

		TestEqual(TEXT("Nivel pedido"), Position.GetLevel(), Level);
		TestTrue(TEXT("Local en [-1, 1]^3"), IsLocalInRange(Position));
		TestTrue(TEXT("Ida y vuelta a coordenadas raíz"), ToRootPosition(Position).Equals(Root, 1e-12));

		const Menger::FFractalPosition UpDown = Menger::Ascend(Menger::Descend(Position));
		TestTrue(TEXT("Subir tras bajar devuelve la posición"), UpDown.Local.Equals(Position.Local, 1e-12));
		TestEqual(TEXT("Subir tras bajar devuelve el nivel"), UpDown.GetLevel(), Level);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerPathCarryTest, "MengerInfinite.Path.Carry", TestFlags)

bool FMengerPathCarryTest::RunTest(const FString& Parameters)
{
	FRandomStream Random(99);
	constexpr int32 Level = 6;
	const double LocalToRoot = FMath::Pow(3.0, -static_cast<double>(Level));
	for (int32 Sample = 0; Sample < 200; ++Sample)
	{
		const FVector3d Root(Random.FRandRange(-2.0, 2.0), Random.FRandRange(-2.0, 2.0), Random.FRandRange(-2.0, 2.0));
		const FVector3d Delta(Random.FRandRange(-40.0, 40.0), Random.FRandRange(-40.0, 40.0), Random.FRandRange(-40.0, 40.0));
		const Menger::FFractalPosition Start = Menger::FromRootPosition(Root, Level);
		const Menger::FFractalPosition Moved = Menger::MoveLocal(Start, Delta);

		TestEqual(TEXT("Mover no cambia el nivel"), Moved.GetLevel(), Level);
		TestTrue(TEXT("Local vuelve a [-1, 1]^3"), IsLocalInRange(Moved));
		TestTrue(TEXT("El acarreo equivale a mover en coordenadas raíz"),
			ToRootPosition(Moved).Equals(Root + Delta * LocalToRoot, 1e-12));
	}

	// Acarreo hasta la raíz: desde la última celda +X de la esponja raíz hacia la celda raíz vecina.
	Menger::FFractalPosition Edge;
	Edge.Digits = { FIntVector(1, 0, 1), FIntVector(1, 1, 0), FIntVector(1, -1, -1) };
	Edge.Local = FVector3d(0.9, 0.0, 0.0);
	const Menger::FFractalPosition Crossed = Menger::MoveLocal(Edge, FVector3d(0.2, 0.0, 0.0));
	TestEqual(TEXT("La celda raíz avanza una en X"), Crossed.RootCell, FIntVector(1, 0, 0));
	TestEqual(TEXT("Los dígitos X dan la vuelta a -1"), Crossed.Digits[0].X + Crossed.Digits[1].X + Crossed.Digits[2].X, -3);
	TestEqual(TEXT("Local entra por el otro lado"), Crossed.Local.X, -0.9, 1e-12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerPathSolidityTest, "MengerInfinite.Path.Solidity", TestFlags)

bool FMengerPathSolidityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Esquina sólida"), Menger::IsSolidDigit(FIntVector(1, -1, 1)));
	TestTrue(TEXT("Arista sólida"), Menger::IsSolidDigit(FIntVector(0, 1, -1)));
	TestFalse(TEXT("Centro de cara vacío"), Menger::IsSolidDigit(FIntVector(0, 0, 1)));
	TestFalse(TEXT("Centro vacío"), Menger::IsSolidDigit(FIntVector(0, 0, 0)));

	Menger::FFractalPosition Position;
	Position.Digits = { FIntVector(1, 1, 0), FIntVector(-1, 1, 1) };
	TestTrue(TEXT("Todos los dígitos sólidos"), Menger::IsCellSolid(Position));
	Position.RootCell = FIntVector(1, 0, 0);
	TestFalse(TEXT("Celda raíz impar: hueco entre repeticiones"), Menger::IsCellSolid(Position));
	Position.RootCell = FIntVector(-2, 0, 4);
	TestTrue(TEXT("Celda raíz par: otra copia de la esponja"), Menger::IsCellSolid(Position));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerWallPoseTest, "MengerInfinite.Path.WallPose", TestFlags)

bool FMengerWallPoseTest::RunTest(const FString& Parameters)
{
	for (int32 Level = 1; Level <= 12; ++Level)
	{
		const Menger::FFractalPosition ByDigits = Menger::MakeWallPose(Level).Position;
		const Menger::FFractalPosition ByRoot = Menger::FromRootPosition(WallRootPosition(Level), Level);
		TestTrue(FString::Printf(TEXT("Nivel %d: mismos dígitos que desde coordenadas raíz"), Level), ByDigits.Digits == ByRoot.Digits);
		TestTrue(FString::Printf(TEXT("Nivel %d: misma posición local"), Level), ByDigits.Local.Equals(ByRoot.Local, 1e-6));
	}

	for (const int32 Level : { 1, 5, 10, 50, 1000 })
	{
		const Menger::FFractalPosition Position = Menger::MakeWallPose(Level).Position;
		const double Distance = Menger::DistanceToSponge(Position, 8);
		TestTrue(FString::Printf(TEXT("Nivel %d: la cámara está en hueco (DE %.3g)"), Level, Distance), Distance > 0.0);
		TestTrue(FString::Printf(TEXT("Nivel %d: la pared está a 0,5 unidades locales (DE %.3g)"), Level, Distance),
			Distance <= 0.5 + 1e-12 && Distance >= 0.125);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerRenderFrameTest, "MengerInfinite.Path.RenderFrame", TestFlags)

bool FMengerRenderFrameTest::RunTest(const FString& Parameters)
{
	const Menger::FRenderFrame Root = Menger::MakeRenderFrame(Menger::MakeOverviewPose().Position);
	TestTrue(TEXT("Vista exterior: marco raíz repetido"), Root.bIsTiled);
	// (-2,5, 0, 0) está en la celda raíz -1; el marco reduce por paridad a 1,5, el mismo punto módulo el periodo 4.
	TestTrue(TEXT("Vista exterior: la cámara está en (-2,5, 0, 0) módulo 4"), Root.CameraInFrame.Equals(FVector3d(1.5, 0.0, 0.0), 1e-12));
	TestEqual(TEXT("Vista exterior: mismo DE que en coordenadas raíz"),
		Menger::TiledSpongeDistance(Root.CameraInFrame, 8), Menger::TiledSpongeDistance(FVector3d(-2.5, 0.0, 0.0), 8), 1e-12);

	// La pose de pared repite sus dígitos con periodo 2: el marco debe ser idéntico en niveles de la misma paridad.
	const Menger::FRenderFrame Reference = Menger::MakeRenderFrame(Menger::MakeWallPose(10).Position);
	TestFalse(TEXT("Nivel 10: marco de bloque"), Reference.bIsTiled);
	TestEqual(TEXT("Nivel 10: marco 6 niveles arriba"), Reference.FrameLevel, 10 - Menger::FrameDepth);
	TestTrue(TEXT("Nivel 10: la celda vecina +Y (la pared) es sólida"), (Reference.OccupancyMask >> 16) & 1u);
	TestFalse(TEXT("Nivel 10: la celda de la cámara (túnel) está vacía"), (Reference.OccupancyMask >> 13) & 1u);

	for (const int32 Level : { 50, 1000 })
	{
		const Menger::FRenderFrame Deep = Menger::MakeRenderFrame(Menger::MakeWallPose(Level).Position);
		TestEqual(FString::Printf(TEXT("Nivel %d: misma ocupación que en el 10"), Level), Deep.OccupancyMask, Reference.OccupancyMask);
		TestTrue(FString::Printf(TEXT("Nivel %d: misma cámara en el marco que en el 10"), Level),
			Deep.CameraInFrame.Equals(Reference.CameraInFrame, 1e-15));
		TestEqual(FString::Printf(TEXT("Nivel %d: misma escala de marco"), Level), Deep.LocalToFrame, Reference.LocalToFrame);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerFrameDistanceTest, "MengerInfinite.Path.FrameDistance", TestFlags)

bool FMengerFrameDistanceTest::RunTest(const FString& Parameters)
{
	// El bloque de celdas y los pliegues sobre coordenadas raíz describen la misma esponja: deben coincidir
	// en qué puntos son sólidos. Las magnitudes difieren (en los huecos el DE plegado es solo una cota),
	// así que se exige el mismo signo y el mismo orden de magnitud.
	constexpr int32 Level = 8;
	const Menger::FFractalPosition Camera = Menger::MakeWallPose(Level).Position;
	const Menger::FRenderFrame Frame = Menger::MakeRenderFrame(Camera);
	const double LocalToRoot = FMath::Pow(3.0, -static_cast<double>(Level));
	const FVector3d CameraRoot = ToRootPosition(Camera);
	constexpr int32 LocalIterations = 5;

	FRandomStream Random(7);
	int32 SignMismatches = 0;
	for (int32 Sample = 0; Sample < 500; ++Sample)
	{
		const FVector3d Delta(Random.FRandRange(-3.0, 3.0), Random.FRandRange(-3.0, 3.0), Random.FRandRange(-3.0, 3.0));
		const double InFrame = Menger::FrameDistance(Frame, Frame.CameraInFrame + Delta * Frame.LocalToFrame,
			LocalIterations + Level - Frame.FrameLevel) / Frame.LocalToFrame;
		const double InRoot = Menger::TiledSpongeDistance(CameraRoot + Delta * LocalToRoot, LocalIterations + Level) / LocalToRoot;

		if (FMath::Abs(InFrame) < 1e-6 || FMath::Abs(InRoot) < 1e-6)
		{
			continue;
		}
		SignMismatches += (InFrame > 0.0) != (InRoot > 0.0);
		const double Ratio = InFrame / InRoot;
		TestTrue(FString::Printf(TEXT("Mismo orden de magnitud (marco %.4g, raíz %.4g)"), InFrame, InRoot), Ratio > 0.2 && Ratio < 5.0);
	}
	TestEqual(TEXT("Mismo signo en todos los puntos"), SignMismatches, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerNavigatorTest, "MengerInfinite.Path.Navigator", TestFlags)

bool FMengerNavigatorTest::RunTest(const FString& Parameters)
{
	// A nivel 1000 un double en coordenadas raíz ya no distingue nada; aquí ir y volver debe ser exacto.
	const Menger::FCameraPose Pose = Menger::MakeWallPose(1000);
	const Menger::FCameraState Start = Menger::MakeCameraState(Pose.Position, Pose.Scale);
	TestEqual(TEXT("La pose de nivel 1000 se queda en el nivel 1000"), Start.Position.GetLevel(), 1000);
	TestEqual(TEXT("Una unidad de vista es una unidad local"), Menger::ViewToLocal(Start), 1.0, 1e-12);

	Menger::FCameraState State = Start;
	for (int32 Step = 0; Step < 100; ++Step)
	{
		State = Menger::MoveCamera(State, FVector3d(0.3, 0.0, 0.05));
	}
	TestTrue(TEXT("Tras 30 unidades de vista sigue en hueco"), Menger::DistanceToSponge(State.Position, 6) > 0.0);
	for (int32 Step = 0; Step < 100; ++Step)
	{
		State = Menger::MoveCamera(State, FVector3d(-0.3, 0.0, -0.05));
	}
	TestTrue(TEXT("Ida y vuelta: mismos dígitos"), State.Position.Digits == Start.Position.Digits);
	TestTrue(TEXT("Ida y vuelta: misma posición local"), State.Position.Local.Equals(Start.Position.Local, 1e-9));

	// Lejos de la geometría la escala de vista crece para que la pared quede a la vista.
	const Menger::FCameraState Outside = Menger::MakeCameraState(Menger::MakeOverviewPose().Position, 10.0);
	TestTrue(TEXT("Fuera de la esponja la vista no baja a la escala pedida"), Outside.ViewScale < 10.0);
	TestEqual(TEXT("El nivel es el entero de la escala de vista"), Outside.Position.GetLevel(), FMath::FloorToInt32(Outside.ViewScale));

	const Menger::FCameraState Stepped = Menger::StepCameraScale(Start, -3.0f);
	TestTrue(TEXT("La rueda hacia atrás pide una escala más fina"), Stepped.UserScale > Start.UserScale);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerOccupancyTest, "MengerInfinite.Path.Occupancy", TestFlags)

bool FMengerOccupancyTest::RunTest(const FString& Parameters)
{
	// La máscara rápida (solo los niveles que toca el acarreo) debe coincidir con copiar la ruta de cada vecina.
	FRandomStream Random(42);
	for (int32 Sample = 0; Sample < 300; ++Sample)
	{
		const FVector3d Root(Random.FRandRange(-1.2, 1.2), Random.FRandRange(-1.2, 1.2), Random.FRandRange(-1.2, 1.2));
		const int32 Level = Random.RandRange(Menger::FrameDepth + 1, 14);
		const Menger::FFractalPosition Position = Menger::FromRootPosition(Root, Level);
		const Menger::FRenderFrame Frame = Menger::MakeRenderFrame(Position);

		Menger::FFractalPosition FrameCell = Position;
		FrameCell.Digits.SetNum(Frame.FrameLevel);
		uint32 Expected = 0;
		for (int32 Bit = 0; Bit < 27; ++Bit)
		{
			const FIntVector Offset(Bit % 3 - 1, (Bit / 3) % 3 - 1, Bit / 9 - 1);
			Expected |= Menger::IsCellSolid(Menger::OffsetCell(FrameCell, Offset)) ? 1u << Bit : 0u;
		}
		TestEqual(FString::Printf(TEXT("Muestra %d: misma máscara que copiando la ruta"), Sample), Frame.OccupancyMask, Expected);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerCascadeTest, "MengerInfinite.Path.Cascade", TestFlags)

bool FMengerCascadeTest::RunTest(const FString& Parameters)
{
	const TArray<Menger::FRenderFrame> Shallow = Menger::MakeRenderCascade(Menger::MakeWallPose(10).Position);
	TestEqual(TEXT("Nivel 10: bloque a 6 niveles y marco raíz"), Shallow.Num(), 2);
	TestTrue(TEXT("Nivel 10: el último marco es el raíz repetido"), Shallow.Last().bIsTiled);

	const TArray<Menger::FRenderFrame> Deep = Menger::MakeRenderCascade(Menger::MakeWallPose(50).Position);
	const TArray<Menger::FRenderFrame> Deeper = Menger::MakeRenderCascade(Menger::MakeWallPose(1000).Position);
	TestEqual(TEXT("Nivel 50: todos los marcos"), Deep.Num(), Menger::MaxRenderFrames);
	TestEqual(TEXT("Nivel 1000: todos los marcos"), Deeper.Num(), Menger::MaxRenderFrames);
	for (int32 Index = 0; Index < FMath::Min(Deep.Num(), Deeper.Num()); ++Index)
	{
		TestEqual(FString::Printf(TEXT("Marco %d: nivel 6·(i+1) por encima"), Index), Deep[Index].FrameLevel, 50 - Menger::FrameDepth * (Index + 1));
		TestEqual(FString::Printf(TEXT("Marco %d: misma ocupación en los niveles 50 y 1000"), Index), Deep[Index].OccupancyMask, Deeper[Index].OccupancyMask);
		TestTrue(FString::Printf(TEXT("Marco %d: misma cámara en los niveles 50 y 1000"), Index),
			Deep[Index].CameraInFrame.Equals(Deeper[Index].CameraInFrame, 1e-15));
	}
	return true;
}
