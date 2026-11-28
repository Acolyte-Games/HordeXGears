// Copyright Acolyte Games. All Rights Reserved.

#include "Waves/CoralisWaveDirector.h"

#include "AI/CoralisEnemyAIController.h"
#include "AI/CoralisEnemyCharacter.h"
#include "Characters/CoralisPlayerCharacter.h"
#include "Combat/CoralisHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Waves/CoralisWaveSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoralisWaves, Log, All);

namespace
{
	/** Failed arrivals in a row before the log says the spawn rules may be impossible on this map. */
	constexpr int32 FailedSpawnsBeforeWarning = 10;
	/** Gap between the floor and the capsule of a new enemy, so it does not start inside the ground. */
	constexpr float SpawnFloorClearance = 5.f;

	void ForEachDirector(UWorld* World, TFunctionRef<void(ACoralisWaveDirector&)> Action)
	{
		if (!World || World->GetNetMode() == NM_Client)
		{
			return;
		}
		for (TActorIterator<ACoralisWaveDirector> It(World); It; ++It)
		{
			Action(**It);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs CmdWaveRestart(
		TEXT("Coralis.Wave.Restart"), TEXT("Server: back to wave 1, with every player respawned."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDirector(World, [](ACoralisWaveDirector& Director) { Director.RestartRun(); });
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdWaveSkip(
		TEXT("Coralis.Wave.Skip"), TEXT("Server: ends the current wave or countdown."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDirector(World, [](ACoralisWaveDirector& Director) { Director.Skip(); });
		}));
}

ACoralisWaveDirector::ACoralisWaveDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	// Every player shows its state, wherever they are.
	bAlwaysRelevant = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

void ACoralisWaveDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoralisWaveDirector, WaveState);
}

const UCoralisWaveSettings& ACoralisWaveDirector::GetSettings() const
{
	return Settings ? *Settings : *GetDefault<UCoralisWaveSettings>();
}

float ACoralisWaveDirector::GetCountdownRemaining() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? FMath::Max(0.f, static_cast<float>(WaveState.PhaseEndTime - GameState->GetServerWorldTimeSeconds())) : 0.f;
}

void ACoralisWaveDirector::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && bStartAutomatically)
	{
		StartWaves();
	}
}

void ACoralisWaveDirector::StartWaves()
{
	if (!HasAuthority())
	{
		return;
	}
	RemoveAllEnemies();
	bSquadWasAlive = false;
	BeginCountdown(1, GetSettings().FirstWaveDelay);
}

void ACoralisWaveDirector::RestartRun()
{
	if (!HasAuthority())
	{
		return;
	}
	StartWaves();
	RespawnPlayers(false);
}

void ACoralisWaveDirector::Skip()
{
	if (!HasAuthority())
	{
		return;
	}
	switch (WaveState.Phase)
	{
	case ECoralisWavePhase::Countdown:
		WaveState.PhaseEndTime = GetWorld()->GetTimeSeconds();
		break;

	case ECoralisWavePhase::Fighting:
		PendingSpawns.Reset();
		RemoveAllEnemies();
		break;

	case ECoralisWavePhase::Restarting:
		RestartRun();
		break;

	default:
		break;
	}
}

void ACoralisWaveDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || WaveState.Phase == ECoralisWavePhase::Stopped)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();

	if (WaveState.Phase == ECoralisWavePhase::Restarting)
	{
		CheckPlayers();
		if (Now >= WaveState.PhaseEndTime)
		{
			RestartRun();
		}
		return;
	}
	if (CheckPlayers())
	{
		BeginRestartCountdown();
		return;
	}

	if (WaveState.Phase == ECoralisWavePhase::Countdown)
	{
		if (Now >= WaveState.PhaseEndTime)
		{
			BeginWave();
		}
		return;
	}
	TickFighting(Now);
}

void ACoralisWaveDirector::BeginCountdown(int32 Wave, float Delay)
{
	PendingSpawns.Reset();
	WaveState.Wave = Wave;
	WaveState.Phase = ECoralisWavePhase::Countdown;
	WaveState.EnemiesRemaining = 0;
	// The game state's clock is the world time on the server.
	WaveState.PhaseEndTime = GetWorld()->GetTimeSeconds() + Delay;
	ForceNetUpdate();
}

void ACoralisWaveDirector::BeginWave()
{
	const UCoralisWaveSettings& S = GetSettings();
	const double Now = GetWorld()->GetTimeSeconds();
	const int32 Count = S.GetEnemyCount(WaveState.Wave);

	// The wave is first cut into arrivals: single enemies and, by chance, whole groups.
	TArray<int32> Sizes;
	int32 Groups = 0;
	for (int32 Left = Count; Left > 0; Left -= Sizes.Last())
	{
		int32 Size = 1;
		if (Left > 1 && FMath::FRand() < S.GroupChance)
		{
			Size = FMath::Min(Left, FMath::RandRange(S.GroupSizeMin, FMath::Max(S.GroupSizeMin, S.GroupSizeMax)));
			++Groups;
		}
		Sizes.Add(Size);
	}

	// One random moment in each equal slice of the round: spread out, but never on a beat.
	PendingSpawns.Reset(Sizes.Num());
	const float Slice = S.RoundDuration / FMath::Max(1, Sizes.Num());
	float Previous = -S.MinTimeBetweenSpawns;
	for (int32 Index = 0; Index < Sizes.Num(); ++Index)
	{
		float Offset = (Index + FMath::FRand()) * Slice;
		if (Index == 0)
		{
			Offset = FMath::FRandRange(0.f, FMath::Min(S.MaxFirstSpawnDelay, S.RoundDuration));
		}
		Offset = FMath::Max(Offset, Previous + S.MinTimeBetweenSpawns);
		Previous = Offset;
		PendingSpawns.Add({ Now + Offset, Sizes[Index] });
	}

	NextRetryTime = 0.0;
	EmptySinceTime = Now;
	FailedSpawns = 0;
	WaveState.Phase = ECoralisWavePhase::Fighting;
	WaveState.EnemiesRemaining = Count;
	WaveState.PhaseEndTime = 0.0;
	ForceNetUpdate();
	UE_LOG(LogCoralisWaves, Log, TEXT("Wave %d: %d enemies over %.0f s in %d arrivals, %d of them groups"),
		WaveState.Wave, Count, S.RoundDuration, Sizes.Num(), Groups);
}

void ACoralisWaveDirector::BeginRestartCountdown()
{
	PendingSpawns.Reset();
	WaveState.Phase = ECoralisWavePhase::Restarting;
	WaveState.PhaseEndTime = GetWorld()->GetTimeSeconds() + GetSettings().RestartDelay;
	ForceNetUpdate();
	UE_LOG(LogCoralisWaves, Log, TEXT("Run lost on wave %d"), WaveState.Wave);
}

void ACoralisWaveDirector::TickFighting(double Now)
{
	const UCoralisWaveSettings& S = GetSettings();
	Alive.RemoveAll([](const TWeakObjectPtr<ACoralisEnemyCharacter>& Enemy) { return !Enemy.IsValid() || Enemy->IsDead(); });

	if (!Alive.IsEmpty())
	{
		EmptySinceTime = -1.0;
	}
	else if (!PendingSpawns.IsEmpty())
	{
		if (EmptySinceTime < 0.0)
		{
			EmptySinceTime = Now;
		}
		if (S.MaxEmptyTime > 0.f)
		{
			PendingSpawns[0].Time = FMath::Min(PendingSpawns[0].Time, EmptySinceTime + S.MaxEmptyTime);
		}
	}

	// One arrival per frame at most, so a late batch doesn't pop in together.
	if (!PendingSpawns.IsEmpty() && Now >= PendingSpawns[0].Time && Now >= NextRetryTime)
	{
		const int32 Spawned = SpawnArrival(PendingSpawns[0].Count);
		if (Spawned > 0)
		{
			// Members that found no room stay next in line.
			PendingSpawns[0].Count -= Spawned;
			if (PendingSpawns[0].Count <= 0)
			{
				PendingSpawns.RemoveAt(0);
			}
			NextRetryTime = Now + S.MinTimeBetweenSpawns;
			FailedSpawns = 0;
		}
		else
		{
			NextRetryTime = Now + S.SpawnRetryDelay;
			if (++FailedSpawns == FailedSpawnsBeforeWarning)
			{
				UE_LOG(LogCoralisWaves, Warning, TEXT("No spawn place found %d times in a row. Check the enemy class, the navmesh, the director's position and the spawn distances."), FailedSpawns);
			}
		}
	}

	const int32 Remaining = Alive.Num() + CountPending();
	if (Remaining != WaveState.EnemiesRemaining)
	{
		WaveState.EnemiesRemaining = Remaining;
		ForceNetUpdate();
	}
	if (Remaining == 0)
	{
		if (S.bRespawnDeadAtWaveEnd)
		{
			RespawnPlayers(true);
		}
		BeginCountdown(WaveState.Wave + 1, S.TimeBetweenWaves);
	}
}

bool ACoralisWaveDirector::CheckPlayers()
{
	const UCoralisWaveSettings& S = GetSettings();
	int32 Players = 0;
	int32 Living = 0;
	bool bNewDeath = false;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		++Players;
		ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(PC->GetPawn());
		if (Character && !Character->IsDead())
		{
			++Living;
			continue;
		}
		ACoralisPlayerCharacter* Player = Cast<ACoralisPlayerCharacter>(Character);
		if (!Player)
		{
			continue;
		}
		if (!KnownDead.Contains(Player))
		{
			KnownDead.Add(Player);
			bNewDeath = true;
		}
		// While the director decides when players come back, their own timer stays off.
		if (S.bRespawnDeadAtWaveEnd || WaveState.Phase == ECoralisWavePhase::Restarting)
		{
			Player->CancelPendingRespawn();
		}
	}
	bSquadWasAlive |= Living > 0;

	switch (S.RestartRule)
	{
	case ECoralisWaveRestartRule::WholeSquadDown:
		return bSquadWasAlive && Players > 0 && Living == 0;

	case ECoralisWaveRestartRule::AnyPlayerDies:
		return bNewDeath;

	default:
		// Nothing restarts: a fully dead squad would wait for ever, so bring it back.
		if (S.bRespawnDeadAtWaveEnd && bSquadWasAlive && Players > 0 && Living == 0)
		{
			RespawnPlayers(true);
		}
		return false;
	}
}

void ACoralisWaveDirector::RespawnPlayers(bool bOnlyDead)
{
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (!GameMode)
	{
		return;
	}
	// Collected first: respawning changes the pawns being looked at.
	TArray<APlayerController*> Controllers;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		const ACoralisCharacterBase* Character = PC ? Cast<ACoralisCharacterBase>(PC->GetPawn()) : nullptr;
		if (PC && (!bOnlyDead || !Character || Character->IsDead()))
		{
			Controllers.Add(PC);
		}
	}
	for (APlayerController* PC : Controllers)
	{
		if (ACoralisPlayerCharacter* Player = Cast<ACoralisPlayerCharacter>(PC->GetPawn()))
		{
			Player->Respawn();
		}
		else if (!PC->GetPawn())
		{
			GameMode->RestartPlayer(PC);
		}
	}
	KnownDead.Reset();
}

void ACoralisWaveDirector::RemoveAllEnemies()
{
	for (TActorIterator<ACoralisEnemyCharacter> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	Alive.Reset();
}

int32 ACoralisWaveDirector::CountPending() const
{
	int32 Count = 0;
	for (const FArrival& Arrival : PendingSpawns)
	{
		Count += Arrival.Count;
	}
	return Count;
}

int32 ACoralisWaveDirector::SpawnArrival(int32 Count)
{
	const UCoralisWaveSettings& S = GetSettings();
	if (!S.EnemyClass)
	{
		return 0;
	}
	const UCapsuleComponent* Capsule = S.EnemyClass->GetDefaultObject<ACoralisEnemyCharacter>()->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	FVector First;
	if (!FindSpawnPoint(Radius, HalfHeight, First))
	{
		return 0;
	}

	TArray<FVector> Places = { First };
	FVector Next;
	while (Places.Num() < Count && FindGroupPoint(First, Places, Radius, HalfHeight, Next))
	{
		Places.Add(Next);
	}

	ACoralisEnemyAIController* Leader = nullptr;
	int32 Spawned = 0;
	for (const FVector& Place : Places)
	{
		const ACoralisEnemyCharacter* Enemy = SpawnEnemyAt(Place);
		if (!Enemy)
		{
			continue;
		}
		++Spawned;
		if (ACoralisEnemyAIController* Brain = Cast<ACoralisEnemyAIController>(Enemy->GetController()))
		{
			if (!Leader)
			{
				Leader = Brain;
			}
			else if (S.bGroupsFightAsSquad)
			{
				Brain->JoinSquad(*Leader);
			}
		}
	}
	if (Count > 1)
	{
		UE_LOG(LogCoralisWaves, Log, TEXT("Group arrived: %d of %d"), Spawned, Count);
	}
	return Spawned;
}

ACoralisEnemyCharacter* ACoralisWaveDirector::SpawnEnemyAt(const FVector& Location)
{
	// Turned toward the nearest player.
	TArray<ACoralisCharacterBase*> Players;
	GetLivingPlayers(Players);
	FRotator Rotation = FRotator::ZeroRotator;
	float Nearest = TNumericLimits<float>::Max();
	for (const ACoralisCharacterBase* Player : Players)
	{
		const float Distance = FVector::Dist(Location, Player->GetActorLocation());
		if (Distance < Nearest)
		{
			Nearest = Distance;
			Rotation = FRotator(0.f, (Player->GetActorLocation() - Location).Rotation().Yaw, 0.f);
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	ACoralisEnemyCharacter* Enemy = GetWorld()->SpawnActor<ACoralisEnemyCharacter>(GetSettings().EnemyClass, Location, Rotation, Params);
	if (!Enemy)
	{
		return nullptr;
	}
	if (!Enemy->GetController())
	{
		Enemy->SpawnDefaultController();
	}
	Alive.Add(Enemy);
	return Enemy;
}

float ACoralisWaveDirector::DistanceToNearestPlayer(const FVector& Location) const
{
	TArray<ACoralisCharacterBase*> Players;
	GetLivingPlayers(Players);
	float Nearest = TNumericLimits<float>::Max();
	for (const ACoralisCharacterBase* Player : Players)
	{
		Nearest = FMath::Min(Nearest, static_cast<float>(FVector::Dist(Location, Player->GetActorLocation())));
	}
	return Nearest;
}

bool ACoralisWaveDirector::FindGroupPoint(const FVector& Center, const TArray<FVector>& Taken, float CapsuleRadius, float CapsuleHalfHeight, FVector& OutLocation) const
{
	const UCoralisWaveSettings& S = GetSettings();
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const FVector Lift(0.f, 0.f, CapsuleHalfHeight + SpawnFloorClearance);
	for (int32 Attempt = 0; Nav && Attempt < S.SpawnAttempts; ++Attempt)
	{
		FNavLocation Point;
		if (!Nav->GetRandomReachablePointInRadius(Center - Lift, S.GroupSpawnRadius, Point))
		{
			continue;
		}
		const FVector Location = Point.Location + Lift;
		// The navigation radius is only a hint, so the distance is checked again here.
		if (FVector::Dist2D(Location, Center) > S.GroupSpawnRadius
			|| Taken.ContainsByPredicate([&](const FVector& Other) { return FVector::Dist2D(Location, Other) < S.GroupMemberSpacing; }))
		{
			continue;
		}
		if (DistanceToNearestPlayer(Location) >= S.MinDistanceFromPlayers
			&& !GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn,
				FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight)))
		{
			OutLocation = Location;
			return true;
		}
	}
	return false;
}

bool ACoralisWaveDirector::FindSpawnPoint(float CapsuleRadius, float CapsuleHalfHeight, FVector& OutLocation) const
{
	const UCoralisWaveSettings& S = GetSettings();
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav)
	{
		return false;
	}
	FNavLocation Origin;
	if (!Nav->ProjectPointToNavigation(GetActorLocation(), Origin, FVector(500.f, 500.f, 1000.f)))
	{
		return false;
	}

	TArray<ACoralisCharacterBase*> Players;
	GetLivingPlayers(Players);

	bool bFoundVisible = false;
	for (int32 Attempt = 0; Attempt < S.SpawnAttempts; ++Attempt)
	{
		FNavLocation Point;
		if (!Nav->GetRandomReachablePointInRadius(Origin.Location, S.SpawnRadius, Point))
		{
			continue;
		}
		const FVector Location = Point.Location + FVector(0.f, 0.f, CapsuleHalfHeight + SpawnFloorClearance);

		const float NearestPlayer = DistanceToNearestPlayer(Location);
		if (NearestPlayer < S.MinDistanceFromPlayers || (S.MaxDistanceFromPlayers > 0.f && !Players.IsEmpty() && NearestPlayer > S.MaxDistanceFromPlayers))
		{
			continue;
		}
		const bool bCrowded = Alive.ContainsByPredicate([&](const TWeakObjectPtr<ACoralisEnemyCharacter>& Enemy)
		{
			return Enemy.IsValid() && FVector::Dist(Location, Enemy->GetActorLocation()) < S.MinDistanceFromOtherEnemies;
		});
		if (bCrowded || GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight)))
		{
			continue;
		}

		bool bSeen = false;
		if (S.bPreferOutOfSight)
		{
			for (const ACoralisCharacterBase* Player : Players)
			{
				FCollisionQueryParams Query(SCENE_QUERY_STAT(CoralisSpawnSight), false, Player);
				if (!GetWorld()->LineTraceTestByChannel(Player->GetPawnViewLocation(), Location, ECC_Visibility, Query))
				{
					bSeen = true;
					break;
				}
			}
		}
		if (!bSeen)
		{
			OutLocation = Location;
			return true;
		}
		if (!bFoundVisible)
		{
			bFoundVisible = true;
			OutLocation = Location;
		}
	}
	return bFoundVisible;
}

void ACoralisWaveDirector::GetLivingPlayers(TArray<ACoralisCharacterBase*>& OutPlayers) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		ACoralisCharacterBase* Player = PC ? Cast<ACoralisCharacterBase>(PC->GetPawn()) : nullptr;
		if (Player && !Player->IsDead())
		{
			OutPlayers.Add(Player);
		}
	}
}
