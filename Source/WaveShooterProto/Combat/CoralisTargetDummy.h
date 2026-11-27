// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CoralisTargetDummy.generated.h"

class UCoralisHealthComponent;
class UTextRenderComponent;

/**
 * Standing target for testing weapons: a body with health and hit zones and a readout of the last hit over its head.
 * Refills its health a moment after running out. Give it a skeletal mesh with a physics asset in a Blueprint.
 */
UCLASS(Blueprintable)
class WAVESHOOTERPROTO_API ACoralisTargetDummy : public ACharacter
{
	GENERATED_BODY()

public:
	ACoralisTargetDummy();

	UFUNCTION(BlueprintPure, Category = "Coralis|Health")
	UCoralisHealthComponent* GetHealthComponent() const { return Health; }

	/** Time at zero health before it refills. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coralis|Test", meta = (ClampMin = "0.1", Units = "Seconds"))
	float ResetDelay = 2.f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Health")
	TObjectPtr<UCoralisHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coralis|Test")
	TObjectPtr<UTextRenderComponent> Readout;

private:
	UFUNCTION()
	void HandleDepleted(AController* InstigatorController);

	void Refill();
};
