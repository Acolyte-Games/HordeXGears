// Copyright Acolyte Games. All Rights Reserved.

#include "Combat/CoralisTargetDummy.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/CoralisHealthComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ACoralisTargetDummy::ACoralisTargetDummy()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Health = CreateDefaultSubobject<UCoralisHealthComponent>(TEXT("Health"));

	Readout = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Readout"));
	Readout->SetupAttachment(GetRootComponent());
	Readout->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	Readout->SetHorizontalAlignment(EHTA_Center);
	Readout->SetWorldSize(22.f);
	Readout->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACoralisTargetDummy::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Health->OnDepleted.AddDynamic(this, &ThisClass::HandleDepleted);
	}
}

void ACoralisTargetDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FCoralisDamageEvent& Last = Health->GetLastDamage();
	FString Text = FString::Printf(TEXT("HP %.1f / %.0f"), Health->GetHealth(), Health->MaxHealth);
	if (Last.Amount > 0.f)
	{
		Text += FString::Printf(TEXT("<br>%s -%.1f"), *UEnum::GetDisplayValueAsText(Last.Zone).ToString(), Last.Amount);
	}
	Readout->SetText(FText::FromString(Text));

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->PlayerCameraManager)
	{
		const FVector ToCamera = PC->PlayerCameraManager->GetCameraLocation() - Readout->GetComponentLocation();
		Readout->SetWorldRotation(FRotator(0.f, ToCamera.Rotation().Yaw, 0.f));
	}
}

void ACoralisTargetDummy::HandleDepleted(AController* InstigatorController)
{
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &ThisClass::Refill, ResetDelay, false);
}

void ACoralisTargetDummy::Refill()
{
	Health->ResetHealth();
}
