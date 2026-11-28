// Copyright Acolyte Games. All Rights Reserved.

#include "Waves/CoralisWaveSettings.h"

#include "AI/CoralisEnemyCharacter.h"

int32 UCoralisWaveSettings::GetEnemyCount(int32 Wave) const
{
	const int32 Index = FMath::Max(0, Wave - 1);
	if (FixedWaveCounts.IsValidIndex(Index))
	{
		return FMath::Clamp(FixedWaveCounts[Index], 1, MaxEnemiesPerWave);
	}
	// The increases form a rising series: increase, increase + growth, increase + 2 growth, ...
	const int64 Count = FirstWaveEnemies + static_cast<int64>(Index) * FirstWaveIncrease
		+ static_cast<int64>(IncreaseGrowth) * Index * (Index - 1) / 2;
	return static_cast<int32>(FMath::Clamp<int64>(Count, 1, MaxEnemiesPerWave));
}
