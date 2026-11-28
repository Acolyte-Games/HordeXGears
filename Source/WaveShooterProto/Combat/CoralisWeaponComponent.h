// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/CoralisHeatModel.h"
#include "Combat/CoralisHitZoneSettings.h"
#include "Components/ActorComponent.h"
#include "CoralisWeaponComponent.generated.h"

class UCoralisWeaponComponent;
class UCoralisWeaponSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoralisShotFiredSignature, FVector, Start, FVector, End);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCoralisHitConfirmedSignature, float, Damage, ECoralisHitZone, Zone, bool, bDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoralisVentStartedSignature, bool, bSweetSpot);
DECLARE_MULTICAST_DELEGATE_FourParams(FCoralisServerShotSignature, const UCoralisWeaponComponent& /*Weapon*/,
	const FVector& /*Start*/, const FVector& /*End*/, const AActor* /*HitActor*/);

/**
 * A heat weapon carried by a character.
 *
 * The owning machine runs the heat rules itself so firing and venting respond at once, and sends each shot and vent to
 * the server with the time it happened on its own clock. The server replays them on that clock through the same rules
 * (FCoralisHeatState), so both reach the same heat, jam and sweet spot results regardless of lag. The server alone
 * deals damage. When it refuses an input it sends its state back to the owner.
 * AI uses the same component by calling StartFire / StopFire / Vent on the server.
 */
UCLASS(Blueprintable, ClassGroup = (Coralis), meta = (BlueprintSpawnableComponent))
class WAVESHOOTERPROTO_API UCoralisWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoralisWeaponComponent();

	/** The weapon carried. Unset = this character can't shoot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Weapon")
	TObjectPtr<UCoralisWeaponSettings> Settings;

	// --- Input (call on the owning client, or on the server for AI) ---

	/** Trigger pulled: fires at once if it can and keeps firing while held. A venting or jammed weapon doesn't fire. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Coralis|Weapon")
	void StopFire();

	/** Starts a vent. The weapon can't fire again until the heat is gone. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Weapon")
	void Vent();

	/** Debug: sets heat as a fraction of max, on the server too. Development builds only. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Debug")
	void DebugSetHeat(float Fraction);

	// --- State (owner and server: exact. Others: as last replicated.) ---

	UFUNCTION(BlueprintPure, Category = "Coralis|Weapon")
	FCoralisHeatState GetHeatState() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Weapon")
	float GetHeatFraction() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Weapon")
	ECoralisHeatPhase GetHeatPhase() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Weapon")
	bool IsBuffActive() const;

	/** Where the next vent has to start, as fractions of max heat. */
	UFUNCTION(BlueprintPure, Category = "Coralis|Weapon")
	void GetSweetSpotWindow(float& Start, float& End) const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Weapon")
	float GetBuffTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|Weapon")
	bool IsTriggerHeld() const { return bTriggerHeld; }

	// --- Events ---

	/** Cosmetics: a shot left the weapon. On the owner at once, elsewhere when the server reports it. */
	UPROPERTY(BlueprintAssignable, Category = "Coralis|Weapon")
	FCoralisShotFiredSignature OnShotFired;

	/** On the owner, when the server confirms one of its shots did damage. */
	UPROPERTY(BlueprintAssignable, Category = "Coralis|Weapon")
	FCoralisHitConfirmedSignature OnHitConfirmed;

	/** On the owner and the server. */
	UPROPERTY(BlueprintAssignable, Category = "Coralis|Weapon")
	FCoralisVentStartedSignature OnVentStarted;

	/** Server: every shot of every weapon in every world, with what it struck (may be null). For reactions to incoming fire. */
	static FCoralisServerShotSignature OnServerShot;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** One input from the owner, kept in order until the server can judge it. */
	struct FPendingInput
	{
		double ClientTime = 0.0;
		double ReceivedTime = 0.0;
		bool bVent = false;
		FVector Origin = FVector::ZeroVector;
		FVector Direction = FVector::ForwardVector;
	};

	UFUNCTION(Server, Reliable)
	void ServerFire(double ClientTime, FVector_NetQuantize10 Origin, FVector_NetQuantizeNormal Direction);

	UFUNCTION(Server, Reliable)
	void ServerVent(double ClientTime);

	UFUNCTION(Server, Reliable)
	void ServerDebugSetHeat(float Fraction);

	/** The server refused an input: take its heat, with its times moved onto the owner's clock. */
	UFUNCTION(Client, Reliable)
	void ClientCorrectHeat(const FCoralisHeatState& ServerState, double ServerClockNow);

	UFUNCTION(Client, Unreliable)
	void ClientConfirmHit(float Damage, ECoralisHitZone Zone, bool bDepleted);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShot(FVector_NetQuantize Start, FVector_NetQuantize End);

	/** Heat for machines that don't run the rules for this weapon, on the server's clock. */
	UPROPERTY(ReplicatedUsing = OnRep_ProxyHeat)
	FCoralisHeatState ProxyHeat;

	UFUNCTION()
	void OnRep_ProxyHeat();

	/** Picked by the server when the weapon appears, so the owner places the first sweet spot where the server does. */
	UPROPERTY(ReplicatedUsing = OnRep_StartWindowSeed)
	int32 StartWindowSeed = 0;

	UFUNCTION()
	void OnRep_StartWindowSeed();

	double GetLocalTime() const;
	double GetStateTime() const;
	bool IsLocallyControlledOwner() const;
	bool CanOwnerUseWeapon() const;
	/** The owner has Coralis health and none of it is left. */
	bool IsOwnerDead() const;
	bool IsOwnerAiming() const;

	void TryFireShot(double ShotTime);
	void GetShotView(FVector& OutOrigin, FVector& OutDirection) const;
	FVector GetMuzzleLocation(const FVector& AimDirection) const;
	/** Follows the view to what is under it, then traces from the muzzle to there, so cover in between blocks. */
	bool TraceShot(const FVector& Origin, const FVector& Direction, FHitResult& OutHit, FVector& OutMuzzle, FVector& OutEnd) const;
	void ResolveShot(const FVector& Origin, const FVector& Direction, bool bBuffed);
	void ApplyRecoil();
	void PlayShotCosmetics(const FVector& Start, const FVector& End);
	void CommitServerState();

	/** Server: puts the client's input time on a clock it can trust. False = the client clock runs too far ahead. */
	bool CheckClientClock(double ClientTime);
	void ProcessPendingInputs();
	void RefuseInput();

	FCoralisHeatState Heat;
	TArray<FPendingInput> PendingInputs;

	bool bTriggerHeld = false;
	double NextShotTime = 0.0;

	/** Server: server time minus the remote owner's weapon clock. */
	double ClientClockOffset = 0.0;
	bool bHasClientClock = false;

	/** Others: server time minus local time, taken when ProxyHeat arrived. */
	double ProxyClockOffset = 0.0;
};
