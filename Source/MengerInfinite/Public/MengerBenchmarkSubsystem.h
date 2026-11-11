#pragma once

#include "CoreMinimal.h"
#include "MengerNavigator.h"
#include "Subsystems/WorldSubsystem.h"
#include "MengerBenchmarkSubsystem.generated.h"

/**
 * Benchmark por línea de comandos (-MengerBench): recorre poses fijas, mide el tiempo de frame y de
 * GPU, guarda una captura por pose y un CSV en Saved/Bench y cierra el juego.
 */
UCLASS()
class MENGERINFINITE_API UMengerBenchmarkSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FPoseResult
	{
		FString Name;
		int32 Frames = 0;
		double FrameMsSum = 0.0;
		double GpuMsSum = 0.0;
		double GpuMsMax = 0.0;
	};

	void HoldCurrentPose() const;
	void RecordCurrentPose();

	TArray<Menger::FCameraPose> Poses;
	FPoseResult Current;
	FString Csv;
	FString OutputDir;
	int32 PoseIndex = 0;
	int32 FrameInPose = 0;
	bool bIsRunning = false;
};
