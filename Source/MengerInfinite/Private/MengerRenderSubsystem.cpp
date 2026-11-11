#include "MengerRenderSubsystem.h"

#include "SceneViewExtension.h"

void UMengerRenderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ViewExtension = FSceneViewExtensions::NewExtension<FMengerViewExtension>(GetWorld());
}

void UMengerRenderSubsystem::Deinitialize()
{
	ViewExtension.Reset();
	Super::Deinitialize();
}

void UMengerRenderSubsystem::SetFrameParams(const FMengerFrameParams& Params)
{
	if (ViewExtension)
	{
		ViewExtension->SetFrameParams(Params);
	}
}

bool UMengerRenderSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
