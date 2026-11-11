#include "MengerPath.h"

#include "MengerMath.h"

namespace Menger
{
	namespace
	{
		int32 FloorDiv(int32 Value, int32 Divisor)
		{
			const int32 Quotient = Value / Divisor;
			return (Value % Divisor != 0 && (Value < 0) != (Divisor < 0)) ? Quotient - 1 : Quotient;
		}

		int32 DigitFor(double Coordinate)
		{
			if (Coordinate > 1.0 / 3.0) { return 1; }
			if (Coordinate < -1.0 / 3.0) { return -1; }
			return 0;
		}

		bool IsRootCellSolid(const FIntVector& RootCell)
		{
			return RootCell.X % 2 == 0 && RootCell.Y % 2 == 0 && RootCell.Z % 2 == 0;
		}

		/** Suma Steps celdas en el eje Axis al último nivel, con acarreo en base 3 equilibrada. */
		void AddCellsOnAxis(FFractalPosition& Position, int32 Axis, int32 Steps)
		{
			int32 Carry = Steps;
			for (int32 Index = Position.Digits.Num() - 1; Index >= 0 && Carry != 0; --Index)
			{
				const int32 Value = Position.Digits[Index][Axis] + Carry + 1;
				Carry = FloorDiv(Value, 3);
				Position.Digits[Index][Axis] = Value - 3 * Carry - 1;
			}
			Position.RootCell[Axis] += Carry;
		}

		FVector3d AscendPoint(const FVector3d& Point, const FIntVector& Digit)
		{
			return (Point + 2.0 * FVector3d(Digit)) / 3.0;
		}

		int32 FirstEmptyDigitIndex(const TArray<FIntVector>& Digits, int32 Count)
		{
			for (int32 Index = 0; Index < Count; ++Index)
			{
				if (!IsSolidDigit(Digits[Index]))
				{
					return Index;
				}
			}
			return Count;
		}

		/**
		 * Solidez de la celda vecina (Offset) de la celda de nivel Level sin copiar la ruta: solo se
		 * recorren los niveles que toca el acarreo, casi siempre uno o dos. FirstEmpty es el índice
		 * del primer dígito hueco de la ruta (o Level si no hay ninguno).
		 */
		bool IsNeighborSolid(const FFractalPosition& Position, int32 Level, const FIntVector& Offset, int32 FirstEmpty)
		{
			FIntVector Carry = Offset;
			int32 Index = Level - 1;
			for (; Index >= 0 && Carry != FIntVector::ZeroValue; --Index)
			{
				FIntVector Digit = Position.Digits[Index];
				for (int32 Axis = 0; Axis < 3; ++Axis)
				{
					const int32 Value = Digit[Axis] + Carry[Axis] + 1;
					Carry[Axis] = FloorDiv(Value, 3);
					Digit[Axis] = Value - 3 * Carry[Axis] - 1;
				}
				if (!IsSolidDigit(Digit))
				{
					return false;
				}
			}
			// Los dígitos 0..Index no los toca el acarreo.
			return FirstEmpty > Index && IsRootCellSolid(Position.RootCell + Carry);
		}
	}

	bool IsSolidDigit(const FIntVector& Digit)
	{
		const int32 CentralCount = (Digit.X == 0) + (Digit.Y == 0) + (Digit.Z == 0);
		return CentralCount <= 1;
	}

	FFractalPosition FromRootPosition(const FVector3d& RootPosition, int32 Level)
	{
		FFractalPosition Position;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const int32 Cell = FMath::FloorToInt32((RootPosition[Axis] + 1.0) / 2.0);
			Position.RootCell[Axis] = Cell;
			Position.Local[Axis] = RootPosition[Axis] - 2.0 * Cell;
		}
		return WithLevel(Position, Level);
	}

	FVector3d ToRootPosition(const FFractalPosition& Position)
	{
		FVector3d Point = Position.Local;
		for (int32 Index = Position.Digits.Num() - 1; Index >= 0; --Index)
		{
			Point = AscendPoint(Point, Position.Digits[Index]);
		}
		return Point + 2.0 * FVector3d(Position.RootCell);
	}

	FFractalPosition Descend(const FFractalPosition& Position)
	{
		FFractalPosition Result = Position;
		const FIntVector Digit(DigitFor(Position.Local.X), DigitFor(Position.Local.Y), DigitFor(Position.Local.Z));
		Result.Digits.Add(Digit);
		Result.Local = 3.0 * Position.Local - 2.0 * FVector3d(Digit);
		return Result;
	}

	FFractalPosition Ascend(const FFractalPosition& Position)
	{
		if (Position.Digits.IsEmpty())
		{
			return Position;
		}
		FFractalPosition Result = Position;
		Result.Local = AscendPoint(Position.Local, Result.Digits.Pop(EAllowShrinking::No));
		return Result;
	}

	FFractalPosition WithLevel(const FFractalPosition& Position, int32 Level)
	{
		FFractalPosition Result = Position;
		while (Result.GetLevel() > FMath::Max(Level, 0))
		{
			Result = Ascend(Result);
		}
		while (Result.GetLevel() < Level)
		{
			Result = Descend(Result);
		}
		return Result;
	}

	FFractalPosition MoveLocal(const FFractalPosition& Position, const FVector3d& DeltaLocal)
	{
		FFractalPosition Result = Position;
		Result.Local += DeltaLocal;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const double Coordinate = Result.Local[Axis];
			if (Coordinate >= -1.0 && Coordinate <= 1.0)
			{
				continue;
			}
			const int32 Cells = FMath::FloorToInt32((Coordinate + 1.0) / 2.0);
			Result.Local[Axis] = Coordinate - 2.0 * Cells;
			AddCellsOnAxis(Result, Axis, Cells);
		}
		return Result;
	}

	FFractalPosition OffsetCell(const FFractalPosition& Position, const FIntVector& Offset)
	{
		FFractalPosition Result = Position;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			AddCellsOnAxis(Result, Axis, Offset[Axis]);
		}
		return Result;
	}

	bool IsCellSolid(const FFractalPosition& Position)
	{
		if (!IsRootCellSolid(Position.RootCell))
		{
			return false;
		}
		for (const FIntVector& Digit : Position.Digits)
		{
			if (!IsSolidDigit(Digit))
			{
				return false;
			}
		}
		return true;
	}

	FRenderFrame MakeRenderFrame(const FFractalPosition& Position, int32 Depth)
	{
		FRenderFrame Frame;
		const int32 Level = Position.GetLevel();
		Frame.FrameLevel = FMath::Max(0, Level - Depth);
		Frame.bIsTiled = Frame.FrameLevel == 0;
		Frame.LocalToFrame = FMath::Pow(3.0, -static_cast<double>(Level - Frame.FrameLevel));

		FVector3d Point = Position.Local;
		for (int32 Index = Level - 1; Index >= Frame.FrameLevel; --Index)
		{
			Point = AscendPoint(Point, Position.Digits[Index]);
		}

		if (Frame.bIsTiled)
		{
			// La repetición tiene periodo 4 (dos celdas): basta la paridad de la celda raíz.
			const FIntVector Parity(Position.RootCell.X & 1, Position.RootCell.Y & 1, Position.RootCell.Z & 1);
			Frame.CameraInFrame = Point + 2.0 * FVector3d(Parity);
			return Frame;
		}

		Frame.CameraInFrame = Point;
		const int32 FirstEmpty = FirstEmptyDigitIndex(Position.Digits, Frame.FrameLevel);
		for (int32 Bit = 0; Bit < 27; ++Bit)
		{
			const FIntVector Offset(Bit % 3 - 1, (Bit / 3) % 3 - 1, Bit / 9 - 1);
			if (IsNeighborSolid(Position, Frame.FrameLevel, Offset, FirstEmpty))
			{
				Frame.OccupancyMask |= 1u << Bit;
			}
		}
		return Frame;
	}

	TArray<FRenderFrame> MakeRenderCascade(const FFractalPosition& Position, int32 MaxFrames)
	{
		TArray<FRenderFrame> Frames;
		for (int32 Index = 0; Index < MaxFrames; ++Index)
		{
			Frames.Add(MakeRenderFrame(Position, FrameDepth * (Index + 1)));
			if (Frames.Last().bIsTiled)
			{
				break;
			}
		}
		return Frames;
	}

	double FrameDistance(const FRenderFrame& Frame, const FVector3d& PointInFrame, int32 Iterations)
	{
		if (Frame.bIsTiled)
		{
			return TiledSpongeDistance(PointInFrame, Iterations);
		}
		return BlockSpongeDistance(PointInFrame, Frame.OccupancyMask, Iterations);
	}

	double DistanceToSponge(const FFractalPosition& Position, int32 Iterations)
	{
		const int32 Level = Position.GetLevel();
		for (int32 Depth = FrameDepth; ; Depth += FrameDepth)
		{
			const FRenderFrame Frame = MakeRenderFrame(Position, Depth);
			const int32 FrameIterations = Iterations + Level - Frame.FrameLevel;
			const double Distance = FrameDistance(Frame, Frame.CameraInFrame, FrameIterations);
			// En el bloque, una distancia de 1 unidad de marco o más solo es una cota: hay que mirar más arriba.
			const bool bIsExact = Frame.bIsTiled || Distance < 1.0;
			if (bIsExact || Depth >= MaxSearchLevels)
			{
				return Distance / Frame.LocalToFrame;
			}
		}
	}
}
