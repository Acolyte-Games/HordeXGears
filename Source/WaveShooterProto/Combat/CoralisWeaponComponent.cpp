// Copyright Acolyte Games. All Rights Reserved.

#include "Combat/CoralisWeaponComponent.h"

#include "Characters/CoralisCharacterBase.h"
#include "Combat/CoralisHealthComponent.h"
#include "Combat/CoralisWeaponSettings.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Movement/CoralisMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

namespace
{
	TAutoConsoleVariable<bool> CVarShowWeaponDebug(
		TEXT("Coralis.Weapon.ShowDebug"), false,
		TEXT("Shows heat, phase and buff of every Coralis weapon in this world on screen, and logs shot results."));

	DEFINE_LOG_CATEGORY_STATIC(LogCoralisWeapon, Log, All);

#if !UE_BUILD_SHIPPING
	UCoralisWeaponComponent* FindLocalWeapon(const UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		return Pawn ? Pawn->FindComponentByClass<UCoralisWeaponComponent>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdSetHeat(
		TEXT("Coralis.Weapon.SetHeat"), TEXT("Sets your weapon's heat as a fraction of max (0..1). 1 jams it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCoralisWeaponComponent* Weapon = FindLocalWeapon(World); Weapon && Args.Num() > 0)
			{
				Weapon->DebugSetHeat(FCString::Atof(*Args[0]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdFire(
		TEXT("Coralis.Weapon.Fire"), TEXT("1 pulls your weapon's trigger, 0 releases it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCoralisWeaponComponent* Weapon = FindLocalWeapon(World); Weapon && Args.Num() > 0)
			{
				if (FCString::Atoi(*Args[0]) != 0) { Weapon->StartFire(); }
				else { Weapon->StopFire(); }
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdVent(
		TEXT("Coralis.Weapon.Vent"), TEXT("Presses vent on your weapon."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCoralisWeaponComponent* Weapon = FindLocalWeapon(World))
			{
				Weapon->Vent();
			}
		}));
#endif
}

FCoralisServerShotSignature UCoralisWeaponComponent::OnServerShot;

UCoralisWeaponComponent::UCoralisWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UCoralisWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UCoralisWeaponComponent, ProxyHeat, COND_SkipOwner);
	DOREPLIFETIME(UCoralisWeaponComponent, StartWindowSeed);
}

void UCoralisWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		StartWindowSeed = static_cast<int32>(FPlatformTime::Cycles() ^ GetUniqueID());
		Heat.WindowSeed = StartWindowSeed;
		ProxyHeat.WindowSeed = StartWindowSeed;
	}
}

void UCoralisWeaponComponent::OnRep_StartWindowSeed()
{
	Heat.WindowSeed = StartWindowSeed;
}

// --- Owner input -----------------------------------------------------------------------------------------------------

void UCoralisWeaponComponent::StartFire()
{
	if (!Settings || !IsLocallyControlledOwner())
	{
		return;
	}
	bTriggerHeld = true;
	const double Now = GetLocalTime();
	NextShotTime = FMath::Max(Now, Heat.LastShotTime + Settings->GetFireInterval());
	if (NextShotTime <= Now)
	{
		TryFireShot(Now);
	}
}

void UCoralisWeaponComponent::StopFire()
{
	bTriggerHeld = false;
}

void UCoralisWeaponComponent::Vent()
{
	if (!Settings || !IsLocallyControlledOwner() || IsOwnerDead())
	{
		return;
	}
	const double Now = GetLocalTime();
	if (!Heat.TryVent(Now, *Settings))
	{
		return;
	}
	if (Settings->bNewTriggerPullAfterVent)
	{
		bTriggerHeld = false;
	}
	OnVentStarted.Broadcast(Heat.Phase == ECoralisHeatPhase::SweetVenting);
	if (GetOwner()->HasAuthority())
	{
		CommitServerState();
	}
	else
	{
		ServerVent(Now);
	}
}

void UCoralisWeaponComponent::DebugSetHeat(float Fraction)
{
#if !UE_BUILD_SHIPPING
	if (!Settings || !GetOwner())
	{
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		const double Now = GetStateTime();
		Heat.ForceHeat(Now, Fraction * Settings->MaxHeat, *Settings);
		CommitServerState();
		if (!IsLocallyControlledOwner())
		{
			ClientCorrectHeat(Heat, Now);
		}
	}
	else
	{
		ServerDebugSetHeat(Fraction);
	}
#endif
}

void UCoralisWeaponComponent::ServerDebugSetHeat_Implementation(float Fraction)
{
	// Editor scripts run RPCs on the calling machine. Without this check a client would call itself until the stack overflows.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		DebugSetHeat(Fraction);
	}
}

void UCoralisWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Settings || !GetOwner())
	{
		return;
	}

	if (GetOwner()->HasAuthority() && PendingInputs.Num() > 0)
	{
		ProcessPendingInputs();
	}

	if (bTriggerHeld && IsLocallyControlledOwner())
	{
		const double Now = GetLocalTime();
		const ECoralisHeatPhase Phase = Heat.At(Now, *Settings).Phase;
		if (Phase != ECoralisHeatPhase::Ready)
		{
			const bool bNeedsNewPull = Phase == ECoralisHeatPhase::Jammed
				? Settings->bNewTriggerPullAfterJam : Settings->bNewTriggerPullAfterVent;
			if (bNeedsNewPull)
			{
				bTriggerHeld = false;
			}
			// A trigger kept held fires from the moment the weapon is ready again, not from before it.
			NextShotTime = Now;
		}
		else if (!CanOwnerUseWeapon())
		{
			NextShotTime = Now;
		}
		else if (Now >= NextShotTime)
		{
			// Stamped with the time the shot was due (within this frame), so the fire rate doesn't depend on the frame rate.
			TryFireShot(FMath::Max(NextShotTime, Now - DeltaTime));
		}
	}

#if !UE_BUILD_SHIPPING
	if (CVarShowWeaponDebug.GetValueOnGameThread() && GEngine)
	{
		const FCoralisHeatState State = GetHeatState();
		float WindowStart, WindowEnd;
		State.GetSweetSpotWindow(*Settings, WindowStart, WindowEnd);
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::Orange,
			FString::Printf(TEXT("%s  heat %.1f / %.0f  %s  buff %.1f s  sweet spot %.0f-%.0f%%"), *GetOwner()->GetName(), State.Heat,
				Settings->MaxHeat, *UEnum::GetDisplayValueAsText(State.Phase).ToString(), GetBuffTimeRemaining(),
				WindowStart * 100.f, WindowEnd * 100.f));
	}
#endif
}

void UCoralisWeaponComponent::TryFireShot(double ShotTime)
{
	if (!CanOwnerUseWeapon())
	{
		return;
	}
	// A shot can't be older than the last thing that happened to the heat.
	ShotTime = FMath::Max(ShotTime, Heat.Time);
	if (!Heat.TryFire(ShotTime, *Settings))
	{
		return;
	}
	NextShotTime = ShotTime + Settings->GetFireInterval();

	FVector Origin, Direction;
	GetShotView(Origin, Direction);

	if (GetOwner()->HasAuthority())
	{
		ResolveShot(Origin, Direction, Heat.IsBuffActive(ShotTime));
		CommitServerState();
	}
	else
	{
		FHitResult Hit;
		FVector Muzzle, End;
		TraceShot(Origin, Direction, Hit, Muzzle, End);
		PlayShotCosmetics(Muzzle, End);
		ServerFire(ShotTime, Origin, Direction);
	}
	ApplyRecoil();
}

// --- Shot ------------------------------------------------------------------------------------------------------------

void UCoralisWeaponComponent::GetShotView(FVector& OutOrigin, FVector& OutDirection) const
{
	FRotator ViewRotation = GetOwner()->GetActorRotation();
	OutOrigin = GetOwner()->GetActorLocation();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->GetController())
	{
		Pawn->GetController()->GetPlayerViewPoint(OutOrigin, ViewRotation);
	}
	const float HalfAngle = IsOwnerAiming() ? Settings->AimSpread : Settings->HipSpread;
	OutDirection = FMath::VRandCone(ViewRotation.Vector(), FMath::DegreesToRadians(HalfAngle));
}

FVector UCoralisWeaponComponent::GetMuzzleLocation(const FVector& AimDirection) const
{
	const FRotator YawOnly(0.f, AimDirection.Rotation().Yaw, 0.f);
	FVector Offset = Settings->MuzzleOffset;
	const ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(GetOwner());
	if (Character && Character->IsLeftHanded())
	{
		Offset.Y = -Offset.Y;
	}
	return GetOwner()->GetActorLocation() + YawOnly.RotateVector(Offset);
}

bool UCoralisWeaponComponent::TraceShot(const FVector& Origin, const FVector& Direction, FHitResult& OutHit, FVector& OutMuzzle, FVector& OutEnd) const
{
	const UWorld* World = GetWorld();
	OutMuzzle = GetMuzzleLocation(Direction);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CoralisWeaponShot), true, GetOwner());
	TArray<AActor*> Attached;
	GetOwner()->GetAttachedActors(Attached);
	Params.AddIgnoredActors(Attached);

	// The view starts behind the shooter: skip what lies between the camera and the muzzle's depth.
	const float SkipDistance = FMath::Max(0.f, static_cast<float>(FVector::DotProduct(OutMuzzle - Origin, Direction)));
	const FVector ViewStart = Origin + Direction * SkipDistance;
	const FVector ViewEnd = Origin + Direction * (SkipDistance + Settings->Range);
	FHitResult ViewHit;
	const FVector AimPoint = World->LineTraceSingleByChannel(ViewHit, ViewStart, ViewEnd, Settings->TraceChannel, Params)
		? ViewHit.ImpactPoint : ViewEnd;

	// Runs a little past the aim point so the second trace still reaches the surface it is aimed at.
	const FVector ToAim = (AimPoint - OutMuzzle).GetSafeNormal();
	const FVector MuzzleEnd = AimPoint + ToAim * 10.f;
	if (World->LineTraceSingleByChannel(OutHit, OutMuzzle, MuzzleEnd, Settings->TraceChannel, Params))
	{
		OutEnd = OutHit.ImpactPoint;
		return true;
	}
	OutEnd = AimPoint;
	return false;
}

void UCoralisWeaponComponent::ResolveShot(const FVector& Origin, const FVector& Direction, bool bBuffed)
{
	FHitResult Hit;
	FVector Muzzle, End;
	const bool bHit = TraceShot(Origin, Direction, Hit, Muzzle, End);
	MulticastShot(Muzzle, End);

	AActor* Target = bHit ? Hit.GetActor() : nullptr;
	OnServerShot.Broadcast(*this, Muzzle, End, Target);
	if (!Target)
	{
		return;
	}
	const APawn* TargetPawn = Cast<APawn>(Target);
	const APawn* Shooter = Cast<APawn>(GetOwner());
	// Two sides for now: player controlled pawns and every other pawn.
	if (!Settings->bFriendlyFire && TargetPawn && Shooter && TargetPawn->IsPlayerControlled() == Shooter->IsPlayerControlled())
	{
		return;
	}

	AController* ShooterController = Shooter ? Shooter->GetController() : nullptr;
	const float Damage = Settings->Damage * (bBuffed ? Settings->BuffDamageMultiplier : 1.f);

	if (UCoralisHealthComponent* Health = Target->FindComponentByClass<UCoralisHealthComponent>())
	{
		const bool bWasDepleted = Health->IsDepleted();
		ECoralisHitZone Zone = ECoralisHitZone::Body;
		const float Taken = Health->ApplyHit(Damage, Hit, ShooterController, Zone);
		if (CVarShowWeaponDebug.GetValueOnGameThread())
		{
			UE_LOG(LogCoralisWeapon, Log, TEXT("Hit %s bone %s zone %s damage %.2f%s health %.2f"), *Target->GetName(),
				*Hit.BoneName.ToString(), *UEnum::GetDisplayValueAsText(Zone).ToString(), Taken,
				bBuffed ? TEXT(" (buffed)") : TEXT(""), Health->GetHealth());
		}
		if (Taken > 0.f)
		{
			ClientConfirmHit(Taken, Zone, !bWasDepleted && Health->IsDepleted());
		}
	}
	else
	{
		// Anything without Coralis health still gets ordinary engine damage.
		const float Taken = UGameplayStatics::ApplyPointDamage(Target, Damage, Direction, Hit, ShooterController, GetOwner(), nullptr);
		// Walls also accept engine damage, so only a pawn counts as a hit. One that destroys itself when hit counts as a kill.
		if (TargetPawn && Taken > 0.f)
		{
			ClientConfirmHit(Taken, ECoralisHitZone::Body, Target->IsActorBeingDestroyed());
		}
	}
}

void UCoralisWeaponComponent::ApplyRecoil()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController() || PC->IsLookInputIgnored())
	{
		return;
	}
	// Set on the control rotation itself: held-fire shots go off after the controller's tick, which clears queued
	// rotation input. The camera manager still clamps the pitch on its next update.
	const float Scale = IsOwnerAiming() ? Settings->AimRecoilScale : 1.f;
	FRotator Rotation = PC->GetControlRotation();
	Rotation.Pitch = FRotator::NormalizeAxis(Rotation.Pitch) + Settings->RecoilPitch * Scale;
	Rotation.Yaw += FMath::FRandRange(-Settings->RecoilYaw, Settings->RecoilYaw) * Scale;
	PC->SetControlRotation(Rotation);
}

void UCoralisWeaponComponent::PlayShotCosmetics(const FVector& Start, const FVector& End)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (Settings->FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, Settings->FireSound, Start);
		}
#if ENABLE_DRAW_DEBUG
		if (Settings->TracerDuration > 0.f)
		{
			DrawDebugLine(GetWorld(), Start, End, Settings->TracerColor, false, Settings->TracerDuration, 0, 1.5f);
		}
#endif
	}
	OnShotFired.Broadcast(Start, End);
}

void UCoralisWeaponComponent::MulticastShot_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End)
{
	// A remote owner already showed this shot when it fired.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Settings || (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->HasAuthority()))
	{
		return;
	}
	PlayShotCosmetics(Start, End);
}

void UCoralisWeaponComponent::ClientConfirmHit_Implementation(float Damage, ECoralisHitZone Zone, bool bDepleted)
{
	OnHitConfirmed.Broadcast(Damage, Zone, bDepleted);
}

// --- Server ----------------------------------------------------------------------------------------------------------

bool UCoralisWeaponComponent::CheckClientClock(double ClientTime)
{
	const double ServerNow = GetLocalTime();
	const double Offset = ServerNow - ClientTime;
	if (!bHasClientClock)
	{
		ClientClockOffset = Offset;
		bHasClientClock = true;
		// Anything that happened to the heat so far (debug commands) was timed on the server's clock.
		Heat.ShiftTimes(-Offset);
		return true;
	}
	// How far the client has run ahead of where its clock should be by now.
	const double Lead = ClientClockOffset - Offset;
	if (Lead > Settings->ClientClockTolerance)
	{
		return false;
	}
	if (Lead < -Settings->ClientClockTolerance)
	{
		// Far behind (a hitch or a lag spike): follow it, or later inputs would be allowed to run ahead by that much.
		ClientClockOffset = Offset;
	}
	return true;
}

void UCoralisWeaponComponent::ServerFire_Implementation(double ClientTime, FVector_NetQuantize10 Origin, FVector_NetQuantizeNormal Direction)
{
	if (!Settings)
	{
		return;
	}
	if (!CheckClientClock(ClientTime)
		|| FVector::Dist(Origin, GetOwner()->GetActorLocation()) > Settings->MaxAimOriginDistance)
	{
		RefuseInput();
		return;
	}
	FPendingInput& Input = PendingInputs.AddDefaulted_GetRef();
	Input.ClientTime = ClientTime;
	Input.ReceivedTime = GetLocalTime();
	Input.Origin = Origin;
	Input.Direction = Direction.GetSafeNormal();
	ProcessPendingInputs();
}

void UCoralisWeaponComponent::ServerVent_Implementation(double ClientTime)
{
	if (!Settings)
	{
		return;
	}
	if (!CheckClientClock(ClientTime))
	{
		RefuseInput();
		return;
	}
	FPendingInput& Input = PendingInputs.AddDefaulted_GetRef();
	Input.ClientTime = ClientTime;
	Input.ReceivedTime = GetLocalTime();
	Input.bVent = true;
	ProcessPendingInputs();
}

void UCoralisWeaponComponent::ProcessPendingInputs()
{
	while (PendingInputs.Num() > 0)
	{
		const FPendingInput Input = PendingInputs[0];
		bool bAccepted = false;
		if (Input.bVent)
		{
			bAccepted = !IsOwnerDead() && Heat.TryVent(Input.ClientTime, *Settings);
			if (bAccepted)
			{
				OnVentStarted.Broadcast(Heat.Phase == ECoralisHeatPhase::SweetVenting);
			}
		}
		else if (!CanOwnerUseWeapon())
		{
			// The shot can arrive before the move that ended a sprint or roll: give the movement a moment to catch up.
			if (GetLocalTime() - Input.ReceivedTime < Settings->ServerMoveStateGrace)
			{
				return;
			}
		}
		else
		{
			bAccepted = Input.ClientTime >= Heat.Time && Heat.TryFire(Input.ClientTime, *Settings);
			if (bAccepted)
			{
				ResolveShot(Input.Origin, Input.Direction, Heat.IsBuffActive(Input.ClientTime));
			}
		}
		PendingInputs.RemoveAt(0);
		if (bAccepted)
		{
			CommitServerState();
		}
		else
		{
			RefuseInput();
		}
	}
}

void UCoralisWeaponComponent::RefuseInput()
{
	// The server's heat is on the owner's clock, so send that clock's current time along with it.
	const double OwnerClockNow = bHasClientClock ? GetLocalTime() - ClientClockOffset : Heat.Time;
	UE_LOG(LogCoralisWeapon, Verbose, TEXT("%s: refused weapon input, correcting the owner"), *GetNameSafe(GetOwner()));
	ClientCorrectHeat(Heat, OwnerClockNow);
}

void UCoralisWeaponComponent::ClientCorrectHeat_Implementation(const FCoralisHeatState& ServerState, double ServerClockNow)
{
	if (!Settings)
	{
		return;
	}
	Heat = ServerState;
	Heat.ShiftTimes(GetLocalTime() - ServerClockNow);
}

void UCoralisWeaponComponent::CommitServerState()
{
	ProxyHeat = Heat;
	if (bHasClientClock && !IsLocallyControlledOwner())
	{
		ProxyHeat.ShiftTimes(ClientClockOffset);
	}
}

void UCoralisWeaponComponent::OnRep_ProxyHeat()
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	ProxyClockOffset = GameState ? GameState->GetServerWorldTimeSeconds() - GetLocalTime() : 0.0;
}

// --- State -----------------------------------------------------------------------------------------------------------

double UCoralisWeaponComponent::GetLocalTime() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

double UCoralisWeaponComponent::GetStateTime() const
{
	// The server's copy of a remote owner's weapon lives on that owner's clock.
	return (bHasClientClock && !IsLocallyControlledOwner()) ? GetLocalTime() - ClientClockOffset : GetLocalTime();
}

FCoralisHeatState UCoralisWeaponComponent::GetHeatState() const
{
	if (!Settings || !GetOwner())
	{
		return FCoralisHeatState();
	}
	if (GetOwner()->HasAuthority() || IsLocallyControlledOwner())
	{
		return Heat.At(GetStateTime(), *Settings);
	}
	return ProxyHeat.At(GetLocalTime() + ProxyClockOffset, *Settings);
}

float UCoralisWeaponComponent::GetHeatFraction() const
{
	return Settings ? GetHeatState().Heat / FMath::Max(Settings->MaxHeat, 1.f) : 0.f;
}

ECoralisHeatPhase UCoralisWeaponComponent::GetHeatPhase() const
{
	return GetHeatState().Phase;
}

float UCoralisWeaponComponent::GetBuffTimeRemaining() const
{
	const FCoralisHeatState State = GetHeatState();
	return static_cast<float>(FMath::Max(0.0, State.BuffEndTime - State.Time));
}

bool UCoralisWeaponComponent::IsBuffActive() const
{
	return GetBuffTimeRemaining() > 0.f;
}

void UCoralisWeaponComponent::GetSweetSpotWindow(float& Start, float& End) const
{
	Start = End = 0.f;
	if (Settings)
	{
		GetHeatState().GetSweetSpotWindow(*Settings, Start, End);
	}
}

bool UCoralisWeaponComponent::IsLocallyControlledOwner() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Pawn->IsLocallyControlled() : (GetOwner() && GetOwner()->HasAuthority());
}

bool UCoralisWeaponComponent::IsOwnerDead() const
{
	const UCoralisHealthComponent* Health = GetOwner() ? GetOwner()->FindComponentByClass<UCoralisHealthComponent>() : nullptr;
	return Health && Health->HasBegunPlay() && Health->IsDepleted();
}

bool UCoralisWeaponComponent::CanOwnerUseWeapon() const
{
	if (IsOwnerDead())
	{
		return false;
	}
	const ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(GetOwner());
	const UCoralisMovementComponent* Move = Character ? Character->GetCoralisMovement() : nullptr;
	return !Move || Move->CanUseWeapon();
}

bool UCoralisWeaponComponent::IsOwnerAiming() const
{
	const ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(GetOwner());
	const UCoralisMovementComponent* Move = Character ? Character->GetCoralisMovement() : nullptr;
	return Move && Move->IsAiming();
}
