#include "MengerFreeCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "MengerMath.h"
#include "MengerRenderSubsystem.h"
#include "MengerViewExtension.h"

namespace
{
	// FOV horizontal que a 16:9 da el FOV vertical del proyecto OpenGL (2·atan(0,5) ≈ 53,13°).
	constexpr float HorizontalFov = 83.97f;
	constexpr double MaxPitch = 89.0;

	UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = ValueType;
		return Action;
	}

	/** La tecla da (1, 0, 0); el swizzle lleva ese 1 al eje de Move (X adelante, Y derecha, Z arriba). */
	void MapMoveKey(UInputMappingContext* Context, UInputAction* Action, const FKey& Key,
		TOptional<EInputAxisSwizzle> Swizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (Swizzle.IsSet())
		{
			UInputModifierSwizzleAxis* Modifier = NewObject<UInputModifierSwizzleAxis>(Context);
			Modifier->Order = Swizzle.GetValue();
			Mapping.Modifiers.Add(Modifier);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
	}
}

AMengerFreeCameraPawn::AMengerFreeCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetFieldOfView(HorizontalFov);
	Camera->bConstrainAspectRatio = false;
	RootComponent = Camera;

	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = true;
}

void AMengerFreeCameraPawn::CreateInputObjects()
{
	if (MappingContext)
	{
		return;
	}

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("FreeCameraContext"));
	MoveAction = MakeAction(this, TEXT("Move"), EInputActionValueType::Axis3D);
	LookAction = MakeAction(this, TEXT("Look"), EInputActionValueType::Axis2D);
	ScaleStepAction = MakeAction(this, TEXT("ScaleStep"), EInputActionValueType::Axis1D);
	TurboAction = MakeAction(this, TEXT("Turbo"), EInputActionValueType::Boolean);

	MapMoveKey(MappingContext, MoveAction, EKeys::W, {}, false);
	MapMoveKey(MappingContext, MoveAction, EKeys::S, {}, true);
	MapMoveKey(MappingContext, MoveAction, EKeys::D, EInputAxisSwizzle::YXZ, false);
	MapMoveKey(MappingContext, MoveAction, EKeys::A, EInputAxisSwizzle::YXZ, true);
	MapMoveKey(MappingContext, MoveAction, EKeys::E, EInputAxisSwizzle::ZYX, false);
	MapMoveKey(MappingContext, MoveAction, EKeys::Q, EInputAxisSwizzle::ZYX, true);

	MappingContext->MapKey(LookAction, EKeys::Mouse2D);
	MappingContext->MapKey(ScaleStepAction, EKeys::MouseWheelAxis);
	MappingContext->MapKey(TurboAction, EKeys::LeftShift);
}

void AMengerFreeCameraPawn::PawnClientRestart()
{
	Super::PawnClientRestart();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!InputSubsystem)
	{
		return;
	}

	CreateInputObjects();
	InputSubsystem->ClearAllMappings();
	InputSubsystem->AddMappingContext(MappingContext, 0);
}

void AMengerFreeCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	CreateInputObjects();
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMengerFreeCameraPawn::OnMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMengerFreeCameraPawn::OnLook);
	Input->BindAction(ScaleStepAction, ETriggerEvent::Triggered, this, &AMengerFreeCameraPawn::OnScaleStep);
	Input->BindAction(TurboAction, ETriggerEvent::Started, this, &AMengerFreeCameraPawn::OnTurbo);
	Input->BindAction(TurboAction, ETriggerEvent::Completed, this, &AMengerFreeCameraPawn::OnTurbo);
}

void AMengerFreeCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	SetCameraPose(Menger::MakeOverviewPose());
}

void AMengerFreeCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PushFrameToRenderer();

	if (bShowDebugText && GEngine)
	{
		const double Distance = Menger::DistanceToSponge(CameraState.Position, Menger::CameraDistanceIterations);
		const FString Text = FString::Printf(TEXT("Nivel %d | escala %.2f (rueda %.2f) | pared a %.3g u. de vista"),
			CameraState.Position.GetLevel(), CameraState.ViewScale, CameraState.UserScale,
			Distance / Menger::ViewToLocal(CameraState));
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.0f, FColor::White, Text);
	}
}

void AMengerFreeCameraPawn::SetCameraPose(const Menger::FCameraPose& Pose)
{
	ApplyCameraState(Menger::MakeCameraState(Pose.Position, Pose.Scale));
	SetActorRotation(Pose.Rotation);
	if (AController* OwnController = GetController())
	{
		OwnController->SetControlRotation(Pose.Rotation);
	}
}

void AMengerFreeCameraPawn::ApplyCameraState(const Menger::FCameraState& NewState)
{
	CameraState = NewState;
	PushFrameToRenderer();
}

void AMengerFreeCameraPawn::PushFrameToRenderer() const
{
	UMengerRenderSubsystem* RenderSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMengerRenderSubsystem>() : nullptr;
	if (!RenderSubsystem)
	{
		return;
	}

	static_assert(Menger::MaxRenderFrames == FMengerFrameParams::MaxFrames, "La cascada del juego y la del render deben coincidir");
	const TArray<Menger::FRenderFrame> Cascade = Menger::MakeRenderCascade(CameraState.Position);
	const double ViewToLocal = Menger::ViewToLocal(CameraState);

	FMengerFrameParams Params;
	Params.NumFrames = Cascade.Num();
	for (int32 Index = 0; Index < Cascade.Num(); ++Index)
	{
		const Menger::FRenderFrame& Frame = Cascade[Index];
		FMengerFrame& Out = Params.Frames[Index];
		Out.CameraInFrame = FVector3f(Frame.CameraInFrame);
		Out.ViewToFrame = static_cast<float>(ViewToLocal * Frame.LocalToFrame);
		Out.OccupancyMask = Frame.OccupancyMask;
		Out.bIsTiled = Frame.bIsTiled;
	}
	RenderSubsystem->SetFrameParams(Params);
}

void AMengerFreeCameraPawn::OnMove(const FInputActionValue& Value)
{
	const FVector Direction = Value.Get<FVector>().GetClampedToMaxSize(1.0);
	const double Speed = ViewSpeed * (bIsTurbo ? TurboMultiplier : 1.0);
	const FVector3d DeltaView = GetControlRotation().RotateVector(Direction) * Speed * GetWorld()->GetDeltaSeconds();
	ApplyCameraState(Menger::MoveCamera(CameraState, DeltaView));
}

void AMengerFreeCameraPawn::OnLook(const FInputActionValue& Value)
{
	AController* OwnController = GetController();
	if (!OwnController)
	{
		return;
	}

	const FVector2D Delta = Value.Get<FVector2D>() * LookSensitivity;
	FRotator Rotation = OwnController->GetControlRotation().GetNormalized();
	Rotation.Yaw += Delta.X;
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Delta.Y, -MaxPitch, MaxPitch);
	OwnController->SetControlRotation(Rotation);
}

void AMengerFreeCameraPawn::OnScaleStep(const FInputActionValue& Value)
{
	ApplyCameraState(Menger::StepCameraScale(CameraState, Value.Get<float>()));
}

void AMengerFreeCameraPawn::OnTurbo(const FInputActionValue& Value)
{
	bIsTurbo = Value.Get<bool>();
}
