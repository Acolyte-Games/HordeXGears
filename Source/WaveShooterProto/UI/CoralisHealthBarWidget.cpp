// Copyright Acolyte Games. All Rights Reserved.

#include "UI/CoralisHealthBarWidget.h"

#include "Engine/World.h"
#include "Rendering/DrawElements.h"

void UCoralisHealthBarWidget::ShowDamage()
{
	DamageTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

int32 UCoralisHealthBarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;

	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const auto DrawBox = [&](int32 Layer, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			Brush, ESlateDrawEffect::None, Color);
	};

	const FVector2D Screen = AllottedGeometry.GetLocalSize();
	const double SinceDamage = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) - DamageTime;

	if (Style.ScreenFlashDuration > 0.f && SinceDamage < Style.ScreenFlashDuration && Style.ScreenFlashColor.A > 0.f)
	{
		FLinearColor Tint = Style.ScreenFlashColor;
		Tint.A *= 1.f - static_cast<float>(SinceDamage) / Style.ScreenFlashDuration;
		DrawBox(LayerId, FVector2D::ZeroVector, Screen, Tint);
	}

	const FVector2D BarPosition(Style.Margin.X, Screen.Y - Style.Margin.Y - Style.Size.Y);
	const FVector2D Frame(Style.Border, Style.Border);
	DrawBox(LayerId + 1, BarPosition - Frame, Style.Size + Frame * 2.0, Style.BackColor);

	const float Fraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	FLinearColor Fill = Fraction <= Style.LowFraction ? Style.LowColor : Style.FillColor;
	if (SinceDamage < Style.FlashDuration)
	{
		Fill = Style.FlashColor;
	}
	if (Fraction > 0.f)
	{
		DrawBox(LayerId + 2, BarPosition, FVector2D(Style.Size.X * Fraction, Style.Size.Y), Fill);
	}

	const auto DrawLabel = [&](const FVector2D& Position, const FText& Text)
	{
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(Screen, FSlateLayoutTransform(Position)), Text, Style.Font, ESlateDrawEffect::None, Style.TextColor);
	};
	const float LineHeight = Style.Font.Size * 1.8f;
	// Rounded up, so a sliver of health never reads as zero.
	DrawLabel(BarPosition + FVector2D(8.0, (Style.Size.Y - LineHeight) * 0.5), FText::AsNumber(FMath::CeilToInt(Health)));
	if (Health <= 0.f)
	{
		DrawLabel(BarPosition - FVector2D(0.0, LineHeight + Style.Border), Style.DeadText);
	}
	return LayerId + 3;
}
