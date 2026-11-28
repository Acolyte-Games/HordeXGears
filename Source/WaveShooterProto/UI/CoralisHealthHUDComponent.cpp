// Copyright Acolyte Games. All Rights Reserved.

#include "UI/CoralisHealthHUDComponent.h"

#include "Characters/CoralisCharacterBase.h"
#include "Combat/CoralisHealthComponent.h"
#include "GameFramework/PlayerController.h"

UCoralisHealthHUDComponent::UCoralisHealthHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCoralisHealthHUDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(GetOwner());
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	const UCoralisHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
	if (!bShowHealthBar || !PC || !PC->IsLocalController() || !Health)
	{
		RemoveWidget();
		return;
	}
	if (!Widget)
	{
		Widget = CreateWidget<UCoralisHealthBarWidget>(PC, UCoralisHealthBarWidget::StaticClass());
		if (!Widget)
		{
			return;
		}
		Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		Widget->AddToPlayerScreen(ZOrder);
		SeenDamageCount = Health->GetLastDamage().Count;
	}
	Widget->Style = Style;
	Widget->Health = Health->GetHealth();
	Widget->MaxHealth = Health->MaxHealth;
	if (Health->GetLastDamage().Count != SeenDamageCount)
	{
		SeenDamageCount = Health->GetLastDamage().Count;
		Widget->ShowDamage();
	}
}

void UCoralisHealthHUDComponent::RemoveWidget()
{
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
}

void UCoralisHealthHUDComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveWidget();
	Super::EndPlay(EndPlayReason);
}
