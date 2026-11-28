// Copyright Acolyte Games. All Rights Reserved.

#include "Combat/CoralisHealthComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Net/UnrealNetwork.h"

#if !UE_BUILD_SHIPPING
namespace
{
	UCoralisHealthComponent* FindLocalHealth(const UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		return Pawn ? Pawn->FindComponentByClass<UCoralisHealthComponent>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdDamage(
		TEXT("Coralis.Health.Damage"), TEXT("Damages your character by the given amount."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCoralisHealthComponent* Health = FindLocalHealth(World); Health && Args.Num() > 0)
			{
				Health->DebugDamage(FCString::Atof(*Args[0]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdKill(
		TEXT("Coralis.Health.Kill"), TEXT("Takes all of your character's health."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCoralisHealthComponent* Health = FindLocalHealth(World))
			{
				Health->DebugDamage(Health->MaxHealth);
			}
		}));
}
#endif

UCoralisHealthComponent::UCoralisHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Only ticks on the server while there is health to regenerate.
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UCoralisHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCoralisHealthComponent, Health);
	DOREPLIFETIME(UCoralisHealthComponent, LastDamage);
}

void UCoralisHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (Owner->HasAuthority())
	{
		Health = MaxHealth;
		Owner->OnTakeAnyDamage.AddDynamic(this, &ThisClass::HandleAnyDamage);
	}
	if (HitZones && bTraceAgainstMeshBodies)
	{
		SetUpMeshTraces();
	}
}

void UCoralisHealthComponent::SetUpMeshTraces()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh || !Mesh->GetPhysicsAsset())
	{
		return;
	}
	const ECollisionChannel Channel = HitZones->TraceChannel;
	if (Character && Character->GetCapsuleComponent())
	{
		Character->GetCapsuleComponent()->SetCollisionResponseToChannel(Channel, ECR_Ignore);
	}
	if (!CollisionEnabledHasQuery(Mesh->GetCollisionEnabled()))
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	Mesh->SetCollisionResponseToChannel(Channel, ECR_Block);
	if (GetOwner()->HasAuthority())
	{
		// The server may not be rendering this body, but its shots are checked against the posed bones.
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

ECoralisHitZone UCoralisHealthComponent::ResolveHitZone(const FHitResult& Hit, float& OutMultiplier) const
{
	OutMultiplier = 1.f;
	if (!HitZones)
	{
		return ECoralisHitZone::Body;
	}
	OutMultiplier = HitZones->BodyDamageMultiplier;
	const USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Hit.GetComponent());
	if (!Mesh)
	{
		return ECoralisHitZone::Body;
	}
	// Walk from the hit bone toward the root until a listed bone turns up.
	for (FName Bone = Hit.BoneName; !Bone.IsNone(); Bone = Mesh->GetParentBone(Bone))
	{
		for (const FCoralisHitZone& Zone : HitZones->Zones)
		{
			if (Zone.Bones.Contains(Bone))
			{
				OutMultiplier = Zone.DamageMultiplier;
				return Zone.Zone;
			}
		}
	}
	return ECoralisHitZone::Body;
}

float UCoralisHealthComponent::ApplyHit(float BaseDamage, const FHitResult& Hit, AController* InstigatorController, ECoralisHitZone& OutZone)
{
	float Multiplier = 1.f;
	OutZone = ResolveHitZone(Hit, Multiplier);
	return TakeHealth(BaseDamage * Multiplier, OutZone, Hit.ImpactPoint, InstigatorController);
}

void UCoralisHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	TakeHealth(Damage, ECoralisHitZone::Body, DamagedActor ? DamagedActor->GetActorLocation() : FVector::ZeroVector, InstigatedBy);
}

float UCoralisHealthComponent::TakeHealth(float Amount, ECoralisHitZone Zone, const FVector& Location, AController* InstigatorController)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bInvulnerable || Amount <= 0.f || IsDepleted())
	{
		return 0.f;
	}
	const float Taken = FMath::Min(Amount, Health);
	Health -= Taken;
	LastDamage.Amount = Taken;
	LastDamage.Zone = Zone;
	LastDamage.Location = Location;
	++LastDamage.Count;
	LastInstigator = InstigatorController;
	if (RegenRate > 0.f && GetWorld())
	{
		RegenStartTime = GetWorld()->GetTimeSeconds() + RegenDelay;
		SetComponentTickEnabled(true);
	}
	// RepNotifies don't run on the server.
	OnRep_Health();
	OnRep_LastDamage();
	if (IsDepleted())
	{
		OnDepleted.Broadcast(InstigatorController);
	}
	return Taken;
}

void UCoralisHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsDepleted() || Health >= MaxHealth || RegenRate <= 0.f)
	{
		SetComponentTickEnabled(false);
		return;
	}
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= RegenStartTime)
	{
		Health = FMath::Min(MaxHealth, Health + RegenRate * DeltaTime);
		OnRep_Health();
	}
}

void UCoralisHealthComponent::DebugDamage(float Amount)
{
#if !UE_BUILD_SHIPPING
	if (!GetOwner())
	{
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		TakeHealth(Amount, ECoralisHitZone::Body, GetOwner()->GetActorLocation(), nullptr);
	}
	else
	{
		ServerDebugDamage(Amount);
	}
#endif
}

void UCoralisHealthComponent::ServerDebugDamage_Implementation(float Amount)
{
	// Editor scripts run RPCs on the calling machine; without this a client would call itself until the stack overflows.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		DebugDamage(Amount);
	}
}

void UCoralisHealthComponent::ResetHealth()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Health = MaxHealth;
		OnRep_Health();
	}
}

void UCoralisHealthComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UCoralisHealthComponent::OnRep_LastDamage()
{
	OnDamaged.Broadcast(LastDamage);
}
