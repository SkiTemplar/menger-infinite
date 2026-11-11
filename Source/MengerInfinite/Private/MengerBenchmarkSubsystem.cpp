#include "MengerBenchmarkSubsystem.h"

#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MengerFreeCameraPawn.h"
#include "MengerInfinite.h"
#include "UnrealClient.h"

namespace
{
	constexpr int32 WarmupFrames = 90;
	constexpr int32 MeasuredFrames = 180;
	// Frames tras pedir la captura antes de pasar a la siguiente pose: la captura se toma al dibujar.
	constexpr int32 ScreenshotFrames = 5;
	constexpr int32 FramesPerPose = WarmupFrames + MeasuredFrames + ScreenshotFrames;
}

bool UMengerBenchmarkSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && FParse::Param(FCommandLine::Get(), TEXT("MengerBench"));
}

bool UMengerBenchmarkSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game;
}

TStatId UMengerBenchmarkSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMengerBenchmarkSubsystem, STATGROUP_Tickables);
}

void UMengerBenchmarkSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	GEngine->Exec(&InWorld, TEXT("r.VSync 0"));
	GEngine->Exec(&InWorld, TEXT("t.MaxFPS 0"));

	// Wall10, Wall50 y Wall1000 deben dar la misma imagen: es la prueba de que la profundidad no degrada.
	Poses = { Menger::MakeOverviewPose(), Menger::MakeWallPose(1), Menger::MakeWallPose(5), Menger::MakeWallPose(10),
		Menger::MakeWallPose(50), Menger::MakeWallPose(1000) };
	OutputDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Bench")));
	Csv = TEXT("pose,width,height,frames,frame_ms,fps,gpu_ms,gpu_ms_max\n");
	bIsRunning = true;
	UE_LOG(LogMengerInfinite, Display, TEXT("MengerBench: %d poses, salida en %s"), Poses.Num(), *OutputDir);
}

void UMengerBenchmarkSubsystem::HoldCurrentPose() const
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	AMengerFreeCameraPawn* Pawn = PlayerController ? PlayerController->GetPawn<AMengerFreeCameraPawn>() : nullptr;
	if (Pawn)
	{
		const Menger::FCameraPose& Pose = Poses[PoseIndex];
		Pawn->bShowDebugText = false;
		Pawn->SetCameraPose(Pose);
	}
}

void UMengerBenchmarkSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsRunning)
	{
		return;
	}

	HoldCurrentPose();

	if (FrameInPose == 0)
	{
		Current = { Poses[PoseIndex].Name };
	}

	if (FrameInPose >= WarmupFrames && FrameInPose < WarmupFrames + MeasuredFrames)
	{
		const double GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		++Current.Frames;
		Current.FrameMsSum += DeltaTime * 1000.0;
		Current.GpuMsSum += GpuMs;
		Current.GpuMsMax = FMath::Max(Current.GpuMsMax, GpuMs);
	}
	else if (FrameInPose == WarmupFrames + MeasuredFrames)
	{
		RecordCurrentPose();
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(OutputDir, Current.Name + TEXT(".png")), false, false);
	}

	if (++FrameInPose < FramesPerPose)
	{
		return;
	}

	FrameInPose = 0;
	if (++PoseIndex >= Poses.Num())
	{
		bIsRunning = false;
		FPlatformMisc::RequestExit(false, TEXT("MengerBench"));
	}
}

void UMengerBenchmarkSubsystem::RecordCurrentPose()
{
	FIntPoint ViewportSize = FIntPoint::ZeroValue;
	if (GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();
	}

	const double Frames = FMath::Max(1, Current.Frames);
	const double FrameMs = Current.FrameMsSum / Frames;
	const double GpuMs = Current.GpuMsSum / Frames;
	const FString Line = FString::Printf(TEXT("%s,%d,%d,%d,%.3f,%.1f,%.3f,%.3f"), *Current.Name,
		ViewportSize.X, ViewportSize.Y, Current.Frames, FrameMs, 1000.0 / FrameMs, GpuMs, Current.GpuMsMax);
	UE_LOG(LogMengerInfinite, Display, TEXT("MengerBench: %s"), *Line);

	// Se reescribe tras cada pose para no perder datos si el juego se cierra antes de terminar.
	Csv += Line + TEXT("\n");
	const FString CsvPath = FPaths::Combine(OutputDir, TEXT("bench.csv"));
	if (!FFileHelper::SaveStringToFile(Csv, *CsvPath))
	{
		UE_LOG(LogMengerInfinite, Error, TEXT("MengerBench: no se pudo escribir %s"), *CsvPath);
	}
}
