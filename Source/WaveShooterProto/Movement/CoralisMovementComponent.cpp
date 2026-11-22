// Copyright Acolyte Games. All Rights Reserved.

#include "Movement/CoralisMovementComponent.h"

#include "Characters/CoralisCharacterBase.h"
#include "Core/CoralisGameplayTags.h"
#include "GameFramework/Controller.h"
#include "Movement/CoralisMovementSettings.h"
#include "WaveShooterProto.h"

namespace
{
	// Velocity within this of the stick target counts as settled (cm/s). Numerical tolerance for move combining, not a tunable.
	constexpr float WeightedSettleTolerance = 1.f;
}

// ---------------------------------------------------------------------------------------------------------------------
// Saved move + prediction data
// ---------------------------------------------------------------------------------------------------------------------

class FSavedMove_Coralis : public FSavedMove_Character
{
public:
	using Super = FSavedMove_Character;

	enum ECoralisFlags : uint8
	{
		FLAG_Sprint = FLAG_Custom_0,
		FLAG_Dodge = FLAG_Custom_1,
		FLAG_Aim = FLAG_Custom_2,
		// FLAG_Custom_3 is kept free for cover.
	};

	uint8 bSavedWantsToSprint : 1 = 0;
	uint8 bSavedWantsToAim : 1 = 0;
	uint8 bSavedWantsToDodge : 1 = 0;
	uint8 bSavedIsSprinting : 1 = 0;
	uint8 bSavedNoCombine : 1 = 0;
	float SavedDodgeTimeRemaining = 0.f;
	float SavedDodgeElapsed = 0.f;
	float SavedRecoveryTimeRemaining = 0.f;
	FVector SavedDodgeDirection = FVector::ForwardVector;

	virtual void Clear() override
	{
		Super::Clear();
		bSavedWantsToSprint = bSavedWantsToAim = bSavedWantsToDodge = bSavedIsSprinting = bSavedNoCombine = 0;
		SavedDodgeTimeRemaining = SavedDodgeElapsed = SavedRecoveryTimeRemaining = 0.f;
		SavedDodgeDirection = FVector::ForwardVector;
	}

	virtual uint8 GetCompressedFlags() const override
	{
		uint8 Result = Super::GetCompressedFlags();
		if (bSavedWantsToSprint) { Result |= FLAG_Sprint; }
		if (bSavedWantsToDodge) { Result |= FLAG_Dodge; }
		if (bSavedWantsToAim) { Result |= FLAG_Aim; }
		return Result;
	}

	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override
	{
		const FSavedMove_Coralis* Other = static_cast<const FSavedMove_Coralis*>(NewMove.Get());
		if (bSavedWantsToSprint != Other->bSavedWantsToSprint || bSavedWantsToAim != Other->bSavedWantsToAim ||
			bSavedWantsToDodge || Other->bSavedWantsToDodge ||
			SavedDodgeTimeRemaining > 0.f || Other->SavedDodgeTimeRemaining > 0.f ||
			(SavedRecoveryTimeRemaining > 0.f) != (Other->SavedRecoveryTimeRemaining > 0.f) ||
			// A combined move only carries the last view rotation and stick direction, so while the facing turns at a
			// limited rate (roadie run, roll, swing back to the camera) or the weighted velocity is still catching up to
			// the stick, the server would integrate a different path.
			bSavedNoCombine || Other->bSavedNoCombine)
		{
			return false;
		}
		return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
	}

	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override
	{
		Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
		if (const UCoralisMovementComponent* Move = Cast<UCoralisMovementComponent>(C->GetCharacterMovement()))
		{
			bSavedWantsToSprint = Move->bWantsToSprint;
			bSavedWantsToAim = Move->bWantsToAim;
			bSavedWantsToDodge = Move->bWantsToDodge;
			bSavedIsSprinting = Move->bIsSprinting;
			SavedDodgeTimeRemaining = Move->DodgeTimeRemaining;
			SavedDodgeElapsed = Move->DodgeElapsed;
			SavedRecoveryTimeRemaining = Move->RecoveryTimeRemaining;
			SavedDodgeDirection = Move->DodgeDirection;
			bSavedNoCombine = Move->IsFacingRateLimited() || Move->IsWeightedVelocityConverging();
		}
	}

	virtual void PrepMoveFor(ACharacter* C) override
	{
		Super::PrepMoveFor(C);
		if (UCoralisMovementComponent* Move = Cast<UCoralisMovementComponent>(C->GetCharacterMovement()))
		{
			Move->bWantsToSprint = bSavedWantsToSprint;
			Move->bWantsToAim = bSavedWantsToAim;
			Move->bWantsToDodge = bSavedWantsToDodge;
			Move->bIsSprinting = bSavedIsSprinting;
			Move->DodgeTimeRemaining = SavedDodgeTimeRemaining;
			Move->DodgeElapsed = SavedDodgeElapsed;
			Move->RecoveryTimeRemaining = SavedRecoveryTimeRemaining;
			Move->DodgeDirection = SavedDodgeDirection;
		}
	}
};

class FNetworkPredictionData_Client_Coralis : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_Coralis(const UCharacterMovementComponent& ClientMovement)
		: FNetworkPredictionData_Client_Character(ClientMovement)
	{
	}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return FSavedMovePtr(new FSavedMove_Coralis());
	}
};

// ---------------------------------------------------------------------------------------------------------------------

UCoralisMovementComponent::UCoralisMovementComponent()
{
	bWantsInitializeComponent = true;

	// Facing is handled in PhysicsRotation below.
	bOrientRotationToMovement = false;
	bUseControllerDesiredRotation = false;

	NavAgentProps.bCanCrouch = true;
	// AI path following drives acceleration, so AI characters ease in and face their path like players do.
	NavMovementProperties.bUseAccelerationForPaths = true;
}

void UCoralisMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	ApplySettings();
}

const UCoralisMovementSettings* UCoralisMovementComponent::GetSettings() const
{
	return Settings ? Settings.Get() : GetDefault<UCoralisMovementSettings>();
}

void UCoralisMovementComponent::ApplySettings()
{
	if (!Settings)
	{
		UE_LOG(LogWaveShooter, Warning, TEXT("%s has no movement settings asset, using class defaults."), *GetPathNameSafe(this));
	}
	const UCoralisMovementSettings* S = GetSettings();
	MaxWalkSpeed = S->JogSpeed;
	MinAnalogWalkSpeed = S->MinAnalogSpeed;
	MaxAcceleration = S->Acceleration;
	BrakingDecelerationWalking = S->BrakingDeceleration;
	GroundFriction = S->GroundFriction;
	bUseSeparateBrakingFriction = true;
	BrakingFriction = S->BrakingFriction;
	MaxWalkSpeedCrouched = S->CrouchSpeed;
	SetCrouchedHalfHeight(S->CrouchedHalfHeight);
	AirControl = S->AirControl;
}

// --- Intent ----------------------------------------------------------------------------------------------------------

void UCoralisMovementComponent::SetWantsToSprint(bool bWants)
{
	bWantsToSprint = bWants;
}

void UCoralisMovementComponent::SetWantsToAim(bool bWants)
{
	bWantsToAim = bWants;
}

void UCoralisMovementComponent::RequestDodge(FVector DirectionOverride)
{
	bWantsToDodge = true;
	if (CharacterOwner && CharacterOwner->HasAuthority())
	{
		PendingDodgeDirectionOverride = DirectionOverride.GetSafeNormal2D();
	}
}

// --- State -----------------------------------------------------------------------------------------------------------

bool UCoralisMovementComponent::IsSimulatedProxyOwner() const
{
	return CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy;
}

bool UCoralisMovementComponent::IsSprinting() const
{
	if (IsSimulatedProxyOwner())
	{
		const ACoralisCharacterBase* C = Cast<ACoralisCharacterBase>(CharacterOwner);
		return C && C->GetReplicatedMoveState().bSprinting;
	}
	return bIsSprinting;
}

bool UCoralisMovementComponent::IsAiming() const
{
	if (IsSimulatedProxyOwner())
	{
		const ACoralisCharacterBase* C = Cast<ACoralisCharacterBase>(CharacterOwner);
		return C && C->GetReplicatedMoveState().bAiming;
	}
	return bWantsToAim && !IsDodging() && !bIsSprinting;
}

bool UCoralisMovementComponent::IsDodging() const
{
	if (IsSimulatedProxyOwner())
	{
		const ACoralisCharacterBase* C = Cast<ACoralisCharacterBase>(CharacterOwner);
		return C && C->GetReplicatedMoveState().bDodging;
	}
	return DodgeTimeRemaining > 0.f;
}

bool UCoralisMovementComponent::IsRecovering() const
{
	if (IsSimulatedProxyOwner())
	{
		const ACoralisCharacterBase* C = Cast<ACoralisCharacterBase>(CharacterOwner);
		return C && C->GetReplicatedMoveState().bRecovering;
	}
	return RecoveryTimeRemaining > 0.f && DodgeTimeRemaining <= 0.f;
}

float UCoralisMovementComponent::GetDodgeAlpha() const
{
	const float Duration = GetSettings()->DodgeDuration;
	return (DodgeTimeRemaining > 0.f && Duration > 0.f) ? FMath::Clamp(DodgeElapsed / Duration, 0.f, 1.f) : 0.f;
}

FVector UCoralisMovementComponent::GetDodgeDirection() const
{
	if (IsSimulatedProxyOwner())
	{
		const ACoralisCharacterBase* C = Cast<ACoralisCharacterBase>(CharacterOwner);
		return C ? C->GetReplicatedMoveState().GetDodgeDirection() : FVector::ForwardVector;
	}
	return DodgeDirection;
}

bool UCoralisMovementComponent::CanUseWeapon() const
{
	return !IsSprinting() && !IsDodging();
}

void UCoralisMovementComponent::GetMovementStateTags(FGameplayTagContainer& OutTags) const
{
	if (IsSprinting()) { OutTags.AddTag(CoralisTags::Movement_Sprinting); }
	if (IsAiming()) { OutTags.AddTag(CoralisTags::Movement_Aiming); }
	if (IsCrouching()) { OutTags.AddTag(CoralisTags::Movement_Crouching); }
	if (IsDodging()) { OutTags.AddTag(CoralisTags::Movement_Dodging); }
	if (IsRecovering()) { OutTags.AddTag(CoralisTags::Movement_Recovering); }
	if (IsFalling()) { OutTags.AddTag(CoralisTags::Movement_Falling); }
	if (MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(ECoralisCustomMovementMode::Cover))
	{
		OutTags.AddTag(CoralisTags::Movement_InCover);
	}
}

bool UCoralisMovementComponent::HasMoveInput() const
{
	return !Acceleration.IsNearlyZero();
}

// --- Speeds ----------------------------------------------------------------------------------------------------------

float UCoralisMovementComponent::GetMaxSpeed() const
{
	const UCoralisMovementSettings* S = GetSettings();
	if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking)
	{
		if (DodgeTimeRemaining > 0.f)
		{
			return S->GetDodgePeakSpeed();
		}
		float Speed = Super::GetMaxSpeed(); // Jog, or crouch speed while crouched.
		if (bIsSprinting)
		{
			Speed = S->SprintSpeed;
		}
		else if (!IsCrouching() && bWantsToAim)
		{
			Speed = S->AimSpeed;
		}
		if (RecoveryTimeRemaining > 0.f)
		{
			Speed *= S->RecoverySpeedScale;
		}
		return Speed;
	}
	return Super::GetMaxSpeed();
}

float UCoralisMovementComponent::GetMaxAcceleration() const
{
	if (bIsSprinting && IsMovingOnGround())
	{
		return GetSettings()->SprintAcceleration;
	}
	return Super::GetMaxAcceleration();
}

// --- Simulation ------------------------------------------------------------------------------------------------------

bool UCoralisMovementComponent::CanStartSprint() const
{
	// Sprint wins over aim: holding aim doesn't block the roadie run; aiming is suppressed while sprinting.
	if (!bWantsToSprint || !IsMovingOnGround() || IsCrouching() || DodgeTimeRemaining > 0.f || RecoveryTimeRemaining > 0.f || !HasMoveInput())
	{
		return false;
	}
	const UCoralisMovementSettings* S = GetSettings();
	if (S->SprintCancelInputAngle < 180.f && UpdatedComponent)
	{
		const FVector Facing = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
		const FVector Input = Acceleration.GetSafeNormal2D();
		const float CosLimit = FMath::Cos(FMath::DegreesToRadians(S->SprintCancelInputAngle));
		// Pulling back stops the sprint, and a sprint can't start against the facing (sprint + back just walks back).
		if (FVector::DotProduct(Facing, Input) < CosLimit)
		{
			return false;
		}
	}
	return true;
}

bool UCoralisMovementComponent::CanStartDodge() const
{
	if (!IsMovingOnGround() || DodgeTimeRemaining > 0.f || RecoveryTimeRemaining > 0.f)
	{
		return false;
	}
	return !IsCrouching() || GetSettings()->bCanDodgeFromCrouch;
}

void UCoralisMovementComponent::StartDodge()
{
	const UCoralisMovementSettings* S = GetSettings();

	FVector Dir = PendingDodgeDirectionOverride;
	PendingDodgeDirectionOverride = FVector::ZeroVector;
	if (Dir.IsNearlyZero())
	{
		Dir = HasMoveInput() ? Acceleration.GetSafeNormal2D() : UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	}
	DodgeDirection = Dir.IsNearlyZero() ? FVector::ForwardVector : Dir;
	DodgeTimeRemaining = S->DodgeDuration;
	DodgeElapsed = 0.f;
	RecoveryTimeRemaining = 0.f;
	bIsSprinting = false;
	if (IsCrouching())
	{
		bWantsToCrouch = false;
		UnCrouch(false);
	}

	// Cosmetics only once per real dodge, not again when the client replays moves after a correction.
	if (ACoralisCharacterBase* C = Cast<ACoralisCharacterBase>(CharacterOwner))
	{
		if (!C->bClientUpdating)
		{
			C->NotifyDodgeStarted(DodgeDirection);
		}
	}
}

void UCoralisMovementComponent::EndDodge()
{
	DodgeTimeRemaining = 0.f;
	DodgeElapsed = 0.f;
	RecoveryTimeRemaining = GetSettings()->RecoveryDuration;
}

void UCoralisMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (!CharacterOwner || IsSimulatedProxyOwner())
	{
		return;
	}

	// Advance timers with the move's own delta so server and client replays stay in step.
	if (DodgeTimeRemaining > 0.f)
	{
		DodgeElapsed += DeltaSeconds;
		DodgeTimeRemaining -= DeltaSeconds;
		if (DodgeTimeRemaining <= 0.f || !IsMovingOnGround())
		{
			EndDodge();
		}
	}
	else if (RecoveryTimeRemaining > 0.f)
	{
		RecoveryTimeRemaining = FMath::Max(0.f, RecoveryTimeRemaining - DeltaSeconds);
	}

	if (bWantsToDodge)
	{
		bWantsToDodge = false;
		if (CanStartDodge())
		{
			StartDodge();
		}
	}

	bIsSprinting = CanStartSprint();
}

void UCoralisMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	if (DodgeTimeRemaining > 0.f && IsMovingOnGround() && !HasAnimRootMotion())
	{
		const UCoralisMovementSettings* S = GetSettings();
		const float Alpha = S->DodgeDuration > 0.f ? FMath::Clamp(DodgeElapsed / S->DodgeDuration, 0.f, 1.f) : 1.f;
		Velocity = DodgeDirection * S->GetDodgeSpeedAt(Alpha);
		return;
	}

	if (bIsSprinting && IsMovingOnGround() && UpdatedComponent)
	{
		// Roadie run: push along the facing, and let the slow facing turn bend the path.
		const FVector InputAccel = Acceleration;
		Acceleration = UpdatedComponent->GetForwardVector().GetSafeNormal2D() * InputAccel.Size();
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		Acceleration = InputAccel;
		return;
	}

	if (CalcWeightedVelocity(DeltaTime, bFluid))
	{
		return;
	}

	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
}

bool UCoralisMovementComponent::CalcWeightedVelocity(float DeltaTime, bool bFluid)
{
	const UCoralisMovementSettings* S = GetSettings();
	// Only ground movement with stick input; no input = the engine's braking, path requests = engine path.
	if (!S->bWeightedMovement || bFluid || !IsMovingOnGround() || Acceleration.IsNearlyZero() || bHasRequestedVelocity ||
		HasAnimRootMotion() || DeltaTime < UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float TargetSpeed = FMath::Max(GetMaxSpeed() * AnalogInputModifier, GetMinAnalogSpeed());
	const FVector Desired = Acceleration.GetSafeNormal2D() * TargetSpeed;
	const FVector Planar(Velocity.X, Velocity.Y, 0.f);
	const float Speed = Planar.Size();
	const FVector Along = Speed > UE_KINDA_SMALL_NUMBER ? Planar / Speed : Desired.GetSafeNormal();

	// Split the change into "faster/slower along the current motion" and "bend sideways", each with its own limit.
	const FVector Change = Desired - Planar;
	float AlongChange = FVector::DotProduct(Change, Along);
	FVector SideChange = Change - Along * AlongChange;

	const float SpeedFraction = TargetSpeed > UE_KINDA_SMALL_NUMBER ? Speed / TargetSpeed : 1.f;
	const float AlongRate = AlongChange >= 0.f ? S->Acceleration * S->GetStartupScale(SpeedFraction) : S->ReverseDeceleration;
	AlongChange = FMath::Clamp(AlongChange, -AlongRate * DeltaTime, AlongRate * DeltaTime);
	SideChange = SideChange.GetClampedToMaxSize(S->TurnAcceleration * DeltaTime);

	const FVector NewPlanar = Planar + Along * AlongChange + SideChange;
	Velocity.X = NewPlanar.X;
	Velocity.Y = NewPlanar.Y;
	return true;
}

bool UCoralisMovementComponent::ComputeDesiredYaw(float& OutYaw, const FCoralisTurnSettings*& OutTurn, float& OutFixedRate, bool& bOutLocked) const
{
	const UCoralisMovementSettings* S = GetSettings();
	OutTurn = nullptr;
	OutFixedRate = 0.f;
	bOutLocked = false;

	if (DodgeTimeRemaining > 0.f)
	{
		OutYaw = DodgeDirection.Rotation().Yaw;
		OutFixedRate = S->DodgeTurnRate;
		return true;
	}
	if (bIsSprinting)
	{
		OutYaw = Acceleration.Rotation().Yaw;
		OutTurn = &S->SprintTurn;
		return HasMoveInput();
	}
	const AController* Controller = CharacterOwner ? CharacterOwner->GetController() : nullptr;
	if (Controller && S->bLockFacingToCamera)
	{
		OutYaw = Controller->GetControlRotation().Yaw;
		// Standing: feet stay planted while the torso follows the camera; past the limit the legs catch up.
		if (!HasMoveInput() && S->IdleTorsoTurnLimit > 0.f && UpdatedComponent)
		{
			const float Gap = FMath::Abs(FMath::FindDeltaAngleDegrees(UpdatedComponent->GetComponentRotation().Yaw, OutYaw));
			if (Gap > S->IdleTorsoTurnLimit)
			{
				bIdleCatchingUp = true;
			}
			else if (Gap <= S->IdleCatchUpStopAngle)
			{
				bIdleCatchingUp = false;
			}
			if (!bIdleCatchingUp)
			{
				return false;
			}
			OutFixedRate = S->IdleCatchUpRate;
			return true;
		}
		bIdleCatchingUp = false;
		bOutLocked = true;
		return true;
	}
	if (Controller && bWantsToAim)
	{
		OutYaw = Controller->GetControlRotation().Yaw;
		OutTurn = &S->AimTurn;
		return true;
	}
	if (HasMoveInput())
	{
		OutYaw = Acceleration.Rotation().Yaw;
		OutTurn = &S->JogTurn;
		return true;
	}
	return false;
}

bool UCoralisMovementComponent::IsWeightedVelocityConverging() const
{
	const UCoralisMovementSettings* S = GetSettings();
	if (!S->bWeightedMovement || Acceleration.IsNearlyZero() || !IsMovingOnGround())
	{
		return false;
	}
	const float TargetSpeed = FMath::Max(GetMaxSpeed() * AnalogInputModifier, GetMinAnalogSpeed());
	const FVector Desired = Acceleration.GetSafeNormal2D() * TargetSpeed;
	return !FVector(Velocity.X - Desired.X, Velocity.Y - Desired.Y, 0.f).IsNearlyZero(WeightedSettleTolerance);
}

bool UCoralisMovementComponent::IsFacingRateLimited() const
{
	if (bWantsToSprint || bIsSprinting || bWantsToDodge || DodgeTimeRemaining > 0.f || !UpdatedComponent)
	{
		return true;
	}
	float TargetYaw = 0.f;
	const FCoralisTurnSettings* Turn = nullptr;
	float FixedRate = 0.f;
	bool bLocked = false;
	if (!ComputeDesiredYaw(TargetYaw, Turn, FixedRate, bLocked))
	{
		return false;
	}
	const float Remaining = FMath::Abs(FMath::FindDeltaAngleDegrees(UpdatedComponent->GetComponentRotation().Yaw, TargetYaw));
	// Locked view snaps inside the catch-up angle, so the result only depends on the final view rotation.
	return bLocked ? Remaining > GetSettings()->LockCatchUpAngle : Remaining > UE_KINDA_SMALL_NUMBER;
}

void UCoralisMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (!HasValidData() || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() ||
		(!CharacterOwner->GetController() && !bRunPhysicsWithNoController) ||
		!(IsMovingOnGround() || IsFalling()))
	{
		Super::PhysicsRotation(DeltaTime);
		return;
	}

	float TargetYaw = 0.f;
	const FCoralisTurnSettings* Turn = nullptr;
	float FixedRate = 0.f;
	bool bLocked = false;
	if (!ComputeDesiredYaw(TargetYaw, Turn, FixedRate, bLocked))
	{
		return;
	}

	const FRotator Current = UpdatedComponent->GetComponentRotation();
	const float Remaining = FMath::FindDeltaAngleDegrees(Current.Yaw, TargetYaw);
	if (FMath::IsNearlyZero(Remaining, UE_KINDA_SMALL_NUMBER))
	{
		return;
	}

	float Rate = FixedRate;
	if (bLocked)
	{
		const UCoralisMovementSettings* S = GetSettings();
		Rate = FMath::Abs(Remaining) <= S->LockCatchUpAngle ? UE_BIG_NUMBER : S->LockCatchUpRate;
	}
	else if (Turn)
	{
		Rate = Turn->MaxRate;
		if (Turn->EaseAngle > 0.f)
		{
			const float Ease = FMath::Clamp(FMath::Abs(Remaining) / Turn->EaseAngle, 0.f, 1.f);
			Rate = FMath::Lerp(Turn->MinRate, Turn->MaxRate, Ease);
		}
	}
	const float MaxStep = Rate * DeltaTime;
	const float Step = FMath::Clamp(Remaining, -MaxStep, MaxStep);
	const FRotator NewRotation(0.f, FRotator::NormalizeAxis(Current.Yaw + Step), 0.f);
	MoveUpdatedComponent(FVector::ZeroVector, NewRotation, /*bSweep*/ false);
}

bool UCoralisMovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() && DodgeTimeRemaining <= 0.f;
}

bool UCoralisMovementComponent::CanAttemptJump() const
{
	return Super::CanAttemptJump() && DodgeTimeRemaining <= 0.f && RecoveryTimeRemaining <= 0.f;
}

void UCoralisMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Coralis::FLAG_Sprint) != 0;
	bWantsToAim = (Flags & FSavedMove_Coralis::FLAG_Aim) != 0;
	// Only ever set here; cleared by the simulation once consumed.
	if ((Flags & FSavedMove_Coralis::FLAG_Dodge) != 0)
	{
		bWantsToDodge = true;
	}
}

FNetworkPredictionData_Client* UCoralisMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UCoralisMovementComponent* MutableThis = const_cast<UCoralisMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Coralis(*this);
	}
	return ClientPredictionData;
}
