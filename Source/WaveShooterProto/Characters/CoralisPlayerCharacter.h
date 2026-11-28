// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CoralisCharacterBase.h"
#include "CoralisPlayerCharacter.generated.h"

class UCameraComponent;
class UCoralisCameraRigComponent;
class UCoralisHealthHUDComponent;
class UCoralisWaveHUDComponent;
class UCoralisInputConfig;
class UCoralisWeaponHUDComponent;
class USpringArmComponent;
struct FInputActionValue;

/** Player body: the shared base plus the over-the-shoulder camera and Enhanced Input. */
UCLASS(Blueprintable)
class WAVESHOOTERPROTO_API ACoralisPlayerCharacter : public ACoralisCharacterBase
{
	GENERATED_BODY()

public:
	ACoralisPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "Coralis|Camera")
	UCoralisCameraRigComponent* GetCameraRig() const { return CameraRig; }

	/**
	 * Test hook for scripted checks: applies this move input (X = right, Y = forward, camera relative) every frame
	 * until cleared with zero. Development builds only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Debug")
	void SetDebugMoveInput(FVector2D Input);

	UFUNCTION(BlueprintPure, Category = "Coralis|Debug")
	FVector2D GetDebugMoveInput() const { return DebugMoveInput; }

	/** Placeholder until the game flow decides this: seconds after death until the player is back. 0 = stays dead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Death", meta = (ClampMin = "0", Units = "Seconds"))
	float RespawnDelay = 6.f;

	/** Server: replaces this body with a fresh one at a player start. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Death")
	void Respawn();

	/** Server: stops the Respawn Delay timer, for a game flow that decides itself when the player comes back. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Death")
	void CancelPendingRespawn();

	/** Swapping the camera shoulder also moves the weapon to that side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coralis|Stance")
	bool bSwitchHandsWithShoulder = true;

	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;

protected:
	virtual void HandleDeath(AController* Killer) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Camera")
	TObjectPtr<UCoralisCameraRigComponent> CameraRig;

	/** Heat bar, crosshair and hit marker of the local player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Weapon")
	TObjectPtr<UCoralisWeaponHUDComponent> WeaponHUD;

	/** Health bar of the local player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Health")
	TObjectPtr<UCoralisHealthHUDComponent> HealthHUD;

	/** Wave number, enemies left and countdown, for the local player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|HUD")
	TObjectPtr<UCoralisWaveHUDComponent> WaveHUD;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Input")
	TObjectPtr<UCoralisInputConfig> InputConfig;

private:
	FTimerHandle RespawnTimer;

	void AddMappingContexts();
	void ApplyMoveInput(const FVector2D& Input);

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnMouseLook(const FInputActionValue& Value);
	void OnSprintStarted(const FInputActionValue& Value);
	void OnSprintCompleted(const FInputActionValue& Value);
	void OnDodge(const FInputActionValue& Value);
	void OnEvadeRunStarted(const FInputActionValue& Value);
	void OnEvadeRunCompleted(const FInputActionValue& Value);
	void OnAimStarted(const FInputActionValue& Value);
	void OnAimCompleted(const FInputActionValue& Value);
	void OnCrouch(const FInputActionValue& Value);
	void OnFireStarted(const FInputActionValue& Value);
	void OnFireCompleted(const FInputActionValue& Value);
	void OnVent(const FInputActionValue& Value);
	void OnShoulderSwap(const FInputActionValue& Value);
	void OnInvertMouseLook(const FInputActionValue& Value);

	void AddLook(float YawDegrees, float PitchDegrees);

	FVector2D LastMoveInput = FVector2D::ZeroVector;
	FVector2D DebugMoveInput = FVector2D::ZeroVector;
	double EvadePressTime = -1.0;
	bool bEvadeHeldIntoSprint = false;
	bool bSprintHeld = false;

	/** Mouse look inversion in use; starts from the input config, the invert key flips it. */
	bool bMouseYInverted = false;

	/**
	 * While sprinting with the stick pulled back past the live steer angle, the stick steers relative to the camera
	 * direction held from that moment, turned only by the player's own look input. The camera's automatic follow then
	 * can't feed back into the run direction (a sprint started backwards keeps running that way while the camera swings
	 * round behind). Otherwise the stick follows the live camera.
	 */
	float SprintInputFrameYaw = 0.f;
	bool bSprintInputFrameLatched = false;
};
