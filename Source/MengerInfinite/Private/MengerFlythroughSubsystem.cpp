#include "MengerFlythroughSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MengerFreeCameraPawn.h"
#include "MengerInfinite.h"
#include "UnrealClient.h"

namespace
{
	// Frames tras la última captura antes de salir, para que se procese la del último frame.
	constexpr int32 TailFrames = 5;
}

bool UMengerFlythroughSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && FParse::Param(FCommandLine::Get(), TEXT("MengerFlythrough"));
}

bool UMengerFlythroughSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game;
}

TStatId UMengerFlythroughSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMengerFlythroughSubsystem, STATGROUP_Tickables);
}

void UMengerFlythroughSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const TCHAR* CommandLine = FCommandLine::Get();
	FParse::Value(CommandLine, TEXT("FlyFrames="), Frames);
	FParse::Value(CommandLine, TEXT("FlyLevels="), Params.Levels);
	FParse::Value(CommandLine, TEXT("FlyStart="), Params.StartScale);
	FParse::Value(CommandLine, TEXT("FlyDistance="), Params.Distance);
	FParse::Value(CommandLine, TEXT("FlyYaw="), Params.YawAmplitude);
	FParse::Value(CommandLine, TEXT("FlyPitch="), Params.PitchAmplitude);
	FParse::Value(CommandLine, TEXT("FlyWarmup="), WarmupFrames);
	FParse::Value(CommandLine, TEXT("FlyEvery="), FrameStride);
	Frames = FMath::Max(1, Frames);
	WarmupFrames = FMath::Max(0, WarmupFrames);
	FrameStride = FMath::Max(1, FrameStride);

	GEngine->Exec(&InWorld, TEXT("r.VSync 0"));
	GEngine->Exec(&InWorld, TEXT("t.MaxFPS 0"));

	// Con -RenderOffscreen el motor no aplica -ResX/-ResY (se queda en r.SetRes, 1280x720 por defecto):
	// se pide aquí y el calentamiento deja frames para que el viewport cambie de tamaño antes de capturar.
	FParse::Value(CommandLine, TEXT("ResX="), RequestedSize.X);
	FParse::Value(CommandLine, TEXT("ResY="), RequestedSize.Y);
	if (RequestedSize.X > 0 && RequestedSize.Y > 0)
	{
		GEngine->Exec(&InWorld, *FString::Printf(TEXT("r.SetRes %dx%dw"), RequestedSize.X, RequestedSize.Y));
	}

	OutputDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Flythrough")));
	IFileManager::Get().DeleteDirectory(*OutputDir, false, true);
	IFileManager::Get().MakeDirectory(*OutputDir, true);

	// Con un delegado enlazado, el viewport entrega los píxeles en lugar de escribir el fichero en la cola
	// asíncrona: se guardan aquí, en orden y sin depender de que la cola termine antes de salir.
	ScreenshotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &UMengerFlythroughSubsystem::OnScreenshotCaptured);
	bIsRunning = true;
	UE_LOG(LogMengerInfinite, Display, TEXT("MengerFlythrough: %d frames, %d niveles desde la escala %.2f, salida en %s"),
		Frames, Params.Levels, Params.StartScale, *OutputDir);
}

void UMengerFlythroughSubsystem::Deinitialize()
{
	UGameViewportClient::OnScreenshotCaptured().Remove(ScreenshotHandle);
	Super::Deinitialize();
}

void UMengerFlythroughSubsystem::HoldPose(double Phase) const
{
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	AMengerFreeCameraPawn* Pawn = PlayerController ? PlayerController->GetPawn<AMengerFreeCameraPawn>() : nullptr;
	if (Pawn)
	{
		Pawn->bShowDebugText = false;
		Pawn->SetCameraPose(Menger::MakeFlythroughPose(Params, Phase));
	}
}

void UMengerFlythroughSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsRunning)
	{
		return;
	}

	const int32 Capture = TickIndex - WarmupFrames;
	const int32 Frame = FMath::Max(0, Capture) * FrameStride;
	++TickIndex;

	if (Capture == 0 && !HasRequestedViewportSize())
	{
		bIsRunning = false;
		FPlatformMisc::RequestExit(false, TEXT("MengerFlythrough"));
		return;
	}

	if (Frame <= Frames)
	{
		// El subsistema hace tick antes de que el PlayerController actualice la cámara: la pose y la
		// orientación que se fijan aquí son las del frame que se dibuja a continuación.
		HoldPose(static_cast<double>(Frame) / Frames);
		if (Capture >= 0)
		{
			PendingFile = FPaths::Combine(OutputDir, FString::Printf(TEXT("frame_%05d.png"), Frame));
			FScreenshotRequest::RequestScreenshot(TEXT("Flythrough"), false, false);
			++Requested;
		}
		return;
	}

	if (Frame > Frames + TailFrames * FrameStride)
	{
		bIsRunning = false;
		UE_LOG(LogMengerInfinite, Display, TEXT("MengerFlythrough: %d de %d capturas guardadas"), Saved, Requested);
		if (Saved != Requested)
		{
			UE_LOG(LogMengerInfinite, Error, TEXT("MengerFlythrough: faltan %d capturas"), Requested - Saved);
		}
		FPlatformMisc::RequestExit(false, TEXT("MengerFlythrough"));
	}
}

bool UMengerFlythroughSubsystem::HasRequestedViewportSize() const
{
	if (RequestedSize.X <= 0 || RequestedSize.Y <= 0)
	{
		return true;
	}

	FIntPoint ViewportSize = FIntPoint::ZeroValue;
	if (GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();
	}
	if (ViewportSize == RequestedSize)
	{
		return true;
	}

	UE_LOG(LogMengerInfinite, Error, TEXT("MengerFlythrough: viewport de %dx%d en lugar de %dx%d; no se captura nada"),
		ViewportSize.X, ViewportSize.Y, RequestedSize.X, RequestedSize.Y);
	return false;
}

void UMengerFlythroughSubsystem::OnScreenshotCaptured(int32 Width, int32 Height, const TArray<FColor>& Colors)
{
	if (PendingFile.IsEmpty())
	{
		return;
	}

	TArray64<uint8> Png;
	FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(Colors.GetData(), Colors.Num()), Png);
	if (FFileHelper::SaveArrayToFile(Png, *PendingFile))
	{
		++Saved;
	}
	else
	{
		UE_LOG(LogMengerInfinite, Error, TEXT("MengerFlythrough: no se pudo escribir %s"), *PendingFile);
	}
	PendingFile.Reset();
}
