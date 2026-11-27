// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "CoralisCharacterBase.generated.h"

class UCoralisMovementComponent;
class UCoralisWeaponComponent;

/** Movement state other machines need for animation. Owning client and server run the real simulation instead. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisReplicatedMoveState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Coralis")
	bool bSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis")
	bool bAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis")
	bool bDodging = false;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis")
	bool bRecovering = false;

	/** Roll direction as a compressed yaw. */
	UPROPERTY()
	uint8 DodgeYaw = 0;

	/** Increments on every roll so a quick second roll still triggers on proxies. */
	UPROPERTY()
	uint8 DodgeCount = 0;

	/** Where the player looks relative to the body (yaw), in 2-degree steps, so other players see the torso twist. */
	UPROPERTY()
	int8 AimYawOffsetHalfDeg = 0;

	FVector GetDodgeDirection() const;
	void SetDodgeDirection(const FVector& Dir);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoralisDodgeStartedSignature, FVector, Direction);

/**
 * Shared body for players and enemies: movement and its replicated state.
 * Players add camera and input in ACoralisPlayerCharacter; enemy Blueprints can parent straight to this.
 * Health, hit zones, weapon and aggro come in as components, not as code in this class.
 * The weapon component is always there; it can't shoot until it is given weapon settings.
 */
UCLASS(Abstract, Blueprintable)
class WAVESHOOTERPROTO_API ACoralisCharacterBase : public ACharacter, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	ACoralisCharacterBase(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Coralis|Movement")
	UCoralisMovementComponent* GetCoralisMovement() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Weapon")
	UCoralisWeaponComponent* GetWeapon() const { return Weapon; }

	// --- Movement verbs (players call these from input, AI from its tasks) ---

	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void SetSprinting(bool bWantsToSprint);

	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void SetAiming(bool bWantsToAim);

	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void ToggleCrouch();

	UFUNCTION(BlueprintCallable, Category = "Coralis|Movement")
	void Dodge(FVector DirectionOverride = FVector::ZeroVector);

	/** Current state tags (movement now, more systems later). */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	const FCoralisReplicatedMoveState& GetReplicatedMoveState() const { return ReplicatedMoveState; }

	/** Look direction minus body facing (yaw, deg). From the controller where there is one, else from replication. */
	UFUNCTION(BlueprintPure, Category = "Coralis")
	float GetAimYawOffset() const;

	/** Called by the movement component when a roll really starts (not on move replays), and on proxies from replication. */
	void NotifyDodgeStarted(const FVector& Direction);

	UPROPERTY(BlueprintAssignable, Category = "Coralis|Movement")
	FCoralisDodgeStartedSignature OnDodgeStarted;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Weapon")
	TObjectPtr<UCoralisWeaponComponent> Weapon;

	/** Plays the roll animation from the movement settings. Override in Blueprint to replace. */
	UFUNCTION(BlueprintNativeEvent, Category = "Coralis|Movement")
	void PlayDodgeCosmetics(FVector Direction);

	UFUNCTION()
	void OnRep_ReplicatedMoveState(const FCoralisReplicatedMoveState& OldState);

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedMoveState)
	FCoralisReplicatedMoveState ReplicatedMoveState;

private:
	void UpdateReplicatedMoveState();
};
