// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/CoralisCharacterBase.h"
#include "CoralisEnemyCharacter.generated.h"

class UCoralisEnemyBehaviorSettings;

/**
 * Enemy body: the same movement, rifle, health and animation as the players, driven by ACoralisEnemyAIController.
 * Mesh, animation, rifle, movement settings and health are set on the Blueprint child.
 */
UCLASS(Blueprintable)
class WAVESHOOTERPROTO_API ACoralisEnemyCharacter : public ACoralisCharacterBase
{
	GENERATED_BODY()

public:
	ACoralisEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	/** How this enemy fights. Unset = the class defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coralis|AI")
	TObjectPtr<UCoralisEnemyBehaviorSettings> Behavior;

	const UCoralisEnemyBehaviorSettings& GetBehavior() const;

protected:
	virtual void HandleDeath(AController* Killer) override;
};
