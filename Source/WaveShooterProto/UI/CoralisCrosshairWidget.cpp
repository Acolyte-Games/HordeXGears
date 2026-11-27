// Copyright Acolyte Games. All Rights Reserved.

#include "UI/CoralisCrosshairWidget.h"

#include "Engine/World.h"
#include "Rendering/DrawElements.h"

void UCoralisCrosshairWidget::ShowHitMarker(const FLinearColor& Color)
{
	HitMarkerColor = Color;
	HitMarkerEndTime = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) + Style.HitMarkerDuration;
}

int32 UCoralisCrosshairWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;

	const auto DrawLine = [&](const FVector2D& From, const FVector2D& To, const FLinearColor& Color, float Thickness)
	{
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2D>{ From, To }, ESlateDrawEffect::None, Color, true, Thickness);
	};

	// The outline goes one layer below its line, stretched so it also wraps the two ends.
	const auto DrawOutlinedLine = [&](const FVector2D& From, const FVector2D& To, const FLinearColor& Color, float Thickness)
	{
		if (Style.OutlineWidth > 0.f)
		{
			const FVector2D Along = (To - From).GetSafeNormal() * Style.OutlineWidth;
			FLinearColor Outline = Style.OutlineColor;
			Outline.A *= Color.A;
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId - 1, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{ From - Along, To + Along }, ESlateDrawEffect::None, Outline, true, Thickness + 2.f * Style.OutlineWidth);
		}
		DrawLine(From, To, Color, Thickness);
	};
	++LayerId;

	const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5;
	const FLinearColor& CrossColor = bBlocked ? Style.BlockedColor : Style.Color;
	for (const FVector2D& Axis : { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) })
	{
		DrawOutlinedLine(Center + Axis * Gap, Center + Axis * (Gap + Style.LineLength), CrossColor, Style.Thickness);
	}

	if (GetWorld() && GetWorld()->GetTimeSeconds() < HitMarkerEndTime)
	{
		for (const FVector2D& Axis : { FVector2D(1, 1), FVector2D(-1, 1), FVector2D(1, -1), FVector2D(-1, -1) })
		{
			const FVector2D Unit = Axis.GetSafeNormal();
			DrawOutlinedLine(Center + Unit * Style.HitMarkerInner, Center + Unit * Style.HitMarkerOuter, HitMarkerColor, Style.Thickness);
		}
	}

	const UWidget* Bar = HeatBar.Get();
	if (Bar && SweetSpotEnd > SweetSpotStart)
	{
		const FGeometry& BarGeometry = Bar->GetCachedGeometry();
		const FVector2D BarSize = BarGeometry.GetLocalSize();
		if (BarSize.X > 0.0 && BarSize.Y > 0.0)
		{
			// Both cached geometries are in the same space, whatever the paint space is.
			const auto ToLocal = [&](double X, double Y)
			{
				return FVector2D(GetCachedGeometry().AbsoluteToLocal(BarGeometry.LocalToAbsolute(FVector2D(X, Y))));
			};
			const double Left = BarSize.X * SweetSpotStart;
			const double Right = BarSize.X * SweetSpotEnd;
			const double Top = -Style.SweetSpotMarkOverhang;
			const double Bottom = BarSize.Y + Style.SweetSpotMarkOverhang;
			for (const double X : { Left, Right })
			{
				DrawLine(ToLocal(X, Top), ToLocal(X, Bottom), Style.SweetSpotMarkColor, Style.SweetSpotMarkThickness);
			}
			for (const double Y : { Top, Bottom })
			{
				DrawLine(ToLocal(Left, Y), ToLocal(Right, Y), Style.SweetSpotMarkColor, Style.SweetSpotMarkThickness);
			}
		}
	}
	return LayerId;
}
