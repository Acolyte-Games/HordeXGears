// Copyright Acolyte Games. All Rights Reserved.

#include "UI/CoralisWaveWidget.h"

#include "Rendering/DrawElements.h"

int32 UCoralisWaveWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;

	// Line height as a multiple of the font size.
	constexpr float LineSpacing = 1.8f;
	const float TitleHeight = Style.TitleFont.Size * LineSpacing;
	const float LineHeight = Style.Font.Size * LineSpacing;
	const FVector2D Size(Style.Width, TitleHeight + Lines.Num() * LineHeight + Style.Padding * 2.f);

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Style.Margin)),
		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Style.BackColor);

	const FVector2D Screen = AllottedGeometry.GetLocalSize();
	FVector2D Position = Style.Margin + FVector2D(Style.Padding, Style.Padding);
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(Screen, FSlateLayoutTransform(Position)),
		Title, Style.TitleFont, ESlateDrawEffect::None, Style.TitleColor);
	Position.Y += TitleHeight;
	for (const FText& Line : Lines)
	{
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(Screen, FSlateLayoutTransform(Position)),
			Line, Style.Font, ESlateDrawEffect::None, Style.TextColor);
		Position.Y += LineHeight;
	}
	return LayerId + 1;
}
