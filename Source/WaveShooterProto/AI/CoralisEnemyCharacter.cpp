// Copyright Acolyte Games. All Rights Reserved.

#include "AI/CoralisEnemyCharacter.h"

#include "AI/CoralisEnemyAIController.h"
#include "AI/CoralisEnemyBehaviorSettings.h"

ACoralisEnemyCharacter::ACoralisEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ACoralisEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	CorpseLifeSpan = 5.f;
}

const UCoralisEnemyBehaviorSettings& ACoralisEnemyCharacter::GetBehavior() const
{
	return Behavior ? *Behavior : *GetDefault<UCoralisEnemyBehaviorSettings>();
}

void ACoralisEnemyCharacter::HandleDeath(AController* Killer)
{
	DetachFromControllerPendingDestroy();
}
