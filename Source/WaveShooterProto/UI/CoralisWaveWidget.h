// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/CoreStyle.h"
#include "CoralisWaveWidget.generated.h"

/** Look and wording of the placeholder wave panel. Sizes are in HUD units. {0} is the number or time. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisWaveStyle
{
	GENERATED_BODY()

	/** Distance of the panel's upper left corner from the upper left corner of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	FVector2D Margin = FVector2D(40.0, 40.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0"))
	float Padding = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0"))
	float Width = 340.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
	FLinearColor BackColor = FLinearColor(0.f, 0.f, 0.f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
	FLinearColor TitleColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
	FLinearColor TextColor = FLinearColor(0.8f, 0.8f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 13);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText TitleText = NSLOCTEXT("CoralisWaves", "Title", "WAVE {0}");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText RemainingText = NSLOCTEXT("CoralisWaves", "Remaining", "Enemies remaining: {0}");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText FirstWaveText = NSLOCTEXT("CoralisWaves", "FirstWave", "First wave in {0}");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText NextWaveText = NSLOCTEXT("CoralisWaves", "NextWave", "Wave cleared. Next wave in {0}");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText RestartText = NSLOCTEXT("CoralisWaves", "Restart", "Run lost. Restarting in {0}");

	/** Extra line for a dead player while the wave goes on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText WaitingText = NSLOCTEXT("CoralisWaves", "Waiting", "You return when the wave is cleared");
};

/** Placeholder wave display in the upper left: a title and lines under it. Draws only; UCoralisWaveHUDComponent feeds it. */
UCLASS()
class WAVESHOOTERPROTO_API UCoralisWaveWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	FCoralisWaveStyle Style;

	FText Title;
	TArray<FText> Lines;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
