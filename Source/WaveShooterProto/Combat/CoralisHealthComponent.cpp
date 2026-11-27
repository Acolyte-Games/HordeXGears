// Copyright Acolyte Games. All Rights Reserved.

#include "Combat/CoralisHealthComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UCoralisHealthComponent::UCoralisHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.f || IsDepleted())
	{
		return 0.f;
	}
	const float Taken = FMath::Min(Amount, Health);
	Health -= Taken;
	LastDamage.Amount = Taken;
	LastDamage.Zone = Zone;
	LastDamage.Location = Location;
	++LastDamage.Count;
	// RepNotifies don't run on the server.
	OnRep_Health();
	OnRep_LastDamage();
	if (IsDepleted())
	{
		OnDepleted.Broadcast(InstigatorController);
	}
	return Taken;
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
