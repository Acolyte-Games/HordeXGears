// Copyright Acolyte Games. All Rights Reserved.

#include "Combat/CoralisWeaponSettings.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

EDataValidationResult UCoralisWeaponSettings::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!(GetJamCoolRate() < PassiveCoolRate && PassiveCoolRate < VentCoolRate && VentCoolRate < SweetSpotCoolRate))
	{
		Context.AddError(NSLOCTEXT("Coralis", "CoolingOrder",
			"Cooling speeds must go slowest to fastest: jam (MaxHeat / JamDuration), passive, vent, sweet spot vent."));
		Result = EDataValidationResult::Invalid;
	}
	if (SweetSpotRangeEnd - SweetSpotRangeStart < SweetSpotWidth - UE_KINDA_SMALL_NUMBER)
	{
		Context.AddError(NSLOCTEXT("Coralis", "SweetSpotRange",
			"The sweet spot window doesn't fit between Sweet Spot Range Start and Sweet Spot Range End."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
