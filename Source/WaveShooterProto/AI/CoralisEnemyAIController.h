// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Combat/CoralisHealthComponent.h"
#include "CoralisEnemyAIController.generated.h"

class ACoralisCharacterBase;
class ACoralisEnemyCharacter;
class UCoralisEnemyBehaviorSettings;
class UCoralisWeaponComponent;
struct FCoralisEnemySquad;

UENUM(BlueprintType)
enum class ECoralisEnemyState : uint8
{
	/** Nobody left to fight. */
	Idle,
	/** Closing in on the target. */
	Advance,
	/** At fighting distance: strafing and shooting. */
	Engage,
	/** Breaking contact: heading for a spot the target can't see, shooting back on the way. */
	MoveToCover,
	InCover,
	/** Running away from the target. */
	Flee,
	/** Joining up with friendlies, or waiting for them. */
	GroupUp
};

/**
 * Server brain of a ranged enemy: picks a player, closes in by running or rushing, strafes and shoots in bursts,
 * dashes and joins up with friendlies. Under fire it breaks contact (heads for cover while shooting back), then attacks
 * again, seeks a group or waits. Badly hurt it may flee. All numbers come from the enemy's behaviour asset.
 * Cover is a placeholder: any reachable spot the target can't see.
 */
UCLASS()
class WAVESHOOTERPROTO_API ACoralisEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACoralisEnemyAIController();

	/** Server only. Joins the other one's squad, starting one if it has none. */
	void JoinSquad(ACoralisEnemyAIController& Mate);

	void LeaveSquad();

	/** Living members of its squad, itself included; 1 when on its own. */
	UFUNCTION(BlueprintPure, Category = "Coralis|AI")
	int32 GetSquadSize() const;

	/** The one its squad follows; itself when on its own. */
	UFUNCTION(BlueprintPure, Category = "Coralis|AI")
	ACoralisEnemyAIController* GetSquadLeader() const;

	UFUNCTION(BlueprintPure, Category = "Coralis|AI")
	bool IsRushing() const { return bRushing; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

	UFUNCTION(BlueprintPure, Category = "Coralis|AI")
	ECoralisEnemyState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Coralis|AI")
	APawn* GetTargetPawn() const;

	/** Where it is heading (valid while it moves somewhere). */
	UFUNCTION(BlueprintPure, Category = "Coralis|AI")
	FVector GetMoveGoal() const { return MoveGoal; }

	UFUNCTION(BlueprintPure, Category = "Coralis|AI")
	bool CanSeeTarget() const { return bSeesTarget; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	UFUNCTION()
	void HandleDamaged(const FCoralisDamageEvent& Damage);

	/** A shot from any weapon: reacts to a player's near miss. */
	void HandleServerShot(const UCoralisWeaponComponent& Weapon, const FVector& Start, const FVector& End, const AActor* HitActor);

	void Think(const UCoralisEnemyBehaviorSettings& B);
	void UpdateTarget(const UCoralisEnemyBehaviorSettings& B);
	void UpdateShooting(const UCoralisEnemyBehaviorSettings& B);
	void ThinkAdvance(const UCoralisEnemyBehaviorSettings& B);
	void ThinkEngage(const UCoralisEnemyBehaviorSettings& B);
	void ThinkGroupUp(const UCoralisEnemyBehaviorSettings& B);

	void UpdateRush(const UCoralisEnemyBehaviorSettings& B);
	void StartRush(const UCoralisEnemyBehaviorSettings& B);
	void UpdateDash(const UCoralisEnemyBehaviorSettings& B);
	/** Dashes that way if there is walkable ground for the whole roll. */
	bool TryDash(const FVector& Direction, const UCoralisEnemyBehaviorSettings& B);
	bool TrySideDash(const UCoralisEnemyBehaviorSettings& B);
	void LookForSquad(const UCoralisEnemyBehaviorSettings& B);
	/** The nearest friendly worth joining, if any. */
	ACoralisEnemyAIController* FindSquadMate(const UCoralisEnemyBehaviorSettings& B) const;
	/** Joins it and starts gathering; false if there is no good way there. */
	bool JoinUp(ACoralisEnemyAIController& Mate, const UCoralisEnemyBehaviorSettings& B);
	/** Back to its own squad's leader, or off to join a friendly; false if neither applies. */
	bool SeekGroup(const UCoralisEnemyBehaviorSettings& B);
	/** Ends the waiting: everybody who gathered goes in together. */
	void LaunchSquad(const UCoralisEnemyBehaviorSettings& B);
	/** Living members with a body, leader first. */
	TArray<ACoralisEnemyAIController*> GetSquadMembers() const;
	bool IsSquadFollower() const;
	bool IsAbleToFight() const;

	void SetState(ECoralisEnemyState NewState);
	void StartAdvance(const UCoralisEnemyBehaviorSettings& B);
	/** Heads for cover, or backs off if there is none. */
	bool StartBreakContact(const UCoralisEnemyBehaviorSettings& B);
	void EnterCover(const UCoralisEnemyBehaviorSettings& B);
	/** Attack, seek a group or wait. */
	void ChooseInCover(const UCoralisEnemyBehaviorSettings& B);
	/** Moves to a point away from the target. */
	bool MoveAway(const UCoralisEnemyBehaviorSettings& B);
	bool StartFlee(const UCoralisEnemyBehaviorSettings& B);
	bool FindCover(const UCoralisEnemyBehaviorSettings& B, FVector& OutSpot) const;
	/** Walks to the point; false if there is no way there. */
	bool GoTo(const FVector& Goal, float AcceptanceRadius);
	bool HasArrived() const;

	bool HasLineOfSight(const FVector& From, const FVector& To, const UCoralisEnemyBehaviorSettings& B) const;
	bool ProjectToNav(const FVector& Point, FVector& Out) const;
	float DistanceToTarget() const;

	UPROPERTY(Transient)
	TObjectPtr<ACoralisEnemyCharacter> Enemy;

	TWeakObjectPtr<ACoralisCharacterBase> Target;
	TWeakObjectPtr<AController> LastAttacker;

	ECoralisEnemyState State = ECoralisEnemyState::Idle;
	FVector MoveGoal = FVector::ZeroVector;
	bool bSeesTarget = false;
	bool bBursting = false;
	float PreferredRange = 0.f;

	double NextThinkTime = 0.0;
	double StateEndTime = 0.0;
	double NextMoveTime = 0.0;
	double NextPushTime = 0.0;
	double SeenSinceTime = 0.0;
	double LastSeenTime = 0.0;
	double BurstEndTime = 0.0;
	double NextBurstTime = 0.0;
	double NextReactionTime = 0.0;
	double LastAttackedTime = -1000.0;
	double NextAimErrorTime = 0.0;
	FRotator AimError = FRotator::ZeroRotator;

	FDelegateHandle ServerShotHandle;
	double NextNearMissTime = 0.0;
	/** False while breaking contact with no cover found. */
	bool bHeadingToCover = false;
	int32 CoverWaits = 0;

	TSharedPtr<FCoralisEnemySquad> Squad;
	bool bRushing = false;
	double RushEndTime = 0.0;
	double NextRushTime = 0.0;
	double NextDashRollTime = 0.0;
	double LastDashTime = -1000.0;
	double NextGroupUpTime = 0.0;
};
