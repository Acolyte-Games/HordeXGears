// Copyright Acolyte Games. All Rights Reserved.

#include "Movement/CoralisMovementSettings.h"

namespace
{
	// Resolution used to integrate the roll speed curve. Not a gameplay value.
	constexpr int32 DodgeCurveSamples = 64;
}

UCoralisMovementSettings::UCoralisMovementSettings()
{
	JogTurn.MaxRate = 600.f;
	JogTurn.MinRate = 120.f;
	JogTurn.EaseAngle = 45.f;

	AimTurn.MaxRate = 900.f;
	AimTurn.MinRate = 240.f;
	AimTurn.EaseAngle = 20.f;

	SprintTurn.MaxRate = 140.f;
	SprintTurn.MinRate = 60.f;
	SprintTurn.EaseAngle = 30.f;

	// Heavy start that builds up gradually: about a third of the acceleration from standstill, smoothly rising to full
	// only near full pace (cubic keys, so there is no sudden kick).
	FRichCurve* Startup = StartupCurve.GetRichCurve();
	for (const FVector2f& Key : { FVector2f(0.f, 0.32f), FVector2f(0.35f, 0.45f), FVector2f(0.7f, 0.75f), FVector2f(1.f, 1.f) })
	{
		const FKeyHandle Handle = Startup->AddKey(Key.X, Key.Y);
		Startup->SetKeyInterpMode(Handle, RCIM_Cubic);
	}
	Startup->AutoSetTangents();

	// Placeholder roll shape: fast burst, then bleed off.
	FRichCurve* Curve = DodgeSpeedCurve.GetRichCurve();
	Curve->AddKey(0.f, 1.3f);
	Curve->AddKey(0.55f, 1.1f);
	Curve->AddKey(1.f, 0.35f);
}

void UCoralisMovementSettings::RefreshDodgeCurveStats() const
{
	const FRichCurve* Curve = DodgeSpeedCurve.GetRichCurveConst();
	if (!Curve || Curve->GetNumKeys() == 0)
	{
		CachedCurveMean = 1.f;
		CachedCurveMax = 1.f;
		return;
	}
	float Sum = 0.f;
	float Max = 0.f;
	for (int32 i = 0; i < DodgeCurveSamples; ++i)
	{
		const float T = (i + 0.5f) / DodgeCurveSamples;
		const float V = FMath::Max(0.f, Curve->Eval(T));
		Sum += V;
		Max = FMath::Max(Max, V);
	}
	CachedCurveMean = Sum > UE_KINDA_SMALL_NUMBER ? Sum / DodgeCurveSamples : 1.f;
	CachedCurveMax = Max > UE_KINDA_SMALL_NUMBER ? Max : 1.f;
}

float UCoralisMovementSettings::GetDodgeSpeedAt(float Alpha) const
{
	if (CachedCurveMean < 0.f)
	{
		RefreshDodgeCurveStats();
	}
	const float BaseSpeed = DodgeDistance / FMath::Max(DodgeDuration, UE_KINDA_SMALL_NUMBER);
	const FRichCurve* Curve = DodgeSpeedCurve.GetRichCurveConst();
	const float Shape = (Curve && Curve->GetNumKeys() > 0) ? FMath::Max(0.f, Curve->Eval(FMath::Clamp(Alpha, 0.f, 1.f))) : 1.f;
	return BaseSpeed * Shape / CachedCurveMean;
}

float UCoralisMovementSettings::GetStartupScale(float SpeedFraction) const
{
	const FRichCurve* Curve = StartupCurve.GetRichCurveConst();
	if (!Curve || Curve->GetNumKeys() == 0)
	{
		return 1.f;
	}
	return FMath::Max(0.f, Curve->Eval(FMath::Clamp(SpeedFraction, 0.f, 1.f)));
}

float UCoralisMovementSettings::GetDodgePeakSpeed() const
{
	if (CachedCurveMean < 0.f)
	{
		RefreshDodgeCurveStats();
	}
	const float BaseSpeed = DodgeDistance / FMath::Max(DodgeDuration, UE_KINDA_SMALL_NUMBER);
	return BaseSpeed * CachedCurveMax / CachedCurveMean;
}

#if WITH_EDITOR
void UCoralisMovementSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	CachedCurveMean = -1.f;
	CachedCurveMax = -1.f;
}
#endif
