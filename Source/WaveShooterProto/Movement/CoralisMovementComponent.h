// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "CoralisMovementComponent.generated.h"

class UCoralisMovementSettings;
struct FCoralisTurnSettings;

/** Custom movement modes. Cover will live here as its own mode. */
UENUM(BlueprintType)
enum class ECoralisCustomMovementMode : uint8
{
	None = 0 UMETA(Hidden),
	Cover = 1 UMETA(DisplayName = "Cover"),
};

/**
 * Character movement with predicted sprint (roadie run), aim walk, crouch and dodge roll.
 *
 * Intent (wants to sprint / aim / dodge) travels in the saved-move flags, and the dodge and recovery timers are saved
 * per move, so the owning client predicts and the server replays the same simulation without corrections.
 * Simulated proxies don't run this simulation; they read the replicated state from ACoralisCharacterBase.
 */
UCLASS(ClassGroup = (Coralis), meta = (BlueprintSpawnableComponent))
class WAVESHOOTERPROTO_API UCoralisMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_Coralis;

public:
	UCoralisMovementComponent();

	/** Movement tunables. If unset, the class defaults of UCoralisMovementSettings are used (with a warning). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis")
	TObjectPtr<UCoralisMovementSettings> Settings;

	// --- Intent (call on the owning client, or on the server for AI) ---

	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void SetWantsToSprint(bool bWants);

	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void SetWantsToAim(bool bWants);

	/**
	 * Requests a dodge roll on the next move. The roll goes in the current input direction, or forward without input.
	 * On the server (AI), DirectionOverride picks the direction instead when non-zero.
	 */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void RequestDodge(FVector DirectionOverride = FVector::ZeroVector);

	// --- State ---

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	bool IsSprinting() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	bool IsAiming() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	bool IsDodging() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	bool IsRecovering() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	bool WantsToSprint() const { return bWantsToSprint; }

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	bool WantsToAim() const { return bWantsToAim; }

	/** 0..1 through the current roll, 0 when not rolling. */
	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	float GetDodgeAlpha() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	FVector GetDodgeDirection() const;

	/** Weapons ask this before firing: no shooting while roadie running or rolling. */
	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	bool CanUseWeapon() const;

	/** Current state as tags (Coralis.Movement.*). Camera, animation and UI read this instead of individual bools. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void GetMovementStateTags(FGameplayTagContainer& OutTags) const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	const UCoralisMovementSettings* GetSettings() const;

	/** Re-applies the settings asset to the base movement properties (call after swapping Settings at runtime). */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void ApplySettings();

	// --- UCharacterMovementComponent ---
	virtual void InitializeComponent() override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void PhysicsRotation(float DeltaTime) override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual bool CanAttemptJump() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	virtual bool CanStartSprint() const;
	virtual bool CanStartDodge() const;
	virtual void StartDodge();
	virtual void EndDodge();

	/**
	 * Picks the yaw the character turns toward this frame. Returns false to keep the current facing.
	 * bOutLocked = locked view: snap within LockCatchUpAngle, otherwise turn at LockCatchUpRate.
	 */
	virtual bool ComputeDesiredYaw(float& OutYaw, const FCoralisTurnSettings*& OutTurn, float& OutFixedRate, bool& bOutLocked) const;

	bool HasMoveInput() const;

	/** Weighted ground velocity (see UCoralisMovementSettings "Weight"). Returns false to fall back to the engine model. */
	bool CalcWeightedVelocity(float DeltaTime, bool bFluid);

	/** True while the facing turns at a limited rate this move (sprint, roll, catch-up), so moves must not be combined. */
	bool IsFacingRateLimited() const;

	/** True while weighted movement is still changing velocity toward the stick (moves must not be combined then). */
	bool IsWeightedVelocityConverging() const;
	bool IsSimulatedProxyOwner() const;

	// Intent (saved-move flags).
	bool bWantsToSprint = false;
	bool bWantsToAim = false;
	bool bWantsToDodge = false;

	// Simulation state (saved per move so replays match).
	bool bIsSprinting = false;
	float DodgeTimeRemaining = 0.f;
	float DodgeElapsed = 0.f;
	float RecoveryTimeRemaining = 0.f;

	/** Standing: legs are catching up with the camera (facing state only; position prediction doesn't depend on it). */
	mutable bool bIdleCatchingUp = false;
	FVector DodgeDirection = FVector::ForwardVector;

	/** Server-only direction for AI-requested rolls. */
	FVector PendingDodgeDirectionOverride = FVector::ZeroVector;
};
