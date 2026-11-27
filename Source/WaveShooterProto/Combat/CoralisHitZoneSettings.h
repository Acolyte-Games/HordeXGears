// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "CoralisHitZoneSettings.generated.h"

UENUM(BlueprintType)
enum class ECoralisHitZone : uint8
{
	Body,
	Head,
	Arm,
	Leg
};

USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisHitZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone")
	ECoralisHitZone Zone = ECoralisHitZone::Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	/** A hit on one of these bones, or on any bone below it, belongs to this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone")
	TArray<FName> Bones;
};

/** Hit zones of one body type. Bones not listed here (or below a listed one) count as the body. */
UCLASS(BlueprintType)
class WAVESHOOTERPROTO_API UCoralisHitZoneSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Defaults fit the engine mannequin skeleton. */
	UCoralisHitZoneSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zones")
	TArray<FCoralisHitZone> Zones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zones", meta = (ClampMin = "0"))
	float BodyDamageMultiplier = 1.f;

	/** The trace channel that should hit the mesh bodies instead of the capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zones")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel1;
};
