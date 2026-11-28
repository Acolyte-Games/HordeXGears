// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoralisEnemyBehaviorSettings.generated.h"

/** One distance band of the rush decision. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisRushBand
{
	GENERATED_BODY()

	/** The band starts at this distance from the target and ends where the next one starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rush", meta = (ClampMin = "0", Units = "cm"))
	float FromDistance = 0.f;

	/** Chance to start a rush, each time the enemy decides while in this band. 0 = never, 1 = always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rush", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 0.f;
};

/**
 * How a ranged enemy fights. One asset per enemy kind; edits apply from the next decision, also while playing.
 * Distances are in cm, times in seconds. Speeds, rifle and health are set on the enemy Blueprint.
 */
UCLASS(BlueprintType)
class WAVESHOOTERPROTO_API UCoralisEnemyBehaviorSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UCoralisEnemyBehaviorSettings();

	/** Chance to start a rush at this distance: the band with the largest From Distance not beyond it. */
	UFUNCTION(BlueprintPure, Category = "Rush")
	float GetRushChance(float Distance) const;

	// --- Targeting -------------------------------------------------------------------------------------------------------

	/** Time between decisions. Aiming still updates every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.02", Units = "Seconds"))
	float DecisionInterval = 0.2f;

	/** Another player has to be this much closer before the enemy turns to them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0", Units = "cm"))
	float SwitchDistanceAdvantage = 300.f;

	/** The player who last hit this enemy counts as this much closer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0", Units = "cm"))
	float AttackerPriorityDistance = 800.f;

	/** How long a hit keeps that priority. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0", Units = "Seconds"))
	float AttackerMemory = 4.f;

	/** What blocks the enemy's sight, also used to judge cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	TEnumAsByte<ECollisionChannel> SightChannel = ECC_Visibility;

	// --- Advancing -------------------------------------------------------------------------------------------------------

	/** Each approach picks a fighting distance between these two. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "0", Units = "cm"))
	float PreferredRangeMin = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "0", Units = "cm"))
	float PreferredRangeMax = 1400.f;

	/** Length of one leg of the approach while the target is in sight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "50", Units = "cm"))
	float AdvanceLegLength = 600.f;

	/** How far each leg may wander to either side, so they don't walk in a straight line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "0", Units = "cm"))
	float AdvanceSideOffset = 450.f;

	/** The target may drift this far past the fighting distance before the enemy follows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "1"))
	float ChaseRangeFactor = 1.25f;

	/** Time without sight of the target before the enemy moves to get it back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "0", Units = "Seconds"))
	float LostSightDelay = 0.8f;

	/** While fighting, every so often the enemy pushes closer. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "0", Units = "Seconds"))
	float PushInterval = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "0", Units = "cm"))
	float PushDistance = 200.f;

	/** Pushing never brings the fighting distance below this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "0", Units = "cm"))
	float MinPushRange = 450.f;

	/** How close counts as arrived. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advancing", meta = (ClampMin = "0", Units = "cm"))
	float AcceptanceRadius = 60.f;

	// --- Run or rush ---------------------------------------------------------------------------------------------------
	// Running = jogging closer while able to shoot. Rushing = sprinting at the target without shooting.

	/** Chance of rushing by distance to the target. Empty = never rushes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rush", meta = (TitleProperty = "FromDistance"))
	TArray<FCoralisRushBand> RushChanceByDistance;

	/** Time between two rush decisions. Together with the chance this sets how eager the enemy is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rush", meta = (ClampMin = "0.1", Units = "Seconds"))
	float RushDecisionInterval = 2.f;

	/** A rush ends once the target is in sight and this close. No rush starts from closer than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rush", meta = (ClampMin = "0", Units = "cm"))
	float RushEndDistance = 800.f;

	/** A rush that gets nowhere is dropped after this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rush", meta = (ClampMin = "0", Units = "Seconds"))
	float RushMaxDuration = 6.f;

	// --- Dash (the same dodge roll the players have; its distance and speed are in the enemy's movement asset) ------------

	/** Time between two dash decisions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "0.1", Units = "Seconds"))
	float DashDecisionInterval = 1.f;

	/** At least this long between two dashes of one enemy, also after a dash from being hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "0", Units = "Seconds"))
	float DashCooldown = 3.f;

	/** Chances per decision, by what the enemy is doing. Running closer: dashes forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "0", ClampMax = "1"))
	float AdvanceDashChance = 0.1f;

	/** While rushing: dashes forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "0", ClampMax = "1"))
	float RushDashChance = 0.25f;

	/** While fighting at its distance: dashes sideways. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "0", ClampMax = "1"))
	float FightDashChance = 0.12f;

	/** While running away: dashes the way it is going. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "0", ClampMax = "1"))
	float EscapeDashChance = 0.3f;

	/** While breaking contact: dashes the way it is going. It can't shoot during a dash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "0", ClampMax = "1"))
	float BreakContactDashChance = 0.1f;

	// --- Grouping up -----------------------------------------------------------------------------------------------------

	/** Whether this enemy looks for others to fight with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up")
	bool bCanGroupUp = true;

	/** Time between two looks for a friendly to join. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "0.1", Units = "Seconds"))
	float GroupUpDecisionInterval = 3.f;

	/** Chance to join up, each time there is an opportunity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "0", ClampMax = "1"))
	float GroupUpChance = 0.4f;

	/** Opportunity: a friendly within this distance... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "0", Units = "cm"))
	float GroupUpSearchRadius = 2000.f;

	/** ...whose walk there is at most this many times the search radius... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "1"))
	float GroupUpMaxPathFactor = 1.5f;

	/** ...who is not further from the target than this much more than this enemy, so joining isn't a retreat... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "0", Units = "cm"))
	float GroupUpMaxBacktrack = 600.f;

	/** ...and whose squad has fewer members than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "2"))
	int32 SquadMaxSize = 4;

	/** The squad counts as gathered when everyone coming is this close to its leader. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "100", Units = "cm"))
	float GatherDistance = 450.f;

	/** The squad stops waiting for stragglers after this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "0", Units = "Seconds"))
	float GatherTimeout = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up")
	bool bSprintToGroupUp = true;

	/** Chance that a squad rushes together as soon as it has gathered. Later rushes follow the leader's decisions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "0", ClampMax = "1"))
	float RushAfterGatherChance = 0.6f;

	/** A squad member further than this from the leader moves back to the leader while closing in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up", meta = (ClampMin = "100", Units = "cm"))
	float SquadCohesionDistance = 800.f;

	/** The whole squad goes for the leader's target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group Up")
	bool bSquadSharesTarget = true;

	// --- Moving while fighting ---------------------------------------------------------------------------------------

	/** Time between sideways moves while fighting (random between the two). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strafing", meta = (ClampMin = "0.1", Units = "Seconds"))
	float StrafeIntervalMin = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strafing", meta = (ClampMin = "0.1", Units = "Seconds"))
	float StrafeIntervalMax = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strafing", meta = (ClampMin = "0", Units = "cm"))
	float StrafeDistanceMin = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strafing", meta = (ClampMin = "0", Units = "cm"))
	float StrafeDistanceMax = 500.f;

	// --- Shooting --------------------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting")
	bool bCanShoot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0", Units = "cm"))
	float FireRange = 3000.f;

	/** Time the target has to be in sight before the first burst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0", Units = "Seconds"))
	float ReactionTime = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0", Units = "Seconds"))
	float BurstDurationMin = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0", Units = "Seconds"))
	float BurstDurationMax = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0", Units = "Seconds"))
	float BurstPauseMin = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0", Units = "Seconds"))
	float BurstPauseMax = 1.6f;

	/** Uses the aiming stance while standing and shooting: tighter spread, slower movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting")
	bool bAimWhenShooting = false;

	/** Aim point above the middle of the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (Units = "cm"))
	float AimHeightOffset = 20.f;

	/** The enemy's aim wanders up to this far from the aim point, on top of the rifle's spread. Higher = misses more. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0", Units = "Degrees"))
	float AimErrorDegrees = 3.f;

	/** How often the wander picks a new point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0.02", Units = "Seconds"))
	float AimErrorInterval = 0.35f;

	/** How fast the enemy swings its aim, degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "1"))
	float AimTurnRate = 260.f;

	/** Only shoots once the aim is within this angle of the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0", Units = "Degrees"))
	float FireAngleTolerance = 15.f;

	/** Vents between bursts once heat passes this fraction. Above 1 = never vents, and jams now and then. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0"))
	float VentAtHeat = 0.7f;

	// --- Reaction to incoming fire ----------------------------------------------------------------------------------------
	// Breaking contact = heading for cover while shooting back from the hip.

	/** Time after a reaction during which further hits and near misses are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0", Units = "Seconds"))
	float ReactionCooldown = 2.f;

	/** Chance that a hit makes the enemy break contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0", ClampMax = "1"))
	float TakeCoverChance = 0.9f;

	/** A player's shot passing or landing this close counts as being shot at. 0 = only hits count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0", Units = "cm"))
	float IncomingFireRadius = 200.f;

	/** Chance that a near miss makes the enemy break contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0", ClampMax = "1"))
	float NearMissCoverChance = 0.6f;

	/** Time between two near miss rolls, so automatic fire doesn't roll on every shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0", Units = "Seconds"))
	float NearMissCooldown = 1.f;

	/** Off = a rushing enemy ignores near misses. Hits still count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction")
	bool bNearMissStopsRush = false;

	/** If a hit doesn't make it break contact: chance of a side dash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0", ClampMax = "1"))
	float DodgeChance = 0.35f;

	/** Below this fraction of health a hit may make the enemy run away instead, without shooting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0", ClampMax = "1"))
	float FleeHealthFraction = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction", meta = (ClampMin = "0", ClampMax = "1"))
	float FleeChance = 0.6f;

	// --- Cover (placeholder until there is a cover system) ---------------------------------------------------------------

	/** Shoots back while on the way, without the aiming stance. Needs sight of the target, and doesn't work while sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	bool bShootWhileBreakingContact = true;

	/** Longest wait before the first shot back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "0", Units = "Seconds"))
	float ReturnFireDelay = 0.2f;

	/** Replaces Aim Error Degrees while breaking contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "0", Units = "Degrees"))
	float BreakContactAimErrorDegrees = 5.f;

	/** How far to look for a spot the target can't see. With none found it backs off by Flee Distance instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "0", Units = "cm"))
	float CoverSearchRadius = 1500.f;

	/** Spots tried per search. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "1", ClampMax = "256"))
	int32 CoverSamples = 64;

	/** A spot counts as cover when the target's eyes can't see this height above its floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "0", Units = "cm"))
	float CoverHideHeight = 100.f;

	/** Cover closer to the target than this is ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "0", Units = "cm"))
	float CoverMinTargetDistance = 450.f;

	/** The walk to a spot may be at most this many times the search radius, so cover behind a long wall is skipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "1"))
	float CoverMaxPathFactor = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	bool bCrouchInCover = true;

	/** On = runs to cover instead of walking, without shooting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	bool bSprintToCover = false;

	/** Gives up on the way to cover after this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover", meta = (ClampMin = "0", Units = "Seconds"))
	float CoverTravelTimeout = 10.f;

	// --- What next, in cover -----------------------------------------------------------------------------------------

	/** Time to settle in cover before choosing what next (random between the two). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "In Cover", meta = (ClampMin = "0", Units = "Seconds"))
	float CoverSettleMin = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "In Cover", meta = (ClampMin = "0", Units = "Seconds"))
	float CoverSettleMax = 1.2f;

	/** The three choices are picked by weight: a choice with twice the weight comes up twice as often. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "In Cover", meta = (ClampMin = "0"))
	float CoverAttackWeight = 0.4f;

	/** Joins a friendly, or goes back to its squad. With neither to be had it attacks instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "In Cover", meta = (ClampMin = "0"))
	float CoverSeekGroupWeight = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "In Cover", meta = (ClampMin = "0"))
	float CoverWaitWeight = 0.3f;

	/** How long a wait lasts (random between the two). It chooses again afterwards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "In Cover", meta = (ClampMin = "0", Units = "Seconds"))
	float CoverWaitMin = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "In Cover", meta = (ClampMin = "0", Units = "Seconds"))
	float CoverWaitMax = 10.f;

	/** Waits allowed during one stay in cover, so it can't hide forever. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "In Cover", meta = (ClampMin = "0"))
	int32 MaxCoverWaits = 1;

	// --- Running away ------------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee", meta = (ClampMin = "0", Units = "cm"))
	float FleeDistance = 1200.f;

	/** The escape direction may swing this far to either side of straight away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float FleeSpreadAngle = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee", meta = (ClampMin = "0", Units = "Seconds"))
	float FleeMaxDuration = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
	bool bSprintWhenFleeing = true;
};
