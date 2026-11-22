// Copyright Acolyte Games. All Rights Reserved.

#include "Camera/CoralisCameraRigComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayTagAssetInterface.h"

UCoralisCameraRigComponent::UCoralisCameraRigComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Run after movement so the camera reads this frame's state.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	SetIsReplicatedByDefault(false);
}

const UCoralisCameraSettings* UCoralisCameraRigComponent::GetSettings() const
{
	return Settings ? Settings.Get() : GetDefault<UCoralisCameraSettings>();
}

void UCoralisCameraRigComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		Boom = Owner->FindComponentByClass<USpringArmComponent>();
		Camera = Owner->FindComponentByClass<UCameraComponent>();
	}
	if (Camera)
	{
		CameraBaseLocation = Camera->GetRelativeLocation();
	}
	bRightShoulder = GetSettings()->bStartOnRightShoulder;
	ShoulderBlend = bRightShoulder ? 1.f : -1.f;
	SnapToTarget();
}

void UCoralisCameraRigComponent::ApplyViewLimits()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PC && PC->IsLocalController() && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->ViewPitchMin = GetSettings()->PitchMin;
		PC->PlayerCameraManager->ViewPitchMax = GetSettings()->PitchMax;
	}
}

void UCoralisCameraRigComponent::ToggleShoulder()
{
	bRightShoulder = !bRightShoulder;
}

void UCoralisCameraRigComponent::SetRightShoulder(bool bRight)
{
	bRightShoulder = bRight;
}

namespace
{
	// How close the blended lag speed must get to the stand-in before lag is switched off. Numerical tolerance, not a tunable.
	constexpr float NoLagReachedTolerance = 1.f;
}

float UCoralisCameraRigComponent::GetBlendableLagSpeed(const FCoralisCameraProfile& Profile) const
{
	return Profile.PositionLagSpeed > 0.f ? Profile.PositionLagSpeed : GetSettings()->NoLagBlendSpeed;
}

void UCoralisCameraRigComponent::SnapToTarget()
{
	FGameplayTagContainer Tags;
	if (const IGameplayTagAssetInterface* TagOwner = Cast<IGameplayTagAssetInterface>(GetOwner()))
	{
		TagOwner->GetOwnedGameplayTags(Tags);
	}
	Current = GetSettings()->FindProfile(Tags);
	bTargetHasNoLag = Current.PositionLagSpeed <= 0.f;
	Current.PositionLagSpeed = GetBlendableLagSpeed(Current);
	bInitialized = true;
	ApplyToComponents(0.f);
}

void UCoralisCameraRigComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled() || !Boom || !Camera)
	{
		return;
	}
	if (!bInitialized)
	{
		SnapToTarget();
	}

	FGameplayTagContainer Tags;
	if (const IGameplayTagAssetInterface* TagOwner = Cast<IGameplayTagAssetInterface>(Pawn))
	{
		TagOwner->GetOwnedGameplayTags(Tags);
	}
	const UCoralisCameraSettings* S = GetSettings();
	const FCoralisCameraProfile& Target = S->FindProfile(Tags);

	const float Speed = Target.BlendSpeed;
	Current.ArmLength = FMath::FInterpTo(Current.ArmLength, Target.ArmLength, DeltaTime, Speed);
	Current.SocketOffset = FMath::VInterpTo(Current.SocketOffset, Target.SocketOffset, DeltaTime, Speed);
	Current.TargetOffset = FMath::VInterpTo(Current.TargetOffset, Target.TargetOffset, DeltaTime, Speed);
	Current.FieldOfView = FMath::FInterpTo(Current.FieldOfView, Target.FieldOfView, DeltaTime, Speed);
	bTargetHasNoLag = Target.PositionLagSpeed <= 0.f;
	Current.PositionLagSpeed = FMath::FInterpTo(Current.PositionLagSpeed, GetBlendableLagSpeed(Target), DeltaTime, Speed);
	Current.BobAmplitude = FMath::Vector2DInterpTo(Current.BobAmplitude, Target.BobAmplitude, DeltaTime, Speed);
	Current.BobFrequency = FMath::FInterpTo(Current.BobFrequency, Target.BobFrequency, DeltaTime, Speed);
	Current.BobReferenceSpeed = Target.BobReferenceSpeed;
	Current.LookSensitivityScale = FMath::FInterpTo(Current.LookSensitivityScale, Target.LookSensitivityScale, DeltaTime, Speed);
	Current.HeadingFollowSpeed = FMath::FInterpTo(Current.HeadingFollowSpeed, Target.HeadingFollowSpeed, DeltaTime, Speed);
	Current.BlendSpeed = Target.BlendSpeed;

	ShoulderBlend = FMath::FInterpTo(ShoulderBlend, bRightShoulder ? 1.f : -1.f, DeltaTime, S->ShoulderSwapSpeed);

	const float GroundSpeed = Pawn->GetVelocity().Size2D();
	const float SpeedRatio = FMath::Clamp(GroundSpeed / FMath::Max(Current.BobReferenceSpeed, 1.f), 0.f, 1.f);
	BobPhase = FMath::Fmod(BobPhase + DeltaTime * Current.BobFrequency * SpeedRatio * UE_TWO_PI, UE_TWO_PI * 2.f);

	ApplyToComponents(SpeedRatio);
}

void UCoralisCameraRigComponent::NotifyLookInput()
{
	if (const UWorld* World = GetWorld())
	{
		LastLookInputTime = World->GetTimeSeconds();
	}
}

void UCoralisCameraRigComponent::ApplyHeadingFollow(float DeltaTime)
{
	if (Current.HeadingFollowSpeed <= 0.f)
	{
		return;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	// Back off while the player is aiming the camera themselves, then fade back in.
	const UCoralisCameraSettings* S = GetSettings();
	const double SinceLook = GetWorld() ? GetWorld()->GetTimeSeconds() - LastLookInputTime : 0.0;
	const float Resume = S->HeadingFollowResumeTime > 0.f
		? FMath::Clamp(static_cast<float>(SinceLook - S->HeadingFollowLookPause) / S->HeadingFollowResumeTime, 0.f, 1.f)
		: (SinceLook >= S->HeadingFollowLookPause ? 1.f : 0.f);
	if (Resume <= 0.f)
	{
		return;
	}
	FRotator View = PC->GetControlRotation();
	const float Gap = FMath::FindDeltaAngleDegrees(View.Yaw, Pawn->GetActorRotation().Yaw);
	View.Yaw += Gap * FMath::Clamp(DeltaTime * Current.HeadingFollowSpeed * Resume, 0.f, 1.f);
	PC->SetControlRotation(View);
}

void UCoralisCameraRigComponent::ApplyToComponents(float SpeedRatio)
{
	if (!Boom || !Camera)
	{
		return;
	}
	Boom->TargetArmLength = Current.ArmLength;
	Boom->SocketOffset = FVector(Current.SocketOffset.X, Current.SocketOffset.Y * ShoulderBlend, Current.SocketOffset.Z);
	Boom->TargetOffset = Current.TargetOffset;
	// Stand-in reached: the lag is already as tight as "no lag", so switching it off now doesn't pop.
	Boom->bEnableCameraLag = !(bTargetHasNoLag && Current.PositionLagSpeed >= GetSettings()->NoLagBlendSpeed - NoLagReachedTolerance);
	Boom->CameraLagSpeed = Current.PositionLagSpeed;
	Boom->ProbeSize = GetSettings()->ProbeSize;
	Camera->SetFieldOfView(Current.FieldOfView);

	// Bob: vertical at BobFrequency, side sway at half of it, both fading with speed.
	const FVector Bob(0.f,
		Current.BobAmplitude.X * SpeedRatio * FMath::Sin(BobPhase * 0.5f),
		Current.BobAmplitude.Y * SpeedRatio * FMath::Sin(BobPhase));
	Camera->SetRelativeLocation(CameraBaseLocation + Bob);
}
