// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "CoralisMovementSettings.generated.h"

class UAnimSequenceBase;

/** How fast a character turns toward a target yaw. The rate eases down inside EaseAngle so turns settle instead of snapping. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisTurnSettings
{
	GENERATED_BODY()

	/** Turn rate when far from the target yaw (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn", meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float MaxRate = 600.f;

	/** Turn rate floor near the target (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn", meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float MinRate = 120.f;

	/** Below this remaining angle the rate scales down from MaxRate toward MinRate. 0 = constant rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float EaseAngle = 45.f;
};

/**
 * All movement tunables for one character type (player, grunt, brute...).
 * The movement component reads only from here, so designers tune without touching code.
 */
UCLASS(BlueprintType)
class WAVESHOOTERPROTO_API UCoralisMovementSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UCoralisMovementSettings();

	// --- Ground (jog) ---

	/** Full-stick jog speed (cm/s). Keep it on a locomotion blendspace sample speed (walk anims are authored at 300) to avoid foot sliding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0", Units = "CentimetersPerSecond"))
	float JogSpeed = 300.f;

	/** Speed at minimum stick deflection (cm/s). Lets a light stick push walk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0", Units = "CentimetersPerSecond"))
	float MinAnalogSpeed = 40.f;

	/** Acceleration while jogging, at full pace (cm/s^2). With weighted movement on, StartupCurve scales it down at low speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0"))
	float Acceleration = 700.f;

	/** Deceleration when input is released (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0"))
	float BrakingDeceleration = 1100.f;

	// --- Weight (inertia) ---

	/**
	 * Weighted ground movement: slow, eased pick-up from standstill, reversals must brake through zero, and direction
	 * changes take time. Off = the engine's default snappy model (velocity swings straight to the stick direction).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight")
	bool bWeightedMovement = true;

	/**
	 * Pick-up shape. X = current speed as a fraction of the target speed (0..1), Y = multiplier on Acceleration.
	 * A low value at X = 0 gives the heavy, delayed start; the curve then eases up to full acceleration.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight", meta = (EditCondition = "bWeightedMovement"))
	FRuntimeFloatCurve StartupCurve;

	/** Braking applied to the part of the velocity that goes against the stick, e.g. switching A to D (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight", meta = (ClampMin = "0", EditCondition = "bWeightedMovement"))
	float ReverseDeceleration = 900.f;

	/** How fast velocity bends sideways toward a new direction (cm/s^2). Lower = wider, heavier turns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight", meta = (ClampMin = "0", EditCondition = "bWeightedMovement"))
	float TurnAcceleration = 1000.f;

	/** How quickly velocity bends toward a new input direction (sprint, and jogging when weighted movement is off). Higher = tighter turns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0"))
	float GroundFriction = 6.f;

	/** Friction applied while braking with no input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0"))
	float BrakingFriction = 2.f;

	/**
	 * Locked view: the character always faces where the camera looks (strafing), except while sprinting or rolling.
	 * If false, the character turns toward the move direction using JogTurn, and faces the camera only while aiming.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground")
	bool bLockFacingToCamera = true;

	/** Locked view: within this angle of the camera yaw the facing follows instantly; further away (after a sprint or roll) it turns back at LockCatchUpRate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees", EditCondition = "bLockFacingToCamera"))
	float LockCatchUpAngle = 30.f;

	/** Locked view: turn rate used to swing back to the camera yaw from beyond LockCatchUpAngle (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0", Units = "DegreesPerSecond", EditCondition = "bLockFacingToCamera"))
	float LockCatchUpRate = 1080.f;

	/**
	 * Standing still with the view locked: the feet stay planted and only the torso follows the camera, up to this angle.
	 * Past it the legs turn to catch up with the camera (0 = always face the camera, as when moving).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees", EditCondition = "bLockFacingToCamera"))
	float IdleTorsoTurnLimit = 70.f;

	/** How fast the legs turn when catching up with the camera while standing (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0", Units = "DegreesPerSecond", EditCondition = "bLockFacingToCamera"))
	float IdleCatchUpRate = 400.f;

	/** The leg catch-up stops once the body is this close to the camera direction (deg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "0", ClampMax = "90", Units = "Degrees", EditCondition = "bLockFacingToCamera"))
	float IdleCatchUpStopAngle = 5.f;

	/** Facing turn toward the move direction while jogging, when the view is not locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (EditCondition = "!bLockFacingToCamera"))
	FCoralisTurnSettings JogTurn;

	// --- Aim ---

	/** Move speed while aiming (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ClampMin = "0", Units = "CentimetersPerSecond"))
	float AimSpeed = 250.f;

	/** Facing turn toward the camera yaw while aiming, when the view is not locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (EditCondition = "!bLockFacingToCamera"))
	FCoralisTurnSettings AimTurn;

	// --- Crouch ---

	/** Move speed while crouched (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch", meta = (ClampMin = "0", Units = "CentimetersPerSecond"))
	float CrouchSpeed = 220.f;

	/** Capsule half height while crouched (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crouch", meta = (ClampMin = "10", Units = "Centimeters"))
	float CrouchedHalfHeight = 60.f;

	// --- Sprint (roadie run) ---

	/** Roadie run speed (cm/s). Keep it on a blendspace sample speed (jog anims are authored at 600) to avoid foot sliding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint", meta = (ClampMin = "0", Units = "CentimetersPerSecond"))
	float SprintSpeed = 600.f;

	/** Acceleration while sprinting (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint", meta = (ClampMin = "0"))
	float SprintAcceleration = 1000.f;

	/** Facing turn while sprinting. Low rates give wide, committed turns: velocity follows the facing, not the stick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint")
	FCoralisTurnSettings SprintTurn;

	/** No sprint while the stick points further than this from the facing: pulling back stops it, and sprint + back just walks back. 180 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float SprintCancelInputAngle = 150.f;

	/**
	 * Stick directions up to this angle from forward steer the sprint from the live camera, so a held A or D keeps
	 * turning while the camera follows. Further back, the run keeps the camera direction from when the stick was pulled
	 * back (turned only by look input). Keep below 135, or a held back diagonal circles with the following camera.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float SprintLiveSteerAngle = 100.f;

	// --- Dodge roll ---

	/** Total ground distance covered by one roll (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0", Units = "Centimeters"))
	float DodgeDistance = 600.f;

	/** Roll duration (s). The roll animation's play rate is fitted to this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0.05", Units = "Seconds"))
	float DodgeDuration = 1.f;

	/**
	 * Speed shape over the roll, X = normalized time 0..1, Y = relative speed.
	 * Only the shape matters: it is normalized so the roll always covers DodgeDistance.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	FRuntimeFloatCurve DodgeSpeedCurve;

	/** How fast the character turns to face the roll direction (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float DodgeTurnRate = 1440.f;

	/** Recovery after a roll: no sprint, no new roll, slowed movement (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0", Units = "Seconds"))
	float RecoveryDuration = 0.3f;

	/** Max speed multiplier during recovery. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0", ClampMax = "1"))
	float RecoverySpeedScale = 0.35f;

	/** Rolling from a crouch stands the character up. If false, crouched characters cannot roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	bool bCanDodgeFromCrouch = true;

	/** Optional full-body animation played for the roll. Play rate is fitted to DodgeDuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge|Animation")
	TObjectPtr<UAnimSequenceBase> DodgeAnimation;

	/** Anim Blueprint slot the roll animation plays in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge|Animation")
	FName DodgeAnimSlot = TEXT("FullBody");

	/** Blend in/out for the roll animation (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge|Animation", meta = (ClampMin = "0", Units = "Seconds"))
	float DodgeAnimBlendTime = 0.08f;

	// --- Air ---

	/** Steering while airborne, 0..1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air", meta = (ClampMin = "0", ClampMax = "1"))
	float AirControl = 0.2f;

	/** Roll speed at normalized time Alpha (cm/s). */
	UFUNCTION(BlueprintPure, Category = "Dodge")
	float GetDodgeSpeedAt(float Alpha) const;

	/** Highest speed reached during a roll (cm/s). */
	UFUNCTION(BlueprintPure, Category = "Dodge")
	float GetDodgePeakSpeed() const;

	/** StartupCurve value at a speed fraction (1 when the curve is empty). */
	UFUNCTION(BlueprintPure, Category = "Weight")
	float GetStartupScale(float SpeedFraction) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void RefreshDodgeCurveStats() const;

	mutable float CachedCurveMean = -1.f;
	mutable float CachedCurveMax = -1.f;
};
