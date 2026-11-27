// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoralisCrosshairWidget.generated.h"

/** Look of the placeholder crosshair, hit marker and sweet spot marks. Sizes are in HUD units. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisCrosshairStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair", meta = (ClampMin = "0"))
	float LineLength = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair", meta = (ClampMin = "0"))
	float Thickness = 2.f;

	/** Distance from the centre to each line, from the hip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair", meta = (ClampMin = "0"))
	float HipGap = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair", meta = (ClampMin = "0"))
	float AimGap = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	FLinearColor Color = FLinearColor(1.f, 1.f, 1.f, 0.85f);

	/** While the weapon can't be used (sprinting, rolling, venting, jammed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	FLinearColor BlockedColor = FLinearColor(1.f, 1.f, 1.f, 0.25f);

	/** Drawn behind the crosshair and hit marker so they read on bright surfaces. Faded along with the line it is behind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline")
	FLinearColor OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.7f);

	/** How far the outline shows around each line. 0 turns it off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline", meta = (ClampMin = "0"))
	float OutlineWidth = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Marker", meta = (ClampMin = "0", Units = "Seconds"))
	float HitMarkerDuration = 0.18f;

	/** Inner and outer distance of the hit marker's four strokes from the centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Marker", meta = (ClampMin = "0"))
	float HitMarkerInner = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Marker", meta = (ClampMin = "0"))
	float HitMarkerOuter = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Marker")
	FLinearColor HitColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Marker")
	FLinearColor HeadHitColor = FLinearColor(1.f, 0.8f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Marker")
	FLinearColor LimbHitColor = FLinearColor(0.55f, 0.7f, 0.85f);

	/** The hit that took the last of the target's health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Marker")
	FLinearColor DepletedColor = FLinearColor(1.f, 0.15f, 0.1f);

	/** The marks around the sweet spot window on the heat bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sweet Spot")
	FLinearColor SweetSpotMarkColor = FLinearColor(0.2f, 1.f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sweet Spot", meta = (ClampMin = "0"))
	float SweetSpotMarkThickness = 2.f;

	/** How far the marks stick out above and below the bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sweet Spot", meta = (ClampMin = "0"))
	float SweetSpotMarkOverhang = 3.f;
};

/**
 * Placeholder full screen overlay: crosshair, hit marker, and the sweet spot window drawn over the heat bar of another
 * widget, so that widget stays as it is. Draws only; UCoralisWeaponHUDComponent feeds it.
 */
UCLASS()
class WAVESHOOTERPROTO_API UCoralisCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FCoralisCrosshairStyle Style;

	float Gap = 0.f;
	bool bBlocked = false;

	/** The bar to mark (may be unset) and the window on it as fractions of the bar, left to right. */
	TWeakObjectPtr<const UWidget> HeatBar;
	float SweetSpotStart = 0.f;
	float SweetSpotEnd = 0.f;

	void ShowHitMarker(const FLinearColor& Color);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	FLinearColor HitMarkerColor = FLinearColor::White;
	double HitMarkerEndTime = 0.0;
};
