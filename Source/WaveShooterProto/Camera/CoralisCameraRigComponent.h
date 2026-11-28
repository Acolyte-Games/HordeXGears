// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CoralisCameraSettings.h"
#include "Components/ActorComponent.h"
#include "CoralisCameraRigComponent.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Over-the-shoulder camera rig. Each frame it picks a profile from the owner's state tags (aim, sprint, crouch,
 * later cover), blends the spring arm and camera toward it, mirrors the shoulder side and adds the sprint bob.
 * Local-only and cosmetic: nothing here is replicated.
 */
UCLASS(ClassGroup = (Coralis), meta = (BlueprintSpawnableComponent))
class WAVESHOOTERPROTO_API UCoralisCameraRigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoralisCameraRigComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis")
	TObjectPtr<UCoralisCameraSettings> Settings;

	UFUNCTION(BlueprintCallable, Category = "Coralis|Camera")
	void ToggleShoulder();

	/**
	 * Swings the view yaw toward the character's facing by the profile's HeadingFollowSpeed.
	 * Called by the owning character's tick, which runs after the controller applied look input and before the
	 * spring arm updates, so the view gets one consistent rotation per frame.
	 */
	void ApplyHeadingFollow(float DeltaTime);

	/** Tell the rig the player is steering the camera; heading follow backs off for a moment. */
	void NotifyLookInput();

	UFUNCTION(BlueprintCallable, Category = "Coralis|Camera")
	void SetRightShoulder(bool bRight);

	UFUNCTION(BlueprintPure, Category = "Coralis|Camera")
	bool IsOnRightShoulder() const { return bRightShoulder; }

	/** Look input multiplier for the current state (blended). */
	UFUNCTION(BlueprintPure, Category = "Coralis|Camera")
	float GetLookSensitivityScale() const { return Current.LookSensitivityScale; }

	/** The blended pose applied this frame (for debugging and probes). */
	UFUNCTION(BlueprintPure, Category = "Coralis|Camera")
	FCoralisCameraProfile GetCurrentPose() const { return Current; }

	/** -1 (left) .. 1 (right), blended. */
	UFUNCTION(BlueprintPure, Category = "Coralis|Camera")
	float GetShoulderBlend() const { return ShoulderBlend; }

	/** Applies pitch limits to the local player's camera manager. Called on possession. */
	void ApplyViewLimits();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	const UCoralisCameraSettings* GetSettings() const;
	void SnapToTarget();
	void ApplyToComponents(float GroundSpeed);

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> Boom;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	FCoralisCameraProfile Current;
	FVector CameraBaseLocation = FVector::ZeroVector;
	bool bRightShoulder = true;
	float ShoulderBlend = 1.f;
	float BobPhase = 0.f;
	bool bInitialized = false;

	/** True while the active profile asks for no lag; lag switches off once the blend has reached NoLagBlendSpeed. */
	bool bTargetHasNoLag = false;

	/** World time of the last look input (for the heading follow pause). */
	double LastLookInputTime = -1.0e9;

	/** Profile lag speed with 0 ("no lag") replaced by the settings' stiff stand-in, so blends never pass through near-zero lag. */
	float GetBlendableLagSpeed(const FCoralisCameraProfile& Profile) const;
};
