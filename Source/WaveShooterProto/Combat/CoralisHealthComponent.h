// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/CoralisHitZoneSettings.h"
#include "CoralisHealthComponent.generated.h"

class UDamageType;
class USkeletalMeshComponent;

/** The latest damage taken, replicated so every machine can react to it. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisDamageEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Health")
	float Amount = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Health")
	ECoralisHitZone Zone = ECoralisHitZone::Body;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Health")
	FVector_NetQuantize Location;

	/** Increments on every damage, so two equal hits in a row still replicate. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Health")
	uint8 Count = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoralisHealthChangedSignature, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoralisDamagedSignature, const FCoralisDamageEvent&, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoralisHealthDepletedSignature, AController*, InstigatorController);

/**
 * Replicated health for anything that can be shot: players, enemies, buildings, test targets.
 * The server changes health; everyone else reads it. What happens at zero is up to the owner (bind OnDepleted).
 * Takes Coralis weapon hits (with hit zones) and ordinary engine damage (Apply Damage in Blueprint).
 */
UCLASS(Blueprintable, ClassGroup = (Coralis), meta = (BlueprintSpawnableComponent))
class WAVESHOOTERPROTO_API UCoralisHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoralisHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** Bone to hit zone mapping and the zone damage multipliers. Unset = every hit is a plain body hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Health")
	TObjectPtr<UCoralisHitZoneSettings> HitZones;

	/**
	 * With hit zones and a skeletal mesh: at start, shots pass through the owner's capsule and hit the mesh's physics
	 * asset bodies instead, and the server keeps the bones posed so they are where players see them.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Health")
	bool bTraceAgainstMeshBodies = true;

	UPROPERTY(BlueprintAssignable, Category = "Coralis|Health")
	FCoralisHealthChangedSignature OnHealthChanged;

	/** Every machine, each time damage lands. */
	UPROPERTY(BlueprintAssignable, Category = "Coralis|Health")
	FCoralisDamagedSignature OnDamaged;

	/** Server only, once, when health reaches zero. */
	UPROPERTY(BlueprintAssignable, Category = "Coralis|Health")
	FCoralisHealthDepletedSignature OnDepleted;

	UFUNCTION(BlueprintPure, Category = "Coralis|Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Coralis|Health")
	bool IsDepleted() const { return Health <= 0.f; }

	UFUNCTION(BlueprintPure, Category = "Coralis|Health")
	const FCoralisDamageEvent& GetLastDamage() const { return LastDamage; }

	/**
	 * Server only. Applies BaseDamage scaled by the zone that Hit landed on.
	 * Returns the damage actually taken (0 when already depleted).
	 */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Health")
	float ApplyHit(float BaseDamage, const FHitResult& Hit, AController* InstigatorController, ECoralisHitZone& OutZone);

	/** Server only. Back to full health. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Health")
	void ResetHealth();

	/** Which zone a hit belongs to and its damage multiplier. */
	UFUNCTION(BlueprintPure, Category = "Coralis|Health")
	ECoralisHitZone ResolveHitZone(const FHitResult& Hit, float& OutMultiplier) const;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_LastDamage)
	FCoralisDamageEvent LastDamage;

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_LastDamage();

	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	float TakeHealth(float Amount, ECoralisHitZone Zone, const FVector& Location, AController* InstigatorController);
	void SetUpMeshTraces();
};
