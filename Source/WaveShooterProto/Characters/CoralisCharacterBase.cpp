// Copyright Acolyte Games. All Rights Reserved.

#include "Characters/CoralisCharacterBase.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Combat/CoralisWeaponComponent.h"
#include "Core/CoralisGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Movement/CoralisMovementComponent.h"
#include "Movement/CoralisMovementSettings.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float YawToByte = 256.f / 360.f;

	TAutoConsoleVariable<bool> CVarShowMoveDebug(
		TEXT("Coralis.Move.ShowDebug"), false,
		TEXT("Shows movement state of locally controlled Coralis characters on screen."));
}

FVector FCoralisReplicatedMoveState::GetDodgeDirection() const
{
	return FRotator(0.f, DodgeYaw / YawToByte, 0.f).Vector();
}

void FCoralisReplicatedMoveState::SetDodgeDirection(const FVector& Dir)
{
	const float Yaw = FRotator::ClampAxis(Dir.Rotation().Yaw);
	DodgeYaw = static_cast<uint8>(FMath::RoundToInt(Yaw * YawToByte) & 0xFF);
}

ACoralisCharacterBase::ACoralisCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCoralisMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Facing comes from the movement component, not straight from the controller.
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	Weapon = CreateDefaultSubobject<UCoralisWeaponComponent>(TEXT("Weapon"));
}

void ACoralisCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ACoralisCharacterBase, ReplicatedMoveState, COND_SimulatedOnly);
}

UCoralisMovementComponent* ACoralisCharacterBase::GetCoralisMovement() const
{
	return Cast<UCoralisMovementComponent>(GetCharacterMovement());
}

void ACoralisCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		UpdateReplicatedMoveState();
	}

#if !UE_BUILD_SHIPPING
	if (CVarShowMoveDebug.GetValueOnGameThread() && IsLocallyControlled() && GEngine)
	{
		const UCoralisMovementComponent* Move = GetCoralisMovement();
		FGameplayTagContainer DebugTags;
		GetOwnedGameplayTags(DebugTags);
		GEngine->AddOnScreenDebugMessage(reinterpret_cast<uint64>(this), 0.f, FColor::Cyan,
			FString::Printf(TEXT("%s  speed %.0f / max %.0f  yaw %.0f  %s"), *GetName(), GetVelocity().Size2D(),
				Move ? Move->GetMaxSpeed() : 0.f, GetActorRotation().Yaw, *DebugTags.ToStringSimple()));
	}
#endif
}

void ACoralisCharacterBase::UpdateReplicatedMoveState()
{
	const UCoralisMovementComponent* Move = GetCoralisMovement();
	if (!Move)
	{
		return;
	}
	ReplicatedMoveState.bSprinting = Move->IsSprinting();
	ReplicatedMoveState.bAiming = Move->IsAiming();
	ReplicatedMoveState.bDodging = Move->IsDodging();
	ReplicatedMoveState.bRecovering = Move->IsRecovering();
	// Stored in 2-degree steps to fit a byte (+-180 deg).
	ReplicatedMoveState.AimYawOffsetHalfDeg = static_cast<int8>(FMath::Clamp(FMath::RoundToInt(GetAimYawOffset() * 0.5f), -90, 90));
}

float ACoralisCharacterBase::GetAimYawOffset() const
{
	if (const AController* C = GetController())
	{
		return FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, C->GetControlRotation().Yaw);
	}
	return ReplicatedMoveState.AimYawOffsetHalfDeg * 2.f;
}

void ACoralisCharacterBase::SetSprinting(bool bWantsToSprint)
{
	if (UCoralisMovementComponent* Move = GetCoralisMovement())
	{
		if (bWantsToSprint && bIsCrouched)
		{
			UnCrouch();
		}
		Move->SetWantsToSprint(bWantsToSprint);
	}
}

void ACoralisCharacterBase::SetAiming(bool bWantsToAim)
{
	if (UCoralisMovementComponent* Move = GetCoralisMovement())
	{
		Move->SetWantsToAim(bWantsToAim);
	}
}

void ACoralisCharacterBase::ToggleCrouch()
{
	if (bIsCrouched || (GetCharacterMovement() && GetCharacterMovement()->bWantsToCrouch))
	{
		UnCrouch();
	}
	else
	{
		if (UCoralisMovementComponent* Move = GetCoralisMovement())
		{
			Move->SetWantsToSprint(false);
		}
		Crouch();
	}
}

void ACoralisCharacterBase::Dodge(FVector DirectionOverride)
{
	if (UCoralisMovementComponent* Move = GetCoralisMovement())
	{
		Move->RequestDodge(DirectionOverride);
	}
}

void ACoralisCharacterBase::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UCoralisMovementComponent* Move = GetCoralisMovement())
	{
		Move->GetMovementStateTags(TagContainer);
	}
	if (Weapon && Weapon->Settings)
	{
		const ECoralisHeatPhase Phase = Weapon->GetHeatPhase();
		if (Phase == ECoralisHeatPhase::Venting || Phase == ECoralisHeatPhase::SweetVenting)
		{
			TagContainer.AddTag(CoralisTags::Weapon_Venting);
		}
		if (Phase == ECoralisHeatPhase::Jammed)
		{
			TagContainer.AddTag(CoralisTags::Weapon_Jammed);
		}
		if (Weapon->IsBuffActive())
		{
			TagContainer.AddTag(CoralisTags::Weapon_Buffed);
		}
	}
}

void ACoralisCharacterBase::NotifyDodgeStarted(const FVector& Direction)
{
	if (HasAuthority())
	{
		ReplicatedMoveState.SetDodgeDirection(Direction);
		++ReplicatedMoveState.DodgeCount;
		ReplicatedMoveState.bDodging = true;
	}
	PlayDodgeCosmetics(Direction);
	OnDodgeStarted.Broadcast(Direction);
}

void ACoralisCharacterBase::OnRep_ReplicatedMoveState(const FCoralisReplicatedMoveState& OldState)
{
	if (ReplicatedMoveState.DodgeCount != OldState.DodgeCount)
	{
		const FVector Dir = ReplicatedMoveState.GetDodgeDirection();
		PlayDodgeCosmetics(Dir);
		OnDodgeStarted.Broadcast(Dir);
	}
}

void ACoralisCharacterBase::PlayDodgeCosmetics_Implementation(FVector Direction)
{
	const UCoralisMovementComponent* Move = GetCoralisMovement();
	const UCoralisMovementSettings* S = Move ? Move->GetSettings() : nullptr;
	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!S || !S->DodgeAnimation || !Anim)
	{
		return;
	}
	const float Length = S->DodgeAnimation->GetPlayLength();
	const float PlayRate = (S->DodgeDuration > 0.f && Length > 0.f) ? Length / S->DodgeDuration : 1.f;
	Anim->PlaySlotAnimationAsDynamicMontage(S->DodgeAnimation, S->DodgeAnimSlot, S->DodgeAnimBlendTime, S->DodgeAnimBlendTime, PlayRate);
}
