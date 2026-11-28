// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/CoralisHealthBarWidget.h"
#include "CoralisHealthHUDComponent.generated.h"

class APlayerController;

/**
 * Shows the owner's health to the local player with a placeholder bar.
 * Does nothing on other players' characters, on AI and on dedicated servers.
 */
UCLASS(ClassGroup = (Coralis), meta = (BlueprintSpawnableComponent))
class WAVESHOOTERPROTO_API UCoralisHealthHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoralisHealthHUDComponent();

	/** Off once a finished HUD shows health itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coralis|HUD")
	bool bShowHealthBar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coralis|HUD")
	FCoralisHealthBarStyle Style;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD")
	int32 ZOrder = 5;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RemoveWidget();

	UPROPERTY(Transient)
	TObjectPtr<UCoralisHealthBarWidget> Widget;

	/** Damage count already shown, to spot a new hit. */
	uint8 SeenDamageCount = 0;
};
