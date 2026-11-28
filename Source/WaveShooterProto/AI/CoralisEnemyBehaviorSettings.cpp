// Copyright Acolyte Games. All Rights Reserved.

#include "AI/CoralisEnemyBehaviorSettings.h"

UCoralisEnemyBehaviorSettings::UCoralisEnemyBehaviorSettings()
{
	// Distance, chance. Starting values; the behaviour asset holds the ones in use.
	RushChanceByDistance = { { 0.f, 0.f }, { 1200.f, 0.15f }, { 2200.f, 0.5f }, { 3500.f, 0.85f } };
}

float UCoralisEnemyBehaviorSettings::GetRushChance(float Distance) const
{
	// The bands may be listed in any order.
	const FCoralisRushBand* Best = nullptr;
	for (const FCoralisRushBand& Band : RushChanceByDistance)
	{
		if (Band.FromDistance <= Distance && (!Best || Band.FromDistance > Best->FromDistance))
		{
			Best = &Band;
		}
	}
	return Best ? Best->Chance : 0.f;
}
