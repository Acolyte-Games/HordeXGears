// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/CoralisWaveWidget.h"
#include "CoralisWaveHUDComponent.generated.h"

class ACoralisWaveDirector;

/**
 * Shows the local player the wave number, the enemies left and the countdown to the next wave.
 * Shows nothing in levels without a wave director.
 */
UCLASS(ClassGroup = (Coralis), meta = (BlueprintSpawnableComponent))
class WAVESHOOTERPROTO_API UCoralisWaveHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoralisWaveHUDComponent();

	/** Off once a finished HUD shows the waves itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coralis|HUD")
	bool bShowWavePanel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coralis|HUD")
	FCoralisWaveStyle Style;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD")
	int32 ZOrder = 5;

	/** The panel's text right now, title first. Empty while nothing is shown. */
	UFUNCTION(BlueprintPure, Category = "Coralis|HUD")
	TArray<FText> GetShownLines() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RemoveWidget();

	UPROPERTY(Transient)
	TObjectPtr<UCoralisWaveWidget> Widget;

	TWeakObjectPtr<ACoralisWaveDirector> Director;
};
