// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "CoralisWeaponSettings.generated.h"

class USoundBase;

/**
 * All tunables for one heat weapon. The weapon reads only from here, so designers tune without touching code.
 * Heat has no unit: only the ratio of the values to MaxHeat matters.
 * Cooling should stay ordered slowest to fastest: jam (MaxHeat / JamDuration), passive, vent, sweet spot vent.
 */
UCLASS(BlueprintType)
class WAVESHOOTERPROTO_API UCoralisWeaponSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	FText DisplayName;

	// --- Fire ---

	/** Shots per second while the trigger is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire", meta = (ClampMin = "0.1"))
	float FireRate = 9.f;

	/** Damage of one shot on a body hit, before hit zone and buff multipliers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire", meta = (ClampMin = "0"))
	float Damage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire", meta = (ClampMin = "0", Units = "Centimeters"))
	float Range = 10000.f;

	/** Trace channel shots use. The project's HitScan channel by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel1;

	/** Shots can damage other players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	bool bFriendlyFire = false;

	/** Where shots leave the character: X forward, Y right, Z up from the actor centre, turned with the view yaw. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	FVector MuzzleOffset = FVector(50.f, 25.f, 35.f);

	// --- Heat ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat", meta = (ClampMin = "1"))
	float MaxHeat = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat", meta = (ClampMin = "0"))
	float HeatPerShot = 4.f;

	/** Time after the last shot before passive cooling starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat", meta = (ClampMin = "0", Units = "Seconds"))
	float PassiveCoolDelay = 1.f;

	/** Passive cooling (heat per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat", meta = (ClampMin = "0"))
	float PassiveCoolRate = 35.f;

	/** Lockout after reaching max heat. Heat drains from max to zero over this time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat", meta = (ClampMin = "0.1", Units = "Seconds"))
	float JamDuration = 4.f;

	/** After a jam, the trigger has to be pulled again (off = a held trigger resumes firing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat")
	bool bNewTriggerPullAfterJam = true;

	// --- Vent ---

	/** After a vent, the trigger has to be pulled again (off = a held trigger fires as soon as the heat is gone). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vent")
	bool bNewTriggerPullAfterVent = true;

	/** Cooling during a manual vent (heat per second). Starts at once; a vent always runs until the heat is gone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vent", meta = (ClampMin = "0"))
	float VentCoolRate = 55.f;

	/** The sweet spot window lands at a random place between these two fractions of max heat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vent", meta = (ClampMin = "0", ClampMax = "1"))
	float SweetSpotRangeStart = 0.6f;

	/** The window never reaches past this. The default is the highest heat that doesn't jam at 4 heat per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vent", meta = (ClampMin = "0", ClampMax = "1"))
	float SweetSpotRangeEnd = 0.96f;

	/** Width of the sweet spot window as a fraction of max heat. 0 turns the sweet spot off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vent", meta = (ClampMin = "0", ClampMax = "1"))
	float SweetSpotWidth = 0.2f;

	/** Cooling after venting inside the window (heat per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vent", meta = (ClampMin = "0"))
	float SweetSpotCoolRate = 110.f;

	// --- Buff ---

	/** Damage multiplier granted by a sweet spot vent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "1"))
	float BuffDamageMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff", meta = (ClampMin = "0", Units = "Seconds"))
	float BuffDuration = 5.f;

	// --- Spread ---

	/** Cone half angle when firing from the hip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0", ClampMax = "45", Units = "Degrees"))
	float HipSpread = 2.5f;

	/** Cone half angle while aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread", meta = (ClampMin = "0", ClampMax = "45", Units = "Degrees"))
	float AimSpread = 0.5f;

	// --- Recoil ---

	/** View kick upward per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0", Units = "Degrees"))
	float RecoilPitch = 0.3f;

	/** Random sideways kick per shot, up to this much either way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0", Units = "Degrees"))
	float RecoilYaw = 0.15f;

	/** Scales both recoil values while aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0"))
	float AimRecoilScale = 0.7f;

	// --- Placeholder effects ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<USoundBase> FireSound;

	/** Placeholder tracer line, development builds only. 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (ClampMin = "0", Units = "Seconds"))
	float TracerDuration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	FColor TracerColor = FColor(80, 220, 255);

	// --- Server checks ---

	/** Server rejects a shot whose view origin is further than this from the shooter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Server", meta = (ClampMin = "0", Units = "Centimeters"))
	float MaxAimOriginDistance = 700.f;

	/** How far a client's weapon clock may run ahead of the server's before its input is refused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Server", meta = (ClampMin = "0", Units = "Seconds"))
	float ClientClockTolerance = 0.25f;

	/** How long the server holds a shot while its copy of the shooter is still sprinting or rolling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Server", meta = (ClampMin = "0", Units = "Seconds"))
	float ServerMoveStateGrace = 0.25f;

	float GetFireInterval() const { return 1.f / FMath::Max(FireRate, 0.1f); }
	float GetJamCoolRate() const { return MaxHeat / FMath::Max(JamDuration, 0.1f); }

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
