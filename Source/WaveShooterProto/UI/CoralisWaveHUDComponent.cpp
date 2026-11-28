// Copyright Acolyte Games. All Rights Reserved.

#include "UI/CoralisWaveHUDComponent.h"

#include "Characters/CoralisCharacterBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Waves/CoralisWaveDirector.h"
#include "Waves/CoralisWaveSettings.h"

namespace
{
	/** m:ss, rounded up so the count only reaches 0:00 when it is over. */
	FText FormatCountdown(float Seconds)
	{
		const int32 Whole = FMath::CeilToInt(Seconds);
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60));
	}
}

UCoralisWaveHUDComponent::UCoralisWaveHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

TArray<FText> UCoralisWaveHUDComponent::GetShownLines() const
{
	TArray<FText> Shown;
	if (Widget)
	{
		Shown.Add(Widget->Title);
		Shown.Append(Widget->Lines);
	}
	return Shown;
}

void UCoralisWaveHUDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ACoralisCharacterBase* Character = Cast<ACoralisCharacterBase>(GetOwner());
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!bShowWavePanel || !PC || !PC->IsLocalController())
	{
		RemoveWidget();
		return;
	}
	if (!Director.IsValid())
	{
		TActorIterator<ACoralisWaveDirector> It(GetWorld());
		Director = It ? *It : nullptr;
	}
	if (!Director.IsValid() || Director->GetWaveState().Phase == ECoralisWavePhase::Stopped)
	{
		RemoveWidget();
		return;
	}
	if (!Widget)
	{
		Widget = CreateWidget<UCoralisWaveWidget>(PC, UCoralisWaveWidget::StaticClass());
		if (!Widget)
		{
			return;
		}
		Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		Widget->AddToPlayerScreen(ZOrder);
	}

	const FCoralisWaveState& State = Director->GetWaveState();
	Widget->Style = Style;
	Widget->Title = FText::Format(Style.TitleText, State.Wave);
	Widget->Lines.Reset();
	const FText Countdown = FormatCountdown(Director->GetCountdownRemaining());
	switch (State.Phase)
	{
	case ECoralisWavePhase::Countdown:
		Widget->Lines.Add(FText::Format(State.Wave <= 1 ? Style.FirstWaveText : Style.NextWaveText, Countdown));
		break;

	case ECoralisWavePhase::Fighting:
		Widget->Lines.Add(FText::Format(Style.RemainingText, State.EnemiesRemaining));
		if (Character->IsDead() && Director->GetSettings().bRespawnDeadAtWaveEnd)
		{
			Widget->Lines.Add(Style.WaitingText);
		}
		break;

	case ECoralisWavePhase::Restarting:
		Widget->Lines.Add(FText::Format(Style.RestartText, Countdown));
		break;

	default:
		break;
	}
}

void UCoralisWaveHUDComponent::RemoveWidget()
{
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
}

void UCoralisWaveHUDComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveWidget();
	Super::EndPlay(EndPlayReason);
}
