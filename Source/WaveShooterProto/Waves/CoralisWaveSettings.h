// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoralisWaveSettings.generated.h"

class ACoralisEnemyCharacter;

/** What sends the run back to the first wave. */
UENUM(BlueprintType)
enum class ECoralisWaveRestartRule : uint8
{
	/** Every player is down at the same time. With one player: that player dies. */
	WholeSquadDown,
	/** The first death restarts the run. */
	AnyPlayerDies,
	Never
};

/** Numbers of the placeholder wave flow. Times in seconds, distances in cm. Edits apply from the next wave. */
UCLASS(BlueprintType)
class WAVESHOOTERPROTO_API UCoralisWaveSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemies")
	TSubclassOf<ACoralisEnemyCharacter> EnemyClass;

	// --- Wave size -------------------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Size", meta = (ClampMin = "1"))
	int32 FirstWaveEnemies = 4;

	/** How many more enemies the second wave has than the first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Size", meta = (ClampMin = "0"))
	int32 FirstWaveIncrease = 2;

	/** Each later increase is this much bigger than the one before. 0 = every wave adds the same number. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Size", meta = (ClampMin = "0"))
	int32 IncreaseGrowth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Size", meta = (ClampMin = "1"))
	int32 MaxEnemiesPerWave = 30;

	/** If filled, these counts are used for the first waves in order, and the formula above takes over after them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave Size")
	TArray<int32> FixedWaveCounts;

	// --- Timing ------------------------------------------------------------------------------------------------------

	/** Countdown before the first wave, also after a restart. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", Units = "Seconds"))
	float FirstWaveDelay = 5.f;

	/** Countdown from the last enemy's death to the next wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", Units = "Seconds"))
	float TimeBetweenWaves = 15.f;

	/** The game round: a wave's enemies arrive at random moments spread over this time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", Units = "Seconds"))
	float RoundDuration = 60.f;

	/** The first enemy of a wave arrives within this time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", Units = "Seconds"))
	float MaxFirstSpawnDelay = 3.f;

	/** At least this long between two arrivals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", Units = "Seconds"))
	float MinTimeBetweenSpawns = 1.5f;

	/** With nobody left to fight and more still to come, the next one arrives within this time. 0 = keep the schedule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", Units = "Seconds"))
	float MaxEmptyTime = 8.f;

	// --- Where enemies appear ----------------------------------------------------------------------------------------

	/** No enemy appears closer than this to a living player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Location", meta = (ClampMin = "0", Units = "cm"))
	float MinDistanceFromPlayers = 2000.f;

	/** No enemy appears further than this from the nearest living player. 0 = no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Location", meta = (ClampMin = "0", Units = "cm"))
	float MaxDistanceFromPlayers = 0.f;

	/** Spawn area around the wave director actor. Only places that can be walked to from the director are used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Location", meta = (ClampMin = "100", Units = "cm"))
	float SpawnRadius = 6000.f;

	/** Enemies avoid appearing where a living player can see them, if another place turns up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Location")
	bool bPreferOutOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Location", meta = (ClampMin = "0", Units = "cm"))
	float MinDistanceFromOtherEnemies = 300.f;

	/** Random places tried for each enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Location", meta = (ClampMin = "1", ClampMax = "256"))
	int32 SpawnAttempts = 48;

	/** Wait before trying again when no place fits. The distance rules are never relaxed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Location", meta = (ClampMin = "0.1", Units = "Seconds"))
	float SpawnRetryDelay = 1.f;

	// --- Groups --------------------------------------------------------------------------------------------------------

	/** Chance that an arrival is a group instead of one enemy. 0 = always alone, 1 = always groups. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups", meta = (ClampMin = "0", ClampMax = "1"))
	float GroupChance = 0.35f;

	/** Enemies in a group (random between the two). The last group of a wave may be smaller. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups", meta = (ClampMin = "2"))
	int32 GroupSizeMin = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups", meta = (ClampMin = "2"))
	int32 GroupSizeMax = 4;

	/** The others of a group appear within this distance of the first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups", meta = (ClampMin = "100", Units = "cm"))
	float GroupSpawnRadius = 600.f;

	/** Closest two members of a group appear to each other. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups", meta = (ClampMin = "0", Units = "cm"))
	float GroupMemberSpacing = 150.f;

	/** On = a spawned group fights as a squad: same target, moves together, rushes together. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups")
	bool bGroupsFightAsSquad = true;

	// --- Death ---------------------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	ECoralisWaveRestartRule RestartRule = ECoralisWaveRestartRule::WholeSquadDown;

	/** Time from the death that ends the run to the restart. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death", meta = (ClampMin = "0", Units = "Seconds"))
	float RestartDelay = 5.f;

	/**
	 * A dead player stays down until the wave is cleared, then comes back if the run goes on.
	 * Off = the player's own Respawn Delay applies.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bRespawnDeadAtWaveEnd = true;

	/** Enemies in the given wave (the first wave is 1). */
	UFUNCTION(BlueprintPure, Category = "Wave Size")
	int32 GetEnemyCount(int32 Wave) const;
};
