#include "MengerMath.h"

namespace Menger
{
	namespace
	{
		double BoxDistance(const FVector3d& Point)
		{
			const FVector3d D = Point.GetAbs() - FVector3d::OneVector;
			const double Inside = FMath::Min(D.GetMax(), 0.0);
			return Inside + D.ComponentMax(FVector3d::ZeroVector).Length();
		}

		bool IsOccupied(uint32 OccupancyMask, const FIntVector& Cell)
		{
			const int32 Bit = (Cell.X + 1) + 3 * (Cell.Y + 1) + 9 * (Cell.Z + 1);
			return (OccupancyMask >> Bit) & 1u;
		}
	}

	double SpongeDistance(const FVector3d& Point, int32 Iterations)
	{
		FVector3d P = Point;
		double Scale = 1.0;
		for (int32 i = 0; i < Iterations; ++i)
		{
			P = P.GetAbs();
			if (P.X < P.Y) { Swap(P.X, P.Y); }
			if (P.X < P.Z) { Swap(P.X, P.Z); }
			if (P.Y < P.Z) { Swap(P.Y, P.Z); }
			Scale *= 3.0;
			P = P * 3.0 - FVector3d(2.0);
			if (P.Z < -1.0) { P.Z += 2.0; }
		}
		return BoxDistance(P) / Scale;
	}

	double TiledSpongeDistance(const FVector3d& Point, int32 Iterations)
	{
		FVector3d Wrapped;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			Wrapped[Axis] = Point[Axis] - TilePeriod * FMath::RoundHalfFromZero(Point[Axis] / TilePeriod);
		}
		return SpongeDistance(Wrapped, Iterations);
	}

	double BlockSpongeDistance(const FVector3d& Point, uint32 OccupancyMask, int32 Iterations)
	{
		FIntVector Cell;
		FIntVector Side;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			Cell[Axis] = FMath::Clamp(FMath::RoundToInt32(Point[Axis] * 0.5), -1, 1);
			Side[Axis] = Point[Axis] - 2.0 * Cell[Axis] >= 0.0 ? 1 : -1;
		}

		double Best = 1.0;
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FIntVector Candidate(
				Cell.X + Side.X * (Corner & 1),
				Cell.Y + Side.Y * ((Corner >> 1) & 1),
				Cell.Z + Side.Z * ((Corner >> 2) & 1));
			const bool bIsInBlock = FMath::Abs(Candidate.X) <= 1 && FMath::Abs(Candidate.Y) <= 1 && FMath::Abs(Candidate.Z) <= 1;
			if (!bIsInBlock || !IsOccupied(OccupancyMask, Candidate))
			{
				continue;
			}
			const FVector3d InCell = Point - 2.0 * FVector3d(Candidate);
			if (BoxDistance(InCell) < Best)
			{
				Best = FMath::Min(Best, SpongeDistance(InCell, Iterations));
			}
		}
		return Best;
	}

	double StepScale(double Scale, float WheelSteps)
	{
		const double Log3Factor = FMath::Loge(WheelFactor) / FMath::Loge(3.0);
		return FMath::Max(MinScale, Scale - static_cast<double>(WheelSteps) * Log3Factor);
	}
}
