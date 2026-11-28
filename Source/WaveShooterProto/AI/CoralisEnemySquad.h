// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class ACoralisEnemyAIController;

/** Enemies fighting together. Shared by its members and gone with the last of them. Server only. */
struct FCoralisEnemySquad
{
	/** The first one still here leads: the others follow its target, position and rushes. */
	TArray<TWeakObjectPtr<ACoralisEnemyAIController>> Members;

	/** Someone is on the way to join; the leader waits for them. */
	bool bGathering = false;

	/** World time at which waiting ends. */
	double GatherEndTime = 0.0;
};
