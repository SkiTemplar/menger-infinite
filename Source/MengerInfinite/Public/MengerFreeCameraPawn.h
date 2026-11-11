#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MengerNavigator.h"
#include "MengerFreeCameraPawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Cámara libre dentro del fractal. El actor se queda en el origen de UE (solo aporta orientación y FOV);
 * la posición vive en un Menger::FCameraState renormalizado, sin límite de profundidad.
 *
 * Controles: el ratón orienta, WASD traslada, E/Q suben y bajan, Shift es turbo y la rueda cambia la
 * escala de vista (y con ella la velocidad) en pasos de x1,25.
 */
UCLASS()
class MENGERINFINITE_API AMengerFreeCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AMengerFreeCameraPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	/** Coloca la cámara en una pose (posición exacta, escala y orientación). */
	void SetCameraPose(const Menger::FCameraPose& Pose);

	const Menger::FCameraState& GetCameraState() const { return CameraState; }

	/** Muestra en pantalla nivel, escala y distancia a la pared. */
	UPROPERTY(EditAnywhere, Category = "Menger")
	bool bShowDebugText = true;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Menger")
	TObjectPtr<UCameraComponent> Camera;

	/** Unidades de vista por segundo. */
	UPROPERTY(EditAnywhere, Category = "Menger")
	float ViewSpeed = 1.0f;

	/** Grados por unidad de movimiento del ratón. */
	UPROPERTY(EditAnywhere, Category = "Menger")
	float LookSensitivity = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Menger")
	float TurboMultiplier = 4.0f;

private:
	void CreateInputObjects();
	void ApplyCameraState(const Menger::FCameraState& NewState);
	void PushFrameToRenderer() const;
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnScaleStep(const FInputActionValue& Value);
	void OnTurbo(const FInputActionValue& Value);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ScaleStepAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> TurboAction;

	Menger::FCameraState CameraState;
	bool bIsTurbo = false;
};
