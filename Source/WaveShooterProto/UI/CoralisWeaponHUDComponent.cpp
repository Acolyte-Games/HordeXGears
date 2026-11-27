// Copyright Acolyte Games. All Rights Reserved.

#include "UI/CoralisWeaponHUDComponent.h"

#include "Characters/CoralisCharacterBase.h"
#include "Combat/CoralisWeaponComponent.h"
#include "Combat/CoralisWeaponSettings.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Movement/CoralisMovementComponent.h"

#define LOCTEXT_NAMESPACE "CoralisWeaponHUD"

UCoralisWeaponHUDComponent::UCoralisWeaponHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	ReadyText = LOCTEXT("Ready", "HEAT");
	VentingText = LOCTEXT("Venting", "VENTING");
	SweetSpotText = LOCTEXT("SweetSpot", "SWEET SPOT");
	JammedText = LOCTEXT("Jammed", "JAMMED");
	BuffSuffix = LOCTEXT("Buff", "  +DMG {Seconds}s");
}

void UCoralisWeaponHUDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(GetOwner());
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	const UCoralisWeaponComponent* Weapon = Character ? Character->GetWeapon() : nullptr;
	if (!PC || !PC->IsLocalController() || !Weapon || !Weapon->Settings)
	{
		RemoveWidgets();
		return;
	}
	if (!CrosshairWidget)
	{
		CreateWidgets(PC);
	}
	UpdateHeat(*Weapon);

	if (CrosshairWidget)
	{
		const UCoralisMovementComponent* Move = Character->GetCoralisMovement();
		CrosshairWidget->Style = Crosshair;
		CrosshairWidget->Gap = Move && Move->IsAiming() ? Crosshair.AimGap : Crosshair.HipGap;
		const bool bReady = Weapon->GetHeatPhase() == ECoralisHeatPhase::Ready;
		CrosshairWidget->bBlocked = !bReady || (Move && !Move->CanUseWeapon());
		// The window is only marked while a vent can start, so it never jumps under a running vent.
		float WindowStart = 0.f;
		float WindowEnd = 0.f;
		if (bReady)
		{
			Weapon->GetSweetSpotWindow(WindowStart, WindowEnd);
		}
		CrosshairWidget->SweetSpotStart = WindowStart;
		CrosshairWidget->SweetSpotEnd = WindowEnd;
	}
}

void UCoralisWeaponHUDComponent::CreateWidgets(APlayerController* PC)
{
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(PC, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToPlayerScreen(HUDZOrder);
			HeatBar = Cast<UProgressBar>(HUDWidget->GetWidgetFromName(HeatBarName));
			HeatLabel = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(HeatLabelName));
			if (!HeatBar || !HeatLabel)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: %s has no progress bar '%s' and/or text '%s'"), *GetNameSafe(GetOwner()),
					*HUDWidget->GetClass()->GetName(), *HeatBarName.ToString(), *HeatLabelName.ToString());
			}
		}
	}

	CrosshairWidget = CreateWidget<UCoralisCrosshairWidget>(PC, UCoralisCrosshairWidget::StaticClass());
	if (CrosshairWidget)
	{
		CrosshairWidget->HeatBar = HeatBar.Get();
		CrosshairWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		CrosshairWidget->AddToPlayerScreen(CrosshairZOrder);
	}

	if (const ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(GetOwner()))
	{
		Character->GetWeapon()->OnHitConfirmed.AddUniqueDynamic(this, &ThisClass::HandleHitConfirmed);
	}
}

void UCoralisWeaponHUDComponent::RemoveWidgets()
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
	}
	if (CrosshairWidget)
	{
		CrosshairWidget->RemoveFromParent();
	}
	HUDWidget = nullptr;
	CrosshairWidget = nullptr;
	HeatBar = nullptr;
	HeatLabel = nullptr;
}

void UCoralisWeaponHUDComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveWidgets();
	Super::EndPlay(EndPlayReason);
}

void UCoralisWeaponHUDComponent::UpdateHeat(const UCoralisWeaponComponent& Weapon)
{
	const ECoralisHeatPhase Phase = Weapon.GetHeatPhase();
	const FText* Text = &ReadyText;
	const FLinearColor* Color = &ReadyColor;
	switch (Phase)
	{
	case ECoralisHeatPhase::Venting: Text = &VentingText; Color = &VentingColor; break;
	case ECoralisHeatPhase::SweetVenting: Text = &SweetSpotText; Color = &SweetSpotColor; break;
	case ECoralisHeatPhase::Jammed: Text = &JammedText; Color = &JammedColor; break;
	default: break;
	}

	if (HeatBar)
	{
		HeatBar->SetPercent(Weapon.GetHeatFraction());
		HeatBar->SetFillColorAndOpacity(*Color);
	}
	if (HeatLabel)
	{
		FText Shown = *Text;
		const float BuffLeft = Weapon.GetBuffTimeRemaining();
		if (BuffLeft > 0.f)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Seconds"), FText::AsNumber(FMath::CeilToInt(BuffLeft)));
			Shown = FText::Format(INVTEXT("{0}{1}"), Shown, FText::Format(BuffSuffix, Arguments));
		}
		HeatLabel->SetText(Shown);
	}
}

void UCoralisWeaponHUDComponent::HandleHitConfirmed(float Damage, ECoralisHitZone Zone, bool bDepleted)
{
	if (!CrosshairWidget)
	{
		return;
	}
	FLinearColor Color = Crosshair.HitColor;
	if (bDepleted)
	{
		Color = Crosshair.DepletedColor;
	}
	else if (Zone == ECoralisHitZone::Head)
	{
		Color = Crosshair.HeadHitColor;
	}
	else if (Zone == ECoralisHitZone::Arm || Zone == ECoralisHitZone::Leg)
	{
		Color = Crosshair.LimbHitColor;
	}
	CrosshairWidget->ShowHitMarker(Color);
}

#undef LOCTEXT_NAMESPACE
