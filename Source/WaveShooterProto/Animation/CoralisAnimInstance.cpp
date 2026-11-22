// Copyright Acolyte Games. All Rights Reserved.

#include "Animation/CoralisAnimInstance.h"

#include "Characters/CoralisCharacterBase.h"
#include "KismetAnimationLibrary.h"
#include "Movement/CoralisMovementComponent.h"

void UCoralisAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(TryGetPawnOwner());
	const UCoralisMovementComponent* Move = Character ? Character->GetCoralisMovement() : nullptr;
	if (!Move)
	{
		return;
	}

	const FVector Velocity = Character->GetVelocity();
	LocoSpeed = Velocity.Size2D();
	LocoDirection = LocoSpeed > MoveSpeedThreshold ? UKismetAnimationLibrary::CalculateDirection(Velocity, Character->GetActorRotation()) : 0.f;
	bLocoFalling = Move->IsFalling();

	// Proxies have no acceleration; treat speed alone as intent there.
	const bool bHasAccel = Character->GetLocalRole() == ROLE_SimulatedProxy || !Move->GetCurrentAcceleration().IsNearlyZero();
	bLocoShouldMove = LocoSpeed > MoveSpeedThreshold && bHasAccel;
	UpdateCardinal(LocoSpeed > MoveSpeedThreshold);

	AimPitch = FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch);

	bAiming = Move->IsAiming();
	UpdateBlendDirection(LocoSpeed > MoveSpeedThreshold, DeltaSeconds);
	bSprinting = Move->IsSprinting();
	bCrouching = Character->bIsCrouched;
	bDodging = Move->IsDodging();
	bRecovering = Move->IsRecovering();
	UpdateSprintLean(DeltaSeconds);
	UpdateClipWarps(LocoSpeed > MoveSpeedThreshold, DeltaSeconds);
	bLocoUseClips = LocoSpeed > MoveSpeedThreshold && !bAiming && !bSprinting && !bDodging && !bLocoFalling;

	const float TwistTarget = (bSprinting || bDodging) ? 0.f : FMath::Clamp(Character->GetAimYawOffset(), -TorsoTwistLimit, TorsoTwistLimit);
	TorsoTwistYaw = FMath::FInterpConstantTo(TorsoTwistYaw, TwistTarget, DeltaSeconds, TorsoTwistRate);
	LocoPlayRate = CardinalClipSpeed > 0.f ? FMath::Clamp(LocoSpeed / CardinalClipSpeed, MinLocoPlayRate, MaxLocoPlayRate) : 1.f;
	bCanUseWeapon = Move->CanUseWeapon();

	StateTags.Reset();
	Character->GetOwnedGameplayTags(StateTags);
}

void UCoralisAnimInstance::UpdateCardinal(bool bMoving)
{
	if (!bMoving)
	{
		// Keep the last clip while slowing to a stop; the next start picks fresh.
		bHadCardinal = false;
		return;
	}

	// Forward, right, back, left; the side ones can be switched off when a clip is unusable.
	const float Cardinals[] = { 0.f, 90.f, 180.f, -90.f };
	const bool bAllowed[] = { true, bUseRightCardinal, true, bUseLeftCardinal };
	auto Score = [this](float Cardinal, float Dir)
	{
		const bool bSide = FMath::IsNearlyEqual(FMath::Abs(Cardinal), 90.f);
		const bool bBack = FMath::IsNearlyEqual(FMath::Abs(Cardinal), 180.f);
		return FMath::Abs(FMath::FindDeltaAngleDegrees(Cardinal, Dir)) + (bSide ? SideCardinalBias : 0.f) + (bBack ? BackCardinalBias : 0.f);
	};

	float Best = LocoCardinalAngle;
	float BestScore = TNumericLimits<float>::Max();
	for (int32 i = 0; i < UE_ARRAY_COUNT(Cardinals); ++i)
	{
		const float S = bAllowed[i] ? Score(Cardinals[i], LocoDirection) : TNumericLimits<float>::Max();
		if (S < BestScore)
		{
			BestScore = S;
			Best = Cardinals[i];
		}
	}
	// Keep the current cardinal until another one is clearly better.
	if (!bHadCardinal || BestScore + CardinalHysteresis < Score(LocoCardinalAngle, LocoDirection))
	{
		LocoCardinalAngle = Best;
	}
	bHadCardinal = true;
	LocoCardinalIndex = FMath::IsNearlyZero(LocoCardinalAngle) ? 0 : (FMath::IsNearlyEqual(LocoCardinalAngle, 90.f) ? 1 : (LocoCardinalAngle > 0.f ? 2 : 3));
}

void UCoralisAnimInstance::UpdateBlendDirection(bool bMoving, float DeltaSeconds)
{
	if (!bMoving)
	{
		// Hold the last direction while slowing down so the stop doesn't jump to the forward pose.
		// Ease the legs back to straight instead of snapping when stopping.
		LocoWarpAngle = FMath::FInterpConstantTo(LocoWarpAngle, 0.f, DeltaSeconds, WarpTurnRate);
		return;
	}
	// Aiming uses the 8-way blendspace directly; otherwise the cardinal set plus leg warping.
	const float Target = bAiming ? LocoDirection : LocoCardinalAngle;
	if (!bHadBlendDirection)
	{
		LocoBlendDirection = Target;
		bHadBlendDirection = true;
	}
	else
	{
		const float Step = DirectionBlendRate * DeltaSeconds;
		float Delta = FMath::FindDeltaAngleDegrees(LocoBlendDirection, Target);
		// If the short way round would swing through the direction opposite to travel (e.g. back -> forward while
		// moving left would pass through right), go the other way so the legs never point away from where they go.
		const float ToOpposite = FMath::FindDeltaAngleDegrees(LocoBlendDirection, LocoDirection + 180.f);
		if (FMath::Sign(ToOpposite) == FMath::Sign(Delta) && FMath::Abs(ToOpposite) < FMath::Abs(Delta))
		{
			Delta -= FMath::Sign(Delta) * 360.f;
		}
		LocoBlendDirection = FRotator::NormalizeAxis(LocoBlendDirection + FMath::Clamp(Delta, -Step, Step));
	}
	// Not aiming, the legs play one discrete clip (forward / right / back / left), so turn them by what is left
	// between that clip and the travel direction. Aiming uses the 8-way blendspace (the warp is zeroed in the graph).
	const float WarpBase = bAiming ? LocoBlendDirection : LocoCardinalAngle;
	const float Warp = FMath::Clamp(FMath::FindDeltaAngleDegrees(WarpBase, LocoDirection), -MaxWarpAngle, MaxWarpAngle);
	// The legs turn in gradually and only fully once moving at pace, so starts and stops don't snap the hips.
	const float SpeedWeight = WarpFullSpeed > 0.f ? FMath::Clamp(LocoSpeed / WarpFullSpeed, 0.f, 1.f) : 1.f;
	LocoWarpAngle = FMath::FInterpConstantTo(LocoWarpAngle, Warp * SpeedWeight, DeltaSeconds, WarpTurnRate);
}

void UCoralisAnimInstance::UpdateSprintLean(float DeltaSeconds)
{
	const float BlendTime = bSprinting ? SprintLeanInTime : SprintLeanOutTime;
	const float Target = bSprinting ? 1.f : 0.f;
	SprintLeanLinear = BlendTime > 0.f ? FMath::FInterpConstantTo(SprintLeanLinear, Target, DeltaSeconds, 1.f / BlendTime) : Target;
	// Ease in and out (smoothstep) so the lean starts and settles softly.
	SprintLeanAlpha = FMath::SmoothStep(0.f, 1.f, SprintLeanLinear);
	SprintLeanBoneRoll = SprintLeanAlpha * SprintLeanDegreesPerBone;
}

void UCoralisAnimInstance::UpdateClipWarps(bool bMoving, float DeltaSeconds)
{
	if (bMoving)
	{
		LastTravelDirection = LocoDirection;
	}
	auto Step = [this, DeltaSeconds](float& Warp, float ClipAngle, float Limit)
	{
		const float Target = FMath::Clamp(FMath::FindDeltaAngleDegrees(ClipAngle, LastTravelDirection), -Limit, Limit);
		Warp = FMath::FInterpConstantTo(Warp, Target, DeltaSeconds, WarpTurnRate);
	};
	// Clip travel directions: forward 0, right 90, back 180, left -90 (left is the mirrored right clip).
	Step(LocoWarpForward, 0.f, MaxClipWarpAngle);
	// Side clips turn their legs toward the back only a little (like the backpedal), so the hips don't swing further
	// back just before the backpedal takes over; toward the front they turn freely.
	const float SideLimit = FMath::Abs(LastTravelDirection) > 90.f ? MaxBackClipWarpAngle : MaxClipWarpAngle;
	Step(LocoWarpRight, 90.f, SideLimit);
	Step(LocoWarpBack, 180.f, MaxBackClipWarpAngle);
	Step(LocoWarpLeft, LeftClipTravelAngle, FMath::Abs(LastTravelDirection) > 90.f ? MaxBackClipWarpAngle + 90.f : MaxLeftClipWarpAngle);
}
