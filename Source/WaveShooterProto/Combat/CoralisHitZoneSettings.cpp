// Copyright Acolyte Games. All Rights Reserved.

#include "Combat/CoralisHitZoneSettings.h"

UCoralisHitZoneSettings::UCoralisHitZoneSettings()
{
	auto AddZone = [this](ECoralisHitZone Zone, float Multiplier, std::initializer_list<const TCHAR*> BoneNames)
	{
		FCoralisHitZone& Entry = Zones.AddDefaulted_GetRef();
		Entry.Zone = Zone;
		Entry.DamageMultiplier = Multiplier;
		for (const TCHAR* Name : BoneNames)
		{
			Entry.Bones.Add(FName(Name));
		}
	};
	AddZone(ECoralisHitZone::Head, 2.f, { TEXT("head"), TEXT("neck_01") });
	AddZone(ECoralisHitZone::Arm, 0.7f, { TEXT("upperarm_l"), TEXT("upperarm_r") });
	AddZone(ECoralisHitZone::Leg, 0.7f, { TEXT("thigh_l"), TEXT("thigh_r") });
}
