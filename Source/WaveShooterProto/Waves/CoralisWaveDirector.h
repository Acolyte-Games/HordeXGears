// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoralisWaveDirector.generated.h"

class ACoralisCharacterBase;
class ACoralisEnemyCharacter;
class UCoralisWaveSettings;

UENUM(BlueprintType)
enum class ECoralisWavePhase : uint8
{
	/** Not running. */
	Stopped,
	/** Counting down to a wave. */
	Countdown,
	/** Enemies arriving or alive. */
	Fighting,
	/** The run is lost; counting down to the first wave again. */
	Restarting
};

/** What every machine needs to show about the waves. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisWaveState
{
	GENERATED_BODY()

	/** The wave being fought, or the one being counted down to. Starts at 1. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Waves")
	int32 Wave = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Waves")
	ECoralisWavePhase Phase = ECoralisWavePhase::Stopped;

	/** Enemies of this wave still to beat: the living ones and those yet to arrive. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Waves")
	int32 EnemiesRemaining = 0;

	/** Server world time at which a countdown ends. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Waves")
	double PhaseEndTime = 0.0;

	bool operator==(const FCoralisWaveState& Other) const
	{
		return Wave == Other.Wave && Phase == Other.Phase && EnemiesRemaining == Other.EnemiesRemaining && PhaseEndTime == Other.PhaseEndTime;
	}
};

/**
 * Placeholder wave flow until the game flow owns it: counts down, brings each wave's enemies in at random times and
 * places, waits for all of them to die, and starts over from wave 1 when the players lose.
 * Place one in the level, on walkable ground; its numbers are in the settings asset. The server runs it.
 * Debug: Coralis.Wave.Restart and Coralis.Wave.Skip.
 */
UCLASS(Blueprintable)
class WAVESHOOTERPROTO_API ACoralisWaveDirector : public AActor
{
	GENERATED_BODY()

public:
	ACoralisWaveDirector();

	/** Unset = the class defaults, which have no enemy class, so nothing spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coralis|Waves")
	TObjectPtr<UCoralisWaveSettings> Settings;

	/** Off = something else calls Start Waves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coralis|Waves")
	bool bStartAutomatically = true;

	UFUNCTION(BlueprintPure, Category = "Coralis|Waves")
	const FCoralisWaveState& GetWaveState() const { return WaveState; }

	/** Seconds left in the current countdown; works on every machine. */
	UFUNCTION(BlueprintPure, Category = "Coralis|Waves")
	float GetCountdownRemaining() const;

	const UCoralisWaveSettings& GetSettings() const;

	/** Server only. Clears the field and counts down to wave 1. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Waves")
	void StartWaves();

	/** Server only. Restart from wave 1: removes every enemy and respawns every player. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Waves")
	void RestartRun();

	/** Server only. Ends the current wave or countdown at once. */
	UFUNCTION(BlueprintCallable, Category = "Coralis|Waves")
	void Skip();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	void BeginCountdown(int32 Wave, float Delay);
	void BeginWave();
	void BeginRestartCountdown();
	void TickFighting(double Now);
	/** Looks after dead players; true if the run is now lost. */
	bool CheckPlayers();
	void RespawnPlayers(bool bOnlyDead);
	void RemoveAllEnemies();
	/** Brings in up to Count enemies standing together; returns how many appeared. */
	int32 SpawnArrival(int32 Count);
	ACoralisEnemyCharacter* SpawnEnemyAt(const FVector& Location);
	bool FindSpawnPoint(float CapsuleRadius, float CapsuleHalfHeight, FVector& OutLocation) const;
	/** A free place next to the first member of a group. */
	bool FindGroupPoint(const FVector& Center, const TArray<FVector>& Taken, float CapsuleRadius, float CapsuleHalfHeight, FVector& OutLocation) const;
	/** Distance to the nearest living player; very large with none. */
	float DistanceToNearestPlayer(const FVector& Location) const;
	int32 CountPending() const;
	void GetLivingPlayers(TArray<ACoralisCharacterBase*>& OutPlayers) const;

	UPROPERTY(Replicated)
	FCoralisWaveState WaveState;

	/** Living enemies of the current wave. */
	TArray<TWeakObjectPtr<ACoralisEnemyCharacter>> Alive;

	/** One enemy, or a group that appears together. */
	struct FArrival
	{
		double Time = 0.0;
		int32 Count = 1;
	};

	/** The arrivals still to come, soonest first. */
	TArray<FArrival> PendingSpawns;

	double NextRetryTime = 0.0;
	double EmptySinceTime = -1.0;
	int32 FailedSpawns = 0;

	/** Set once somebody has been alive, so a level that is still starting up does not count as a lost run. */
	bool bSquadWasAlive = false;
	/** Dead players already seen, so one death counts once. */
	TSet<TWeakObjectPtr<AActor>> KnownDead;
};
