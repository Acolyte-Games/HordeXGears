// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoralisInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisMappingContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	int32 Priority = 0;
};

/** Input actions, contexts and look tuning for the player character. Key bindings live in the mapping contexts. */
UCLASS(BlueprintType)
class WAVESHOOTERPROTO_API UCoralisInputConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contexts")
	TArray<FCoralisMappingContext> MappingContexts;

	/** Axis2D, X = right, Y = forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> MoveAction;

	/** Axis2D look from a stick (rate based). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> LookAction;

	/** Axis2D look from the mouse (delta based). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> MouseLookAction;

	/** Hold to roadie run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> SprintAction;

	/** Press to dodge roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> DodgeAction;

	/** Gears-style shared button: tap = roll, hold = roadie run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> EvadeRunAction;

	/** Hold to aim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> AimAction;

	/** Press to toggle crouch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> ShoulderSwapAction;

	/** Press to flip mouse look up/down for this session. Stand-in until the options menu exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actions")
	TObjectPtr<UInputAction> InvertMouseLookAction;

	/** Mouse look: degrees per unit of mouse delta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = "0"))
	float MouseDegreesPerUnit = 1.5f;

	/** Stick look at full deflection (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float GamepadYawRate = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = "0", Units = "DegreesPerSecond"))
	float GamepadPitchRate = 140.f;

	/** Response curve exponent on stick look (1 = linear, >1 = finer near centre). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = "1"))
	float GamepadLookExponent = 1.6f;

	/** Mouse: moving the mouse forward looks down instead of up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
	bool bInvertMouseY = false;

	/** Gamepad: pushing the right stick up looks down instead of up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
	bool bInvertGamepadY = false;

	/** EvadeRun: released before this = roll, held past it = roadie run (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeRun", meta = (ClampMin = "0", Units = "Seconds"))
	float EvadeHoldThreshold = 0.2f;

	/** EvadeRun tap only rolls when the move stick is pushed past this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeRun", meta = (ClampMin = "0", ClampMax = "1"))
	float EvadeMinMoveInput = 0.3f;
};
