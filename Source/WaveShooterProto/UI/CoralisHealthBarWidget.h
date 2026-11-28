// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/CoreStyle.h"
#include "CoralisHealthBarWidget.generated.h"

/** Look of the placeholder health bar. Sizes are in HUD units. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisHealthBarStyle
{
	GENERATED_BODY()

	/** Distance of the bar's lower left corner from the lower left corner of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FVector2D Margin = FVector2D(40.0, 40.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FVector2D Size = FVector2D(320.0, 22.0);

	/** Width of the dark frame around the bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0"))
	float Border = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
	FLinearColor BackColor = FLinearColor(0.f, 0.f, 0.f, 0.65f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
	FLinearColor FillColor = FLinearColor(0.12f, 0.65f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
	FLinearColor LowColor = FLinearColor(0.8f, 0.06f, 0.04f);

	/** Below this fraction of full health the bar takes the low colour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors", meta = (ClampMin = "0", ClampMax = "1"))
	float LowFraction = 0.35f;

	/** The bar flashes this colour when damage lands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Flash")
	FLinearColor FlashColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Flash", meta = (ClampMin = "0", Units = "Seconds"))
	float FlashDuration = 0.15f;

	/** Tint over the whole screen when damage lands; its alpha is the strength at the moment of the hit. Alpha 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Flash")
	FLinearColor ScreenFlashColor = FLinearColor(0.6f, 0.f, 0.f, 0.18f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Flash", meta = (ClampMin = "0", Units = "Seconds"))
	float ScreenFlashDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FLinearColor TextColor = FLinearColor::White;

	/** Shown above the bar once health is gone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText DeadText = NSLOCTEXT("CoralisHealthBar", "Dead", "YOU DIED");
};

/** Placeholder health display: a bar in the lower left with a damage flash. Draws only; UCoralisHealthHUDComponent feeds it. */
UCLASS()
class WAVESHOOTERPROTO_API UCoralisHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	FCoralisHealthBarStyle Style;

	float Health = 0.f;
	float MaxHealth = 1.f;

	void ShowDamage();

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	double DamageTime = -1000.0;
};
