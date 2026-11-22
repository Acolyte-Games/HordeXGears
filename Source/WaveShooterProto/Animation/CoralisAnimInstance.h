// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "CoralisAnimInstance.generated.h"

/**
 * Parent class for Coralis anim Blueprints. Fills plain variables from the character every frame so the graph only
 * reads variables (works the same on the owning client, the server and other players' proxies).
 */
UCLASS()
class WAVESHOOTERPROTO_API UCoralisAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** Horizontal speed (cm/s). */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoSpeed = 0.f;

	/** Move direction relative to facing (-180..180 deg). */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoDirection = 0.f;

	/**
	 * LocoDirection snapped to forward / left / right / back (0, -90, 90, 180), sticky so diagonals don't flicker.
	 * Feed this to blendspaces that only have cardinal samples, and turn the hips by LocoWarpAngle (orientation warping).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoCardinalAngle = 0.f;

	/**
	 * Direction to feed the locomotion blendspace: eases toward LocoCardinalAngle (or toward LocoDirection while aiming)
	 * at DirectionBlendRate, so quick turns and camera whips cross-fade instead of snapping to another animation.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoBlendDirection = 0.f;

	/** Travel direction minus LocoBlendDirection (deg). Turns the legs so the feet always point where the character goes. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoWarpAngle = 0.f;

	/** Which jog clip the legs play when not aiming: 0 forward, 1 right, 2 back, 3 left. Matches LocoCardinalAngle. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	int32 LocoCardinalIndex = 0;

	/**
	 * Leg turn for each jog clip (deg): forward, right, back, left. Each turns its own clip so the feet point along the
	 * travel direction, so a cross-fade between two clips blends two poses that already agree (no over-correcting).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoWarpForward = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoWarpRight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoWarpBack = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoWarpLeft = 0.f;

	/** True while jogging on the four clips: moving, not aiming, not sprinting, not rolling, on the ground. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	bool bLocoUseClips = false;

	/** Torso twist toward the look direction (yaw, deg). Standing it covers the gap until the legs catch up. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Aim")
	float TorsoTwistYaw = 0.f;

	/** Play rate for the jog clips so the feet keep pace with the actual speed (also while picking up speed). */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float LocoPlayRate = 1.f;

	/** Roadie run lean, 0..1, eased in over SprintLeanInTime and out over SprintLeanOutTime. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Sprint")
	float SprintLeanAlpha = 0.f;

	/** Forward bend applied to each spine bone in the lean chain right now (deg) = SprintLeanAlpha * SprintLeanDegreesPerBone. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Sprint")
	float SprintLeanBoneRoll = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	bool bLocoShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	bool bLocoFalling = false;

	/** Aim pitch (-90..90 deg, up positive). */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|Locomotion")
	float AimPitch = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|State")
	bool bAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|State")
	bool bSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|State")
	bool bCrouching = false;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|State")
	bool bDodging = false;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|State")
	bool bRecovering = false;

	/** Can the weapon fire in the current movement state. */
	UPROPERTY(BlueprintReadOnly, Category = "Coralis|State")
	bool bCanUseWeapon = true;

	UPROPERTY(BlueprintReadOnly, Category = "Coralis|State")
	FGameplayTagContainer StateTags;

	/** Speed above which the graph should treat the character as moving (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0"))
	float MoveSpeedThreshold = 3.f;

	/** How much closer another cardinal must be before switching to it (deg), so a held diagonal stays on one animation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0", ClampMax = "40", Units = "Degrees"))
	float CardinalHysteresis = 10.f;

	/** How fast the legs turn toward the travel direction, and back to straight when stopping (deg/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "1", Units = "DegreesPerSecond"))
	float WarpTurnRate = 360.f;

	/** Speed at which the legs are fully turned toward travel; below it the turn scales down (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0", Units = "CentimetersPerSecond"))
	float WarpFullSpeed = 120.f;

	/** Ground speed the jog clips are made for (cm/s); LocoPlayRate = speed / this. The rifle jogs cover about 290 cm/s. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "1", Units = "CentimetersPerSecond"))
	float CardinalClipSpeed = 290.f;

	/** Play rate limits for the jog clips (slowest while starting, fastest above clip speed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0"))
	float MinLocoPlayRate = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0"))
	float MaxLocoPlayRate = 1.5f;

	/** Largest torso twist toward the look direction (deg). Beyond it the legs turn (see movement Idle Torso Turn Limit). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Aim", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float TorsoTwistLimit = 90.f;

	/** How fast the torso twist follows (deg/s); smooths the replicated value on other players. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Aim", meta = (ClampMin = "1", Units = "DegreesPerSecond"))
	float TorsoTwistRate = 720.f;

	/**
	 * Leg turn limit for the forward and side jog clips (deg). Small limits keep the hips from turning away before the
	 * next clip takes over; the feet slide a little on in-between directions instead.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float MaxClipWarpAngle = 50.f;

	/**
	 * Leg turn limit for the backpedal clip, and for the side clips when turning toward the back (deg). A backpedal
	 * moving diagonally has its hips facing the opposite diagonal, so large turns here make the hips swing the wrong
	 * way around a switch between side jog and backpedal.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float MaxBackClipWarpAngle = 20.f;

	/**
	 * Clip the left branch plays, as its travel angle (deg). 0 = the forward jog turned left by the leg warp: the
	 * available left clips step across with the toes pointing right, which looks broken once turned.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "-180", ClampMax = "180", Units = "Degrees"))
	float LeftClipTravelAngle = 0.f;

	/** Leg turn limit for the left branch (deg); needs about 90 when it plays the forward jog. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float MaxLeftClipWarpAngle = 95.f;

	/** Roadie run: time to lean fully forward after the sprint starts (s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Sprint", meta = (ClampMin = "0", Units = "Seconds"))
	float SprintLeanInTime = 0.5f;

	/** Roadie run: time to straighten up after the sprint ends (s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Sprint", meta = (ClampMin = "0", Units = "Seconds"))
	float SprintLeanOutTime = 0.3f;

	/** Roadie run: forward bend per spine bone in the lean chain (deg). The anim graph applies it to spine_01..spine_03. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Sprint", meta = (Units = "Degrees"))
	float SprintLeanDegreesPerBone = 6.f;

	/** How fast the blendspace direction follows a change (deg/s). Lower = softer, slower animation changes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "1", Units = "DegreesPerSecond"))
	float DirectionBlendRate = 540.f;

	/**
	 * Leg turn limit (deg). Past this the blendspace direction is pulled along so the hips never twist further.
	 * Needs to be about 90 + CardinalHysteresis when a side cardinal is switched off.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float MaxWarpAngle = 100.f;

	/** Side steps count as this much further away (deg), so exact diagonals pick forward/back rather than a side step. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0", ClampMax = "40", Units = "Degrees"))
	float SideCardinalBias = 5.f;

	/**
	 * The backpedal counts as this much further away (deg). Keeps pure sideways movement on the forward clip when a side
	 * cardinal is off, instead of a backpedal twisted 90 deg (hips facing backwards).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion", meta = (ClampMin = "0", ClampMax = "60", Units = "Degrees"))
	float BackCardinalBias = 20.f;

	/** Use the blendspace's left sample. Off = moving left plays forward/back with the legs turned (for a bad left clip). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion")
	bool bUseLeftCardinal = true;

	/** Use the blendspace's right sample. Off = moving right plays forward/back with the legs turned. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coralis|Locomotion")
	bool bUseRightCardinal = true;

private:
	void UpdateCardinal(bool bMoving);
	void UpdateBlendDirection(bool bMoving, float DeltaSeconds);
	void UpdateSprintLean(float DeltaSeconds);
	void UpdateClipWarps(bool bMoving, float DeltaSeconds);

	/** Last travel direction relative to the body while moving (held while stopping). */
	float LastTravelDirection = 0.f;

	float SprintLeanLinear = 0.f;

	bool bHadCardinal = false;
	bool bHadBlendDirection = false;
};
