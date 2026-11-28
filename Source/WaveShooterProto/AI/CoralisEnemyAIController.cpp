// Copyright Acolyte Games. All Rights Reserved.

#include "AI/CoralisEnemyAIController.h"

#include "AI/CoralisEnemyBehaviorSettings.h"
#include "AI/CoralisEnemyCharacter.h"
#include "AI/CoralisEnemySquad.h"
#include "Combat/CoralisWeaponComponent.h"
#include "Combat/CoralisWeaponSettings.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Movement/CoralisMovementComponent.h"
#include "Movement/CoralisMovementSettings.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	TAutoConsoleVariable<bool> CVarShowEnemyDebug(
		TEXT("Coralis.Enemy.ShowDebug"), false,
		TEXT("Server: draws each enemy's state, where it is heading and whether it sees its target."));

	/** Step between cover samples around the circle, so they spread evenly without lining up. */
	constexpr float GoldenAngleDegrees = 137.508f;
	/** How far off the walkable surface a wanted point may be and still count. */
	const FVector NavSearchExtent(120.f, 120.f, 300.f);
	/** Nearest cover spots checked for a real path. */
	constexpr int32 CoverPathChecks = 6;

#if !UE_BUILD_SHIPPING
	FAutoConsoleCommandWithWorldAndArgs CmdKillEnemies(
		TEXT("Coralis.Enemy.KillAll"), TEXT("Server: kills every enemy."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || World->GetNetMode() == NM_Client)
			{
				return;
			}
			for (TActorIterator<ACoralisEnemyCharacter> It(World); It; ++It)
			{
				It->GetHealthComponent()->DebugDamage(It->GetHealthComponent()->MaxHealth);
			}
		}));
#endif
}

ACoralisEnemyAIController::ACoralisEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

APawn* ACoralisEnemyAIController::GetTargetPawn() const
{
	return Target.Get();
}

void ACoralisEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Enemy = Cast<ACoralisEnemyCharacter>(InPawn);
	if (Enemy)
	{
		Enemy->GetHealthComponent()->OnDamaged.AddUniqueDynamic(this, &ThisClass::HandleDamaged);
		ServerShotHandle = UCoralisWeaponComponent::OnServerShot.AddUObject(this, &ThisClass::HandleServerShot);
		SetControlRotation(Enemy->GetActorRotation());
		// Spread the decisions of enemies spawned in the same frame.
		const UCoralisEnemyBehaviorSettings& B = Enemy->GetBehavior();
		const double Now = GetWorld()->GetTimeSeconds();
		NextThinkTime = Now + FMath::FRand() * B.DecisionInterval;
		NextRushTime = Now + FMath::FRand() * B.RushDecisionInterval;
		NextDashRollTime = Now + FMath::FRand() * B.DashDecisionInterval;
		NextGroupUpTime = Now + FMath::FRand() * B.GroupUpDecisionInterval;
	}
}

void ACoralisEnemyAIController::OnUnPossess()
{
	UCoralisWeaponComponent::OnServerShot.Remove(ServerShotHandle);
	ServerShotHandle.Reset();
	if (Enemy)
	{
		Enemy->GetHealthComponent()->OnDamaged.RemoveDynamic(this, &ThisClass::HandleDamaged);
		Enemy->GetWeapon()->StopFire();
	}
	StopMovement();
	LeaveSquad();
	Enemy = nullptr;
	Super::OnUnPossess();
}

void ACoralisEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Enemy || Enemy->IsDead())
	{
		return;
	}
	const UCoralisEnemyBehaviorSettings& B = Enemy->GetBehavior();
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextThinkTime)
	{
		NextThinkTime = Now + B.DecisionInterval;
		Think(B);
	}
	UpdateShooting(B);

#if !UE_BUILD_SHIPPING
	if (CVarShowEnemyDebug.GetValueOnGameThread())
	{
		const FVector At = Enemy->GetActorLocation();
		DrawDebugString(GetWorld(), At + FVector(0, 0, 120), FString::Printf(TEXT("%s  %.0f hp  range %.0f%s%s  squad %d%s"),
			*UEnum::GetDisplayValueAsText(State).ToString(), Enemy->GetHealthComponent()->GetHealth(), PreferredRange,
			bRushing ? TEXT("  rushing") : TEXT(""), bBursting ? TEXT("  firing") : TEXT(""), GetSquadSize(),
			GetSquadSize() > 1 && !IsSquadFollower() ? TEXT(" (leader)") : TEXT("")), nullptr, FColor::White, 0.f, true);
		if (IsSquadFollower())
		{
			if (const APawn* LeaderPawn = GetSquadLeader()->GetPawn())
			{
				DrawDebugLine(GetWorld(), At, LeaderPawn->GetActorLocation(), FColor::Yellow);
			}
		}
		if (GetMoveStatus() == EPathFollowingStatus::Moving)
		{
			DrawDebugLine(GetWorld(), At, MoveGoal, State == ECoralisEnemyState::MoveToCover ? FColor::Cyan : FColor::Green);
			DrawDebugSphere(GetWorld(), MoveGoal, 25.f, 8, FColor::Green);
		}
		if (Target.IsValid())
		{
			DrawDebugLine(GetWorld(), Enemy->GetPawnViewLocation(), Target->GetActorLocation(), bSeesTarget ? FColor::Red : FColor::Silver);
		}
	}
#endif
}

// --- Decisions -------------------------------------------------------------------------------------------------------

void ACoralisEnemyAIController::UpdateTarget(const UCoralisEnemyBehaviorSettings& B)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (B.bSquadSharesTarget && IsSquadFollower())
	{
		ACoralisCharacterBase* Shared = GetSquadLeader()->Target.Get();
		if (Shared && !Shared->IsDead())
		{
			if (Target.Get() != Shared)
			{
				Target = Shared;
				bSeesTarget = false;
			}
			return;
		}
	}
	ACoralisCharacterBase* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	ACoralisCharacterBase* Current = Target.Get();
	float CurrentScore = TNumericLimits<float>::Max();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		ACoralisCharacterBase* Player = PC ? Cast<ACoralisCharacterBase>(PC->GetPawn()) : nullptr;
		if (!Player || Player->IsDead())
		{
			continue;
		}
		float Score = FVector::Dist(Enemy->GetActorLocation(), Player->GetActorLocation());
		if (LastAttacker.Get() == PC && Now - LastAttackedTime <= B.AttackerMemory)
		{
			Score -= B.AttackerPriorityDistance;
		}
		if (Player == Current)
		{
			CurrentScore = Score;
		}
		if (Score < BestScore)
		{
			Best = Player;
			BestScore = Score;
		}
	}
	// Keeps the current target unless another is clearly better (or the current one is gone).
	const bool bKeep = Current && CurrentScore < TNumericLimits<float>::Max() && BestScore > CurrentScore - B.SwitchDistanceAdvantage;
	if (!bKeep && Best != Current)
	{
		Target = Best;
		bSeesTarget = false;
	}
	else if (!Best)
	{
		Target = nullptr;
		bSeesTarget = false;
	}
}

void ACoralisEnemyAIController::Think(const UCoralisEnemyBehaviorSettings& B)
{
	const double Now = GetWorld()->GetTimeSeconds();
	UpdateTarget(B);
	if (!Target.IsValid())
	{
		if (State != ECoralisEnemyState::Idle)
		{
			StopMovement();
			Enemy->SetSprinting(false);
			Enemy->UnCrouch();
			bRushing = false;
			SetState(ECoralisEnemyState::Idle);
		}
		return;
	}

	const bool bSees = HasLineOfSight(Enemy->GetPawnViewLocation(), Target->GetPawnViewLocation(), B);
	if (bSees)
	{
		if (!bSeesTarget)
		{
			SeenSinceTime = Now;
		}
		LastSeenTime = Now;
	}
	bSeesTarget = bSees;

	if (State == ECoralisEnemyState::Advance || State == ECoralisEnemyState::Engage)
	{
		LookForSquad(B);
		UpdateRush(B);
	}
	UpdateDash(B);

	switch (State)
	{
	case ECoralisEnemyState::Idle:
		StartAdvance(B);
		break;

	case ECoralisEnemyState::Advance:
		ThinkAdvance(B);
		break;

	case ECoralisEnemyState::Engage:
		ThinkEngage(B);
		break;

	case ECoralisEnemyState::MoveToCover:
		if (HasArrived())
		{
			if (bHeadingToCover)
			{
				EnterCover(B);
			}
			else
			{
				StartAdvance(B);
			}
		}
		else if (Now >= StateEndTime)
		{
			StartAdvance(B);
		}
		else if (B.bSprintToCover)
		{
			// Asked every time: a sprint drops out on sharp corners.
			Enemy->SetSprinting(true);
		}
		break;

	case ECoralisEnemyState::InCover:
		if (Now >= StateEndTime)
		{
			ChooseInCover(B);
		}
		break;

	case ECoralisEnemyState::Flee:
		if (HasArrived() || Now >= StateEndTime)
		{
			StartAdvance(B);
		}
		else if (B.bSprintWhenFleeing)
		{
			Enemy->SetSprinting(true);
		}
		break;

	case ECoralisEnemyState::GroupUp:
		ThinkGroupUp(B);
		break;
	}
}

void ACoralisEnemyAIController::StartAdvance(const UCoralisEnemyBehaviorSettings& B)
{
	StopMovement();
	Enemy->SetSprinting(false);
	Enemy->UnCrouch();
	PreferredRange = FMath::FRandRange(B.PreferredRangeMin, B.PreferredRangeMax);
	NextMoveTime = 0.0;
	SetState(ECoralisEnemyState::Advance);
}

void ACoralisEnemyAIController::ThinkAdvance(const UCoralisEnemyBehaviorSettings& B)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const float Distance = DistanceToTarget();
	if (bSeesTarget && Distance <= (bRushing ? B.RushEndDistance : PreferredRange))
	{
		bRushing = false;
		StopMovement();
		Enemy->SetSprinting(false);
		NextMoveTime = 0.0;
		NextPushTime = Now + B.PushInterval;
		SetState(ECoralisEnemyState::Engage);
		return;
	}

	Enemy->SetSprinting(bRushing);

	// A new leg when the last one is done. Without sight, the leg is refreshed so it follows a moving target.
	if (!HasArrived() && (bSeesTarget || Now < NextMoveTime))
	{
		return;
	}
	NextMoveTime = Now + B.StrafeIntervalMax;

	// A squad member that fell behind catches up with the leader first.
	if (!bRushing && IsSquadFollower())
	{
		const APawn* LeaderPawn = GetSquadLeader()->GetPawn();
		FVector Goal;
		if (LeaderPawn && FVector::Dist2D(Enemy->GetActorLocation(), LeaderPawn->GetActorLocation()) > B.SquadCohesionDistance
			&& ProjectToNav(LeaderPawn->GetNavAgentLocation(), Goal) && GoTo(Goal, B.SquadCohesionDistance * 0.5f))
		{
			return;
		}
	}

	const FVector TargetLocation = Target->GetActorLocation();
	// Rushing goes straight in; running wanders from side to side.
	if (bSeesTarget && !bRushing)
	{
		const FVector ToTarget = (TargetLocation - Enemy->GetActorLocation()).GetSafeNormal2D();
		const FVector Side(-ToTarget.Y, ToTarget.X, 0.f);
		const float Length = FMath::Min(B.AdvanceLegLength, FMath::Max(0.f, Distance - PreferredRange * 0.8f));
		const FVector Wanted = Enemy->GetActorLocation() + ToTarget * Length + Side * FMath::FRandRange(-B.AdvanceSideOffset, B.AdvanceSideOffset);
		FVector Goal;
		if (ProjectToNav(Wanted, Goal) && GoTo(Goal, B.AcceptanceRadius))
		{
			return;
		}
	}
	// Round walls: head for the target itself.
	FVector Goal;
	if (ProjectToNav(TargetLocation, Goal))
	{
		GoTo(Goal, B.AcceptanceRadius);
	}
}

void ACoralisEnemyAIController::ThinkEngage(const UCoralisEnemyBehaviorSettings& B)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const bool bLost = !bSeesTarget && Now - LastSeenTime >= B.LostSightDelay;
	if (bLost || DistanceToTarget() > PreferredRange * B.ChaseRangeFactor)
	{
		NextMoveTime = 0.0;
		SetState(ECoralisEnemyState::Advance);
		return;
	}
	if (B.PushInterval > 0.f && Now >= NextPushTime)
	{
		NextPushTime = Now + B.PushInterval;
		PreferredRange = FMath::Max(B.MinPushRange, PreferredRange - B.PushDistance);
		if (DistanceToTarget() > PreferredRange)
		{
			NextMoveTime = 0.0;
			SetState(ECoralisEnemyState::Advance);
			return;
		}
	}
	if (Now < NextMoveTime)
	{
		return;
	}
	NextMoveTime = Now + FMath::FRandRange(B.StrafeIntervalMin, B.StrafeIntervalMax);

	const FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	const FVector Side(-ToTarget.Y, ToTarget.X, 0.f);
	const float First = FMath::RandBool() ? 1.f : -1.f;
	const float Length = FMath::FRandRange(B.StrafeDistanceMin, B.StrafeDistanceMax);
	for (const float Sign : { First, -First })
	{
		FVector Goal;
		// Only side steps that keep the target in sight.
		if (ProjectToNav(Enemy->GetActorLocation() + Side * Sign * Length, Goal)
			&& HasLineOfSight(Goal + FVector(0, 0, B.CoverHideHeight), Target->GetPawnViewLocation(), B)
			&& GoTo(Goal, B.AcceptanceRadius))
		{
			return;
		}
	}
}

void ACoralisEnemyAIController::HandleDamaged(const FCoralisDamageEvent& Damage)
{
	if (!Enemy || Enemy->IsDead() || Enemy->GetHealthComponent()->IsDepleted())
	{
		return;
	}
	const UCoralisEnemyBehaviorSettings& B = Enemy->GetBehavior();
	const double Now = GetWorld()->GetTimeSeconds();
	if (AController* Attacker = Enemy->GetHealthComponent()->GetLastInstigator())
	{
		LastAttacker = Attacker;
		LastAttackedTime = Now;
	}
	if (Now < NextReactionTime || State == ECoralisEnemyState::MoveToCover || State == ECoralisEnemyState::Flee)
	{
		return;
	}
	if (!Target.IsValid())
	{
		UpdateTarget(B);
	}
	if (!Target.IsValid())
	{
		return;
	}
	NextReactionTime = Now + B.ReactionCooldown;

	const float Fraction = Enemy->GetHealthComponent()->GetHealth() / Enemy->GetHealthComponent()->MaxHealth;
	if (Fraction <= B.FleeHealthFraction && FMath::FRand() < B.FleeChance && StartFlee(B))
	{
		return;
	}
	if (FMath::FRand() < B.TakeCoverChance && StartBreakContact(B))
	{
		UE_LOG(LogTemp, Verbose, TEXT("%s breaks contact (hit, %s)"), *GetNameSafe(Enemy), bHeadingToCover ? TEXT("to cover") : TEXT("backing off"));
		return;
	}
	if (FMath::FRand() < B.DodgeChance && Now - LastDashTime >= B.DashCooldown)
	{
		TrySideDash(B);
	}
}

void ACoralisEnemyAIController::HandleServerShot(const UCoralisWeaponComponent& Weapon, const FVector& Start,
	const FVector& End, const AActor* HitActor)
{
	// A hit on itself arrives as damage.
	if (!Enemy || Enemy->IsDead() || HitActor == Enemy || Weapon.GetWorld() != GetWorld())
	{
		return;
	}
	const UCoralisEnemyBehaviorSettings& B = Enemy->GetBehavior();
	const APawn* Shooter = Cast<APawn>(Weapon.GetOwner());
	if (B.IncomingFireRadius <= 0.f || !Shooter || !Shooter->IsPlayerControlled())
	{
		return;
	}
	const FVector Self = Enemy->GetActorLocation();
	// Not for someone standing behind the shooter.
	if (FVector::DotProduct(Self - Start, End - Start) <= 0.0
		|| FMath::PointDistToSegment(Self, Start, End) > B.IncomingFireRadius)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	LastAttacker = Shooter->GetController();
	LastAttackedTime = Now;

	// Hidden in cover it stays put; seen there, the cover is no good any more.
	const bool bCanReact = State == ECoralisEnemyState::Advance || State == ECoralisEnemyState::Engage
		|| State == ECoralisEnemyState::GroupUp || (State == ECoralisEnemyState::InCover && bSeesTarget);
	if (!bCanReact || (bRushing && !B.bNearMissStopsRush) || Now < NextReactionTime || Now < NextNearMissTime)
	{
		return;
	}
	NextNearMissTime = Now + B.NearMissCooldown;
	if (!Target.IsValid())
	{
		UpdateTarget(B);
	}
	if (Target.IsValid() && FMath::FRand() < B.NearMissCoverChance && StartBreakContact(B))
	{
		NextReactionTime = Now + B.ReactionCooldown;
		UE_LOG(LogTemp, Verbose, TEXT("%s breaks contact (near miss, %s)"), *GetNameSafe(Enemy), bHeadingToCover ? TEXT("to cover") : TEXT("backing off"));
	}
}

bool ACoralisEnemyAIController::StartBreakContact(const UCoralisEnemyBehaviorSettings& B)
{
	FVector Spot;
	const bool bFoundCover = FindCover(B, Spot);
	Enemy->UnCrouch();
	const bool bToCover = bFoundCover && GoTo(Spot, B.AcceptanceRadius);
	if (!bToCover && !MoveAway(B))
	{
		return false;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	bHeadingToCover = bToCover;
	bRushing = false;
	SetState(ECoralisEnemyState::MoveToCover);
	StateEndTime = Now + B.CoverTravelTimeout;
	Enemy->SetSprinting(B.bSprintToCover);
	NextBurstTime = FMath::Min(NextBurstTime, Now + B.ReturnFireDelay);
	return true;
}

void ACoralisEnemyAIController::EnterCover(const UCoralisEnemyBehaviorSettings& B)
{
	Enemy->SetSprinting(false);
	if (B.bCrouchInCover)
	{
		Enemy->Crouch();
	}
	CoverWaits = 0;
	SetState(ECoralisEnemyState::InCover);
	StateEndTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(B.CoverSettleMin, B.CoverSettleMax);
}

void ACoralisEnemyAIController::ChooseInCover(const UCoralisEnemyBehaviorSettings& B)
{
	const float WaitWeight = CoverWaits < B.MaxCoverWaits ? B.CoverWaitWeight : 0.f;
	const float Roll = FMath::FRand() * (B.CoverAttackWeight + B.CoverSeekGroupWeight + WaitWeight);
	if (Roll < WaitWeight)
	{
		++CoverWaits;
		const float Wait = FMath::FRandRange(B.CoverWaitMin, B.CoverWaitMax);
		StateEndTime = GetWorld()->GetTimeSeconds() + Wait;
		UE_LOG(LogTemp, Verbose, TEXT("%s in cover: waits %.1f s"), *GetNameSafe(Enemy), Wait);
		return;
	}
	if (Roll < WaitWeight + B.CoverSeekGroupWeight)
	{
		if (SeekGroup(B))
		{
			UE_LOG(LogTemp, Verbose, TEXT("%s in cover: seeks a group"), *GetNameSafe(Enemy));
			return;
		}
		UE_LOG(LogTemp, Verbose, TEXT("%s in cover: no group to seek"), *GetNameSafe(Enemy));
	}
	UE_LOG(LogTemp, Verbose, TEXT("%s in cover: attacks"), *GetNameSafe(Enemy));
	StartAdvance(B);
}

bool ACoralisEnemyAIController::FindCover(const UCoralisEnemyBehaviorSettings& B, FVector& OutSpot) const
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav || !Target.IsValid())
	{
		return false;
	}
	const FVector Self = Enemy->GetActorLocation();
	const FVector Feet = Enemy->GetNavAgentLocation();
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector TargetEyes = Target->GetPawnViewLocation();
	const float StartAngle = FMath::FRandRange(0.f, 360.f);

	TArray<TPair<float, FVector>> Spots;
	for (int32 Index = 0; Index < B.CoverSamples; ++Index)
	{
		const float Angle = StartAngle + Index * GoldenAngleDegrees;
		const float Radius = B.CoverSearchRadius * FMath::Sqrt((Index + 1.f) / B.CoverSamples);
		FVector Spot;
		if (!ProjectToNav(Feet + FRotator(0.f, Angle, 0.f).Vector() * Radius, Spot)
			|| FVector::Dist2D(Spot, TargetLocation) < B.CoverMinTargetDistance
			|| HasLineOfSight(TargetEyes, Spot + FVector(0, 0, B.CoverHideHeight), B))
		{
			continue;
		}
		// Nearest first, and rather not a spot that means running toward the target.
		const float TowardTarget = FMath::Max(0.f, FVector::Dist2D(Self, TargetLocation) - FVector::Dist2D(Spot, TargetLocation));
		Spots.Emplace(FVector::Dist2D(Self, Spot) + TowardTarget, Spot);
	}
	Spots.Sort([](const TPair<float, FVector>& A, const TPair<float, FVector>& C) { return A.Key < C.Key; });

	for (int32 Index = 0; Index < FMath::Min(CoverPathChecks, Spots.Num()); ++Index)
	{
		const UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), Feet, Spots[Index].Value, Enemy);
		if (Path && Path->IsValid() && !Path->IsPartial() && Path->GetPathLength() <= B.CoverSearchRadius * B.CoverMaxPathFactor)
		{
			OutSpot = Spots[Index].Value;
			return true;
		}
	}
	return false;
}

bool ACoralisEnemyAIController::MoveAway(const UCoralisEnemyBehaviorSettings& B)
{
	const FVector Away = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	// A few swings around "straight away"; a wall behind shortens or turns the escape.
	for (const float Reach : { 1.f, 0.5f })
	{
		for (const float Swing : { FMath::FRandRange(-1.f, 1.f), 1.f, -1.f })
		{
			const FVector Direction = Away.RotateAngleAxis(Swing * B.FleeSpreadAngle, FVector::UpVector);
			FVector Goal;
			if (ProjectToNav(Enemy->GetNavAgentLocation() + Direction * B.FleeDistance * Reach, Goal) && GoTo(Goal, B.AcceptanceRadius))
			{
				return true;
			}
		}
	}
	return false;
}

bool ACoralisEnemyAIController::StartFlee(const UCoralisEnemyBehaviorSettings& B)
{
	Enemy->UnCrouch();
	if (!MoveAway(B))
	{
		return false;
	}
	bRushing = false;
	SetState(ECoralisEnemyState::Flee);
	StateEndTime = GetWorld()->GetTimeSeconds() + B.FleeMaxDuration;
	Enemy->SetSprinting(B.bSprintWhenFleeing);
	return true;
}

void ACoralisEnemyAIController::SetState(ECoralisEnemyState NewState)
{
	if (State != NewState)
	{
		UE_LOG(LogTemp, Verbose, TEXT("%s: %s"), *GetNameSafe(Enemy), *UEnum::GetValueAsString(NewState));
		State = NewState;
	}
}

// --- Run or rush -----------------------------------------------------------------------------------------------------

void ACoralisEnemyAIController::UpdateRush(const UCoralisEnemyBehaviorSettings& B)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (bRushing)
	{
		if (Now >= RushEndTime)
		{
			bRushing = false;
		}
		return;
	}
	// Squad members rush when their leader does.
	if (State == ECoralisEnemyState::GroupUp || Now < NextRushTime || IsSquadFollower())
	{
		return;
	}
	NextRushTime = Now + B.RushDecisionInterval;
	const float Distance = DistanceToTarget();
	if (Distance <= B.RushEndDistance || FMath::FRand() >= B.GetRushChance(Distance))
	{
		return;
	}
	for (ACoralisEnemyAIController* Member : GetSquadMembers())
	{
		if (Member != this && Member->IsAbleToFight())
		{
			Member->StartRush(Member->Enemy->GetBehavior());
		}
	}
	StartRush(B);
}

void ACoralisEnemyAIController::StartRush(const UCoralisEnemyBehaviorSettings& B)
{
	bRushing = true;
	RushEndTime = GetWorld()->GetTimeSeconds() + B.RushMaxDuration;
	// It fights from where the rush ends instead of backing off to its earlier distance.
	PreferredRange = FMath::Min(PreferredRange, B.RushEndDistance);
	// Drops the current leg so the next think heads straight in.
	StopMovement();
	NextMoveTime = 0.0;
	if (State == ECoralisEnemyState::Engage)
	{
		SetState(ECoralisEnemyState::Advance);
	}
}

bool ACoralisEnemyAIController::IsAbleToFight() const
{
	return Enemy && !Enemy->IsDead() && Target.IsValid()
		&& (State == ECoralisEnemyState::Advance || State == ECoralisEnemyState::Engage);
}

// --- Dash ------------------------------------------------------------------------------------------------------------

void ACoralisEnemyAIController::UpdateDash(const UCoralisEnemyBehaviorSettings& B)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextDashRollTime)
	{
		return;
	}
	NextDashRollTime = Now + B.DashDecisionInterval;
	if (Now - LastDashTime < B.DashCooldown)
	{
		return;
	}

	float Chance = 0.f;
	switch (State)
	{
	case ECoralisEnemyState::Advance:
		Chance = bRushing ? B.RushDashChance : B.AdvanceDashChance;
		break;
	case ECoralisEnemyState::Engage:
		Chance = B.FightDashChance;
		break;
	case ECoralisEnemyState::MoveToCover:
		Chance = B.BreakContactDashChance;
		break;
	case ECoralisEnemyState::Flee:
		Chance = B.EscapeDashChance;
		break;
	default:
		return;
	}
	if (FMath::FRand() >= Chance)
	{
		return;
	}
	if (State == ECoralisEnemyState::Engage)
	{
		TrySideDash(B);
		return;
	}
	// On the way somewhere: forward, but not when the roll would carry it past where it is going.
	const float RollDistance = Enemy->GetCoralisMovement()->GetSettings()->DodgeDistance;
	if (!HasArrived() && FVector::Dist2D(Enemy->GetActorLocation(), MoveGoal) >= RollDistance)
	{
		TryDash(Enemy->GetVelocity(), B);
	}
}

bool ACoralisEnemyAIController::TrySideDash(const UCoralisEnemyBehaviorSettings& B)
{
	if (!Target.IsValid())
	{
		return false;
	}
	const FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	const FVector Side = FVector(-ToTarget.Y, ToTarget.X, 0.f) * (FMath::RandBool() ? 1.f : -1.f);
	return TryDash(Side, B) || TryDash(-Side, B);
}

bool ACoralisEnemyAIController::TryDash(const FVector& Direction, const UCoralisEnemyBehaviorSettings& B)
{
	const UCoralisMovementComponent* Move = Enemy->GetCoralisMovement();
	const FVector Way = Direction.GetSafeNormal2D();
	if (!Move || Way.IsNearlyZero() || Move->IsDodging() || Move->IsRecovering() || !Move->IsMovingOnGround())
	{
		return false;
	}
	// No rolling into walls or off ledges.
	const FVector Feet = Enemy->GetNavAgentLocation();
	FVector Hit;
	if (UNavigationSystemV1::NavigationRaycast(GetWorld(), Feet, Feet + Way * Move->GetSettings()->DodgeDistance, Hit, nullptr, this))
	{
		return false;
	}
	LastDashTime = GetWorld()->GetTimeSeconds();
	Enemy->Dodge(Way);
	return true;
}

// --- Squads ------------------------------------------------------------------------------------------------------

TArray<ACoralisEnemyAIController*> ACoralisEnemyAIController::GetSquadMembers() const
{
	TArray<ACoralisEnemyAIController*> Members;
	if (Squad)
	{
		for (const TWeakObjectPtr<ACoralisEnemyAIController>& Member : Squad->Members)
		{
			ACoralisEnemyAIController* Brain = Member.Get();
			if (Brain && Brain->Enemy && !Brain->Enemy->IsDead())
			{
				Members.Add(Brain);
			}
		}
	}
	return Members;
}

int32 ACoralisEnemyAIController::GetSquadSize() const
{
	return FMath::Max(1, GetSquadMembers().Num());
}

ACoralisEnemyAIController* ACoralisEnemyAIController::GetSquadLeader() const
{
	const TArray<ACoralisEnemyAIController*> Members = GetSquadMembers();
	return Members.IsEmpty() ? const_cast<ACoralisEnemyAIController*>(this) : Members[0];
}

bool ACoralisEnemyAIController::IsSquadFollower() const
{
	return GetSquadLeader() != this;
}

void ACoralisEnemyAIController::JoinSquad(ACoralisEnemyAIController& Mate)
{
	if (&Mate == this || (Squad && Squad == Mate.Squad))
	{
		return;
	}
	LeaveSquad();
	if (!Mate.Squad)
	{
		Mate.Squad = MakeShared<FCoralisEnemySquad>();
		Mate.Squad->Members.Add(&Mate);
	}
	Squad = Mate.Squad;
	Squad->Members.Add(this);
}

void ACoralisEnemyAIController::LeaveSquad()
{
	if (Squad)
	{
		Squad->Members.RemoveAll([this](const TWeakObjectPtr<ACoralisEnemyAIController>& Member)
		{
			return !Member.IsValid() || Member.Get() == this;
		});
		Squad.Reset();
	}
}

void ACoralisEnemyAIController::LookForSquad(const UCoralisEnemyBehaviorSettings& B)
{
	const double Now = GetWorld()->GetTimeSeconds();
	// Only an enemy on its own goes looking, and not in the middle of a rush.
	if (!B.bCanGroupUp || bRushing || Now < NextGroupUpTime || GetSquadSize() > 1)
	{
		return;
	}
	NextGroupUpTime = Now + B.GroupUpDecisionInterval;

	ACoralisEnemyAIController* Mate = FindSquadMate(B);
	// The chance is only rolled when there is an opportunity.
	if (Mate && FMath::FRand() < B.GroupUpChance)
	{
		JoinUp(*Mate, B);
	}
}

ACoralisEnemyAIController* ACoralisEnemyAIController::FindSquadMate(const UCoralisEnemyBehaviorSettings& B) const
{
	const FVector Self = Enemy->GetActorLocation();
	const float OwnTargetDistance = DistanceToTarget();
	ACoralisEnemyAIController* Best = nullptr;
	float BestDistance = B.GroupUpSearchRadius;
	for (TActorIterator<ACoralisEnemyCharacter> It(GetWorld()); It; ++It)
	{
		ACoralisEnemyAIController* Mate = Cast<ACoralisEnemyAIController>(It->GetController());
		if (!Mate || Mate == this || It->IsDead()
			|| Mate->State == ECoralisEnemyState::Flee || Mate->State == ECoralisEnemyState::Idle
			|| Mate->GetSquadSize() >= B.SquadMaxSize)
		{
			continue;
		}
		const float Distance = FVector::Dist(Self, It->GetActorLocation());
		if (Distance >= BestDistance
			|| FVector::Dist(It->GetActorLocation(), Target->GetActorLocation()) > OwnTargetDistance + B.GroupUpMaxBacktrack)
		{
			continue;
		}
		Best = Mate;
		BestDistance = Distance;
	}
	return Best;
}

bool ACoralisEnemyAIController::JoinUp(ACoralisEnemyAIController& Mate, const UCoralisEnemyBehaviorSettings& B)
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav)
	{
		return false;
	}
	const UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), Enemy->GetNavAgentLocation(),
		Mate.Enemy->GetNavAgentLocation(), Enemy);
	if (!Path || !Path->IsValid() || Path->IsPartial() || Path->GetPathLength() > B.GroupUpSearchRadius * B.GroupUpMaxPathFactor)
	{
		return false;
	}

	JoinSquad(Mate);
	Squad->bGathering = true;
	Squad->GatherEndTime = GetWorld()->GetTimeSeconds() + B.GatherTimeout;
	StopMovement();
	Enemy->UnCrouch();
	SetState(ECoralisEnemyState::GroupUp);
	// The leader stops to wait, if it is free to.
	ACoralisEnemyAIController* Leader = GetSquadLeader();
	if (Leader != this && Leader->IsAbleToFight())
	{
		Leader->bRushing = false;
		Leader->StopMovement();
		Leader->Enemy->SetSprinting(false);
		Leader->SetState(ECoralisEnemyState::GroupUp);
	}
	UE_LOG(LogTemp, Verbose, TEXT("%s joins %s (squad of %d)"), *GetNameSafe(Enemy), *GetNameSafe(Leader->Enemy), GetSquadSize());
	return true;
}

bool ACoralisEnemyAIController::SeekGroup(const UCoralisEnemyBehaviorSettings& B)
{
	if (!Target.IsValid())
	{
		return false;
	}
	if (GetSquadSize() > 1)
	{
		// Already in a squad: worth a trip only if it has drifted away from the leader.
		const APawn* LeaderPawn = IsSquadFollower() ? GetSquadLeader()->GetPawn() : nullptr;
		if (!LeaderPawn || FVector::Dist2D(Enemy->GetActorLocation(), LeaderPawn->GetActorLocation()) <= B.GatherDistance)
		{
			return false;
		}
		Squad->bGathering = true;
		Squad->GatherEndTime = GetWorld()->GetTimeSeconds() + B.GatherTimeout;
		StopMovement();
		Enemy->UnCrouch();
		SetState(ECoralisEnemyState::GroupUp);
		return true;
	}
	ACoralisEnemyAIController* Mate = B.bCanGroupUp ? FindSquadMate(B) : nullptr;
	return Mate && JoinUp(*Mate, B);
}

void ACoralisEnemyAIController::ThinkGroupUp(const UCoralisEnemyBehaviorSettings& B)
{
	const double Now = GetWorld()->GetTimeSeconds();
	// Gathering is over: launched, timed out, or nobody left to gather with.
	if (!Squad || !Squad->bGathering || GetSquadSize() < 2)
	{
		StartAdvance(B);
		return;
	}
	const TArray<ACoralisEnemyAIController*> Members = GetSquadMembers();
	const ACoralisEnemyAIController* Leader = Members[0];

	bool bGathered = true;
	for (const ACoralisEnemyAIController* Member : Members)
	{
		// Members busy hiding or running are not waited for.
		if (Member->State == ECoralisEnemyState::GroupUp
			&& FVector::Dist2D(Member->Enemy->GetActorLocation(), Leader->Enemy->GetActorLocation()) > B.GatherDistance)
		{
			bGathered = false;
		}
	}
	if (bGathered || Now >= Squad->GatherEndTime)
	{
		LaunchSquad(B);
		return;
	}

	if (Leader == this)
	{
		StopMovement();
		Enemy->SetSprinting(false);
		return;
	}
	if (FVector::Dist2D(Enemy->GetActorLocation(), Leader->Enemy->GetActorLocation()) <= B.GatherDistance * 0.8f)
	{
		StopMovement();
		Enemy->SetSprinting(false);
		return;
	}
	FVector Goal;
	if (ProjectToNav(Leader->Enemy->GetNavAgentLocation(), Goal)
		&& (HasArrived() || FVector::Dist2D(Goal, MoveGoal) > B.GatherDistance * 0.5f))
	{
		GoTo(Goal, B.GatherDistance * 0.5f);
	}
	Enemy->SetSprinting(B.bSprintToGroupUp);
}

void ACoralisEnemyAIController::LaunchSquad(const UCoralisEnemyBehaviorSettings& B)
{
	Squad->bGathering = false;
	const bool bRushIn = FMath::FRand() < B.RushAfterGatherChance;
	const double Now = GetWorld()->GetTimeSeconds();
	int32 Launched = 0;
	for (ACoralisEnemyAIController* Member : GetSquadMembers())
	{
		if (Member->State != ECoralisEnemyState::GroupUp)
		{
			continue;
		}
		const UCoralisEnemyBehaviorSettings& MemberB = Member->Enemy->GetBehavior();
		Member->StartAdvance(MemberB);
		Member->NextRushTime = Now + MemberB.RushDecisionInterval;
		if (bRushIn && Member->DistanceToTarget() > MemberB.RushEndDistance)
		{
			Member->StartRush(MemberB);
		}
		++Launched;
	}
	UE_LOG(LogTemp, Verbose, TEXT("Squad of %d goes in%s"), Launched, bRushIn ? TEXT(", rushing") : TEXT(""));
}

// --- Aim and fire ----------------------------------------------------------------------------------------------------

void ACoralisEnemyAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// The movement component turns the body from this; the pawn is never rotated from here.
	if (!Enemy || Enemy->IsDead())
	{
		return;
	}
	const UCoralisEnemyBehaviorSettings& B = Enemy->GetBehavior();
	const UCoralisMovementComponent* Move = Enemy->GetCoralisMovement();
	const bool bRunning = State == ECoralisEnemyState::Flee || (Move && Move->IsSprinting());
	const FVector Velocity = Enemy->GetVelocity();

	FRotator Wanted = GetControlRotation();
	if (!bRunning && Target.IsValid())
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (Now >= NextAimErrorTime)
		{
			NextAimErrorTime = Now + B.AimErrorInterval;
			const float Error = State == ECoralisEnemyState::MoveToCover ? B.BreakContactAimErrorDegrees : B.AimErrorDegrees;
			AimError = FRotator(FMath::FRandRange(-Error, Error), FMath::FRandRange(-Error, Error), 0.f);
		}
		const FVector AimPoint = Target->GetActorLocation() + FVector(0, 0, B.AimHeightOffset);
		Wanted = (AimPoint - Enemy->GetPawnViewLocation()).Rotation() + AimError;
	}
	else if (Velocity.SizeSquared2D() > 1.f)
	{
		Wanted = FRotator(0.f, Velocity.Rotation().Yaw, 0.f);
	}
	SetControlRotation(FMath::RInterpConstantTo(GetControlRotation(), Wanted.GetNormalized(), DeltaTime, B.AimTurnRate));
}

void ACoralisEnemyAIController::UpdateShooting(const UCoralisEnemyBehaviorSettings& B)
{
	UCoralisWeaponComponent* Weapon = Enemy->GetWeapon();
	const UCoralisMovementComponent* Move = Enemy->GetCoralisMovement();
	const double Now = GetWorld()->GetTimeSeconds();

	bool bMayShoot = B.bCanShoot && Weapon->Settings && Target.IsValid() && !Target->IsDead() && bSeesTarget
		&& (State == ECoralisEnemyState::Advance || State == ECoralisEnemyState::Engage || State == ECoralisEnemyState::GroupUp
			|| (State == ECoralisEnemyState::MoveToCover && B.bShootWhileBreakingContact))
		&& Move && Move->CanUseWeapon() && !Move->IsSprinting()
		&& Now - SeenSinceTime >= B.ReactionTime && DistanceToTarget() <= B.FireRange;
	if (bMayShoot)
	{
		const FVector ToTarget = (Target->GetActorLocation() - Enemy->GetPawnViewLocation()).GetSafeNormal();
		bMayShoot = FVector::DotProduct(GetControlRotation().Vector(), ToTarget) >= FMath::Cos(FMath::DegreesToRadians(B.FireAngleTolerance));
	}

	const bool bReady = Weapon->Settings && Weapon->GetHeatPhase() == ECoralisHeatPhase::Ready;
	if (bBursting && (!bMayShoot || !bReady || Now >= BurstEndTime || !Weapon->IsTriggerHeld()))
	{
		Weapon->StopFire();
		bBursting = false;
		NextBurstTime = Now + FMath::FRandRange(B.BurstPauseMin, B.BurstPauseMax);
	}
	if (!bBursting && bReady && Weapon->GetHeatFraction() >= B.VentAtHeat)
	{
		Weapon->Vent();
	}
	else if (!bBursting && bMayShoot && bReady && Now >= NextBurstTime)
	{
		bBursting = true;
		BurstEndTime = Now + FMath::FRandRange(B.BurstDurationMin, B.BurstDurationMax);
		Weapon->StartFire();
	}
	Enemy->SetAiming(B.bAimWhenShooting && bBursting && State == ECoralisEnemyState::Engage);
}

// --- Helpers ---------------------------------------------------------------------------------------------------------

bool ACoralisEnemyAIController::GoTo(const FVector& Goal, float AcceptanceRadius)
{
	MoveGoal = Goal;
	// No focus and strafing allowed: where it looks comes from UpdateControlRotation, not from the path.
	return MoveToLocation(Goal, AcceptanceRadius, false, true, false, true) != EPathFollowingRequestResult::Failed;
}

bool ACoralisEnemyAIController::HasArrived() const
{
	return GetMoveStatus() != EPathFollowingStatus::Moving;
}

bool ACoralisEnemyAIController::HasLineOfSight(const FVector& From, const FVector& To, const UCoralisEnemyBehaviorSettings& B) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CoralisEnemySight), false);
	Params.AddIgnoredActor(Enemy);
	Params.AddIgnoredActor(Target.Get());
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, From, To, B.SightChannel, Params);
}

bool ACoralisEnemyAIController::ProjectToNav(const FVector& Point, FVector& Out) const
{
	const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Found;
	if (!Nav || !Nav->ProjectPointToNavigation(Point, Found, NavSearchExtent))
	{
		return false;
	}
	Out = Found.Location;
	return true;
}

float ACoralisEnemyAIController::DistanceToTarget() const
{
	return Target.IsValid() ? FVector::Dist(Enemy->GetActorLocation(), Target->GetActorLocation()) : TNumericLimits<float>::Max();
}
