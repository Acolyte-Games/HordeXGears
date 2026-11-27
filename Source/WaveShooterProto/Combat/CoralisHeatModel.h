// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CoralisHeatModel.generated.h"

class UCoralisWeaponSettings;

UENUM(BlueprintType)
enum class ECoralisHeatPhase : uint8
{
	/** Ready. Passive cooling runs once the delay after the last shot has passed. */
	Ready,
	Venting,
	SweetVenting,
	/** Max heat reached: locked until the jam time is over. */
	Jammed
};

/**
 * Heat of one weapon, anchored at the time of the last input that changed it. Between inputs everything follows from the
 * anchor and the time, so the owning client and the server get the same result from the same timestamps, at any frame
 * rate. Times are seconds on the shooter's clock.
 */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisHeatState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Weapon")
	float Heat = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Weapon")
	ECoralisHeatPhase Phase = ECoralisHeatPhase::Ready;

	/** Time Heat and Phase were true. */
	UPROPERTY()
	double Time = 0.0;

	UPROPERTY()
	double LastShotTime = -1000.0;

	UPROPERTY()
	double JamEndTime = 0.0;

	UPROPERTY()
	double BuffEndTime = 0.0;

	/**
	 * Places the sweet spot window. It moves on with each vent and each jam, by the same steps on every machine,
	 * so the window travels with the state and is never sent by itself.
	 */
	UPROPERTY()
	int32 WindowSeed = 0;

	/** The state as it is at Now (never earlier than the anchor). */
	FCoralisHeatState At(double Now, const UCoralisWeaponSettings& Settings) const;

	/** Not venting or jammed, and the fire interval has passed. */
	bool CanFire(double Now, const UCoralisWeaponSettings& Settings) const;

	/** Adds one shot's heat. Returns false when the weapon can't fire at Now. */
	bool TryFire(double Now, const UCoralisWeaponSettings& Settings);

	/**
	 * Starts a vent, which runs until the heat is gone; nothing cancels it. The heat at this moment decides the sweet
	 * spot, so each vent gets one try. Ignored while jammed, already venting or cold.
	 */
	bool TryVent(double Now, const UCoralisWeaponSettings& Settings);

	/** Debug: forces the heat. At max heat the weapon jams. */
	void ForceHeat(double Now, float NewHeat, const UCoralisWeaponSettings& Settings);

	bool IsBuffActive(double Now) const { return Now < BuffEndTime; }

	/** The window a vent has to start in, as fractions of max heat. End equals start when there is no sweet spot. */
	void GetSweetSpotWindow(const UCoralisWeaponSettings& Settings, float& OutStart, float& OutEnd) const;

	/** Moves every time by Delta, to carry the state over to another clock. */
	void ShiftTimes(double Delta)
	{
		Time += Delta;
		LastShotTime += Delta;
		JamEndTime += Delta;
		BuffEndTime += Delta;
	}

private:
	void Jam(double Now, const UCoralisWeaponSettings& Settings);
	void MoveSweetSpot();
};
