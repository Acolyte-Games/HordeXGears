// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "CoralisCharacterBase.generated.h"

class UCoralisHealthComponent;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoralisCharacterDiedSignature, AController*, Killer);

/**
 * Shared body for players and enemies: movement and its replicated state, health and death.
 * Players add camera and input in ACoralisPlayerCharacter; enemy Blueprints can parent straight to this.
 * Health, hit zones, weapon and aggro live in components; this class only decides what running out of health does.
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

	UFUNCTION(BlueprintPure, Category = "Coralis|Health")
	UCoralisHealthComponent* GetHealthComponent() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Coralis|Health")
	bool IsDead() const { return bDead; }

	/** Server only, once, when this character dies. Wave counting and similar rules hang here. */
	UPROPERTY(BlueprintAssignable, Category = "Coralis|Death")
	FCoralisCharacterDiedSignature OnDied;

	/** The body goes limp. Off = it just stops where it is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Death")
	bool bRagdollOnDeath = true;

	/** Collision profile of the limp body. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Death")
	FName RagdollProfileName = TEXT("Ragdoll");

	/** How long the body stays before it is removed. 0 = it stays. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Death", meta = (ClampMin = "0", Units = "Seconds"))
	float CorpseLifeSpan = 0.f;

	// --- Stance side ---

	/** Which hand holds the weapon. Call it where the character is controlled; the server and other players follow. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Stance")
	void SetLeftHanded(bool bNewLeftHanded);

	/** True while the weapon is held on the left side: the armed animation is mirrored and shots start on the left. */
	UFUNCTION(BlueprintPure, Category = "Coralis|Stance")
	bool IsLeftHanded() const { return bLeftHanded; }

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
	virtual void BeginPlay() override;

	/** Server: runs once when health reaches zero, after the body has stopped. */
	virtual void HandleDeath(AController* Killer) {}

	/** Every machine: stops the character acting and plays its death. */
	virtual void ApplyDeath();

	/** Cosmetic hook on every machine when the character dies. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Coralis|Death", meta = (DisplayName = "On Death Cosmetics"))
	void ReceiveDeathCosmetics();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Health")
	TObjectPtr<UCoralisHealthComponent> Health;

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

	/** The owner decides this itself, so it is not sent back to them. */
	UPROPERTY(EditAnywhere, Replicated, Category = "Coralis|Stance")
	bool bLeftHanded = false;

	UFUNCTION(Server, Reliable)
	void ServerSetLeftHanded(bool bNewLeftHanded);

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	bool bDead = false;

	UFUNCTION()
	void OnRep_Dead();

	UFUNCTION()
	void HandleHealthDepleted(AController* Killer);
};
