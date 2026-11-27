// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/CoralisHitZoneSettings.h"
#include "Components/ActorComponent.h"
#include "UI/CoralisCrosshairWidget.h"
#include "CoralisWeaponHUDComponent.generated.h"

class UCoralisWeaponComponent;
class UProgressBar;
class UTextBlock;

/**
 * Shows the owner's weapon to the local player. Creates the HUD widget and writes heat into its progress bar and text,
 * found by name, so the widget needs no code of its own. Also puts the placeholder crosshair overlay on screen.
 * Does nothing on other players' characters, on AI and on dedicated servers.
 */
UCLASS(ClassGroup = (Coralis), meta = (BlueprintSpawnableComponent))
class WAVESHOOTERPROTO_API UCoralisWeaponHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoralisWeaponHUDComponent();

	/** The HUD to create and fill. Unset = crosshair and hit marker only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	/** Name of the heat progress bar inside the HUD widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD")
	FName HeatBarName = TEXT("ProgressBar_Heat");

	/** Name of the heat text inside the HUD widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD")
	FName HeatLabelName = TEXT("Txt_HeatLabel");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD")
	int32 HUDZOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FText ReadyText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FText VentingText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FText SweetSpotText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FText JammedText;

	/** Added after the text while the damage buff runs. {Seconds} is the time left. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FText BuffSuffix;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FLinearColor ReadyColor = FLinearColor(0.95f, 0.45f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FLinearColor VentingColor = FLinearColor(0.15f, 0.6f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FLinearColor SweetSpotColor = FLinearColor(0.2f, 1.f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Heat")
	FLinearColor JammedColor = FLinearColor(0.9f, 0.05f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Crosshair")
	FCoralisCrosshairStyle Crosshair;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|HUD|Crosshair")
	int32 CrosshairZOrder = 10;

	/** The HUD widget on screen, if any. */
	UFUNCTION(BlueprintPure, Category = "Coralis|HUD")
	UUserWidget* GetHUDWidget() const { return HUDWidget; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CreateWidgets(APlayerController* PC);
	void RemoveWidgets();
	void UpdateHeat(const UCoralisWeaponComponent& Weapon);

	UFUNCTION()
	void HandleHitConfirmed(float Damage, ECoralisHitZone Zone, bool bDepleted);

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UCoralisCrosshairWidget> CrosshairWidget;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HeatBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeatLabel;
};
