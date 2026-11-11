#include "MengerMath.h"
#include "Misc/AutomationTest.h"

namespace
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** Bit de la celda central del bloque 3x3x3: con solo esa celda, el bloque es la esponja raíz. */
	constexpr uint32 CenterCellOnly = 1u << 13;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerSpongeDistanceTest, "MengerInfinite.Math.SpongeDistance", TestFlags)

bool FMengerSpongeDistanceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Sin pliegues es la distancia al cubo"), Menger::SpongeDistance(FVector3d(2.0, 0.0, 0.0), 0), 1.0, 1e-12);
	TestTrue(TEXT("El centro está vacío desde el nivel 1"), Menger::SpongeDistance(FVector3d::ZeroVector, 1) > 0.0);
	TestTrue(TEXT("Una esquina es sólida en el nivel 1"), Menger::SpongeDistance(FVector3d(0.95), 1) < 0.0);
	TestEqual(TEXT("La esquina (1,1,1) está en la superficie a cualquier nivel"), Menger::SpongeDistance(FVector3d(1.0), 30), 0.0, 1e-12);

	// Autosimilitud: el subcubo de la esquina (+,+,+) es la esponja escalada 1/3 alrededor de (2/3, 2/3, 2/3).
	const FVector3d Sorted(0.5, 0.2, -0.3);
	const FVector3d InCorner = FVector3d(2.0 / 3.0) + Sorted / 3.0;
	TestEqual(TEXT("DE(p, n) = 3 · DE(subcubo(p), n + 1)"),
		Menger::SpongeDistance(Sorted, 6), 3.0 * Menger::SpongeDistance(InCorner, 7), 1e-12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerTiledDistanceTest, "MengerInfinite.Math.TiledDistance", TestFlags)

bool FMengerTiledDistanceTest::RunTest(const FString& Parameters)
{
	const FVector3d Point(0.4, -0.7, 0.1);
	TestEqual(TEXT("En la celda raíz coincide con la esponja sola"),
		Menger::TiledSpongeDistance(Point, 8), Menger::SpongeDistance(Point, 8), 1e-15);
	TestEqual(TEXT("Periodo 4 en cada eje"),
		Menger::TiledSpongeDistance(Point + FVector3d(4.0, -8.0, 12.0), 8), Menger::SpongeDistance(Point, 8), 1e-12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerBlockDistanceTest, "MengerInfinite.Math.BlockDistance", TestFlags)

bool FMengerBlockDistanceTest::RunTest(const FString& Parameters)
{
	const FVector3d Inside(0.8, 0.85, -0.9);
	TestEqual(TEXT("Dentro de la celda central sólida es la esponja raíz"),
		Menger::BlockSpongeDistance(Inside, CenterCellOnly, 8), Menger::SpongeDistance(Inside, 8), 1e-15);
	TestEqual(TEXT("Un bloque vacío satura en 1"), Menger::BlockSpongeDistance(FVector3d(0.3, -0.2, 0.5), 0u, 8), 1.0);

	// Solo la celda +X ocupada: desde el centro de la celda central, la cara de esa celda está a 1.
	const uint32 PlusXOnly = 1u << 14;
	TestEqual(TEXT("La celda vecina se ve a través de la frontera"),
		Menger::BlockSpongeDistance(FVector3d(0.5, 0.9, 0.9), PlusXOnly, 0), 0.5, 1e-12);
	TestTrue(TEXT("Un punto dentro de la celda vecina sólida da distancia negativa"),
		Menger::BlockSpongeDistance(FVector3d(2.9, 0.95, 0.95), PlusXOnly, 3) < 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMengerStepScaleTest, "MengerInfinite.Math.StepScale", TestFlags)

bool FMengerStepScaleTest::RunTest(const FString& Parameters)
{
	const double Log3Factor = FMath::Loge(Menger::WheelFactor) / FMath::Loge(3.0);
	TestEqual(TEXT("Un paso hacia delante agranda la vista x1,25"), Menger::StepScale(5.0, 1.0f), 5.0 - Log3Factor, 1e-12);
	TestEqual(TEXT("Un paso hacia atrás la achica x1,25"), Menger::StepScale(5.0, -1.0f), 5.0 + Log3Factor, 1e-12);
	TestEqual(TEXT("Ida y vuelta vuelve al valor"), Menger::StepScale(Menger::StepScale(3.0, 4.0f), -4.0f), 3.0, 1e-12);
	TestEqual(TEXT("No baja de MinScale"), Menger::StepScale(Menger::MinScale, 10.0f), Menger::MinScale);
	TestEqual(TEXT("Sin techo: se puede bajar sin límite"), Menger::StepScale(1e6, -1.0f), 1e6 + Log3Factor, 1e-6);
	return true;
}
