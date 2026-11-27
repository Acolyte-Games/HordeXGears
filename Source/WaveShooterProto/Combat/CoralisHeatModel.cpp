// Copyright Acolyte Games. All Rights Reserved.

#include "Combat/CoralisHeatModel.h"

#include "Combat/CoralisWeaponSettings.h"

namespace
{
	/** Slack on time comparisons, so rounding can't refuse a shot that is exactly on the fire interval. */
	constexpr double TimeSlack = 1.e-4;
}

FCoralisHeatState FCoralisHeatState::At(double Now, const UCoralisWeaponSettings& Settings) const
{
	FCoralisHeatState Result = *this;
	if (Now <= Time)
	{
		return Result;
	}
	Result.Time = Now;
	const double Elapsed = Now - Time;
	switch (Phase)
	{
	case ECoralisHeatPhase::Jammed:
		{
			const double Left = JamEndTime - Now;
			const double LeftAtAnchor = JamEndTime - Time;
			if (Left <= TimeSlack || LeftAtAnchor <= 0.0)
			{
				Result.Heat = 0.f;
				Result.Phase = ECoralisHeatPhase::Ready;
			}
			else
			{
				Result.Heat = static_cast<float>(Heat * Left / LeftAtAnchor);
			}
			break;
		}
	case ECoralisHeatPhase::Venting:
	case ECoralisHeatPhase::SweetVenting:
		{
			const float Rate = Phase == ECoralisHeatPhase::SweetVenting ? Settings.SweetSpotCoolRate : Settings.VentCoolRate;
			Result.Heat = static_cast<float>(FMath::Max(0.0, Heat - Rate * Elapsed));
			if (Result.Heat <= 0.f)
			{
				Result.Phase = ECoralisHeatPhase::Ready;
			}
			break;
		}
	default:
		{
			const double CoolStart = FMath::Max(Time, LastShotTime + Settings.PassiveCoolDelay);
			Result.Heat = static_cast<float>(FMath::Max(0.0, Heat - Settings.PassiveCoolRate * FMath::Max(0.0, Now - CoolStart)));
			break;
		}
	}
	return Result;
}

bool FCoralisHeatState::CanFire(double Now, const UCoralisWeaponSettings& Settings) const
{
	return At(Now, Settings).Phase == ECoralisHeatPhase::Ready
		&& Now - LastShotTime >= Settings.GetFireInterval() - TimeSlack;
}

bool FCoralisHeatState::TryFire(double Now, const UCoralisWeaponSettings& Settings)
{
	if (!CanFire(Now, Settings))
	{
		return false;
	}
	*this = At(Now, Settings);
	LastShotTime = Now;
	Heat += Settings.HeatPerShot;
	if (Heat >= Settings.MaxHeat - UE_KINDA_SMALL_NUMBER)
	{
		Jam(Now, Settings);
	}
	return true;
}

bool FCoralisHeatState::TryVent(double Now, const UCoralisWeaponSettings& Settings)
{
	const FCoralisHeatState Current = At(Now, Settings);
	if (Current.Phase != ECoralisHeatPhase::Ready || Current.Heat <= 0.f)
	{
		return false;
	}
	*this = Current;
	const float Fraction = Heat / Settings.MaxHeat;
	float WindowStart, WindowEnd;
	GetSweetSpotWindow(Settings, WindowStart, WindowEnd);
	if (WindowEnd > WindowStart && Fraction >= WindowStart && Fraction <= WindowEnd)
	{
		Phase = ECoralisHeatPhase::SweetVenting;
		BuffEndTime = Now + Settings.BuffDuration;
	}
	else
	{
		Phase = ECoralisHeatPhase::Venting;
	}
	MoveSweetSpot();
	return true;
}

void FCoralisHeatState::GetSweetSpotWindow(const UCoralisWeaponSettings& Settings, float& OutStart, float& OutEnd) const
{
	const float Width = FMath::Max(Settings.SweetSpotWidth, 0.f);
	const float LatestStart = FMath::Max(Settings.SweetSpotRangeStart, Settings.SweetSpotRangeEnd - Width);
	OutStart = FMath::Lerp(Settings.SweetSpotRangeStart, LatestStart, FRandomStream(WindowSeed).FRand());
	OutEnd = FMath::Min(OutStart + Width, 1.f);
}

void FCoralisHeatState::ForceHeat(double Now, float NewHeat, const UCoralisWeaponSettings& Settings)
{
	*this = At(Now, Settings);
	Phase = ECoralisHeatPhase::Ready;
	Heat = FMath::Clamp(NewHeat, 0.f, Settings.MaxHeat);
	// Counts as a shot for the passive delay, so the forced value holds long enough to try things on it.
	LastShotTime = Now;
	if (Heat >= Settings.MaxHeat - UE_KINDA_SMALL_NUMBER)
	{
		Jam(Now, Settings);
	}
}

void FCoralisHeatState::Jam(double Now, const UCoralisWeaponSettings& Settings)
{
	Heat = Settings.MaxHeat;
	Phase = ECoralisHeatPhase::Jammed;
	JamEndTime = Now + Settings.JamDuration;
	MoveSweetSpot();
}

void FCoralisHeatState::MoveSweetSpot()
{
	const FRandomStream Stream(WindowSeed);
	Stream.GetUnsignedInt();
	WindowSeed = Stream.GetCurrentSeed();
}
