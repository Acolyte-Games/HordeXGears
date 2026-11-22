// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CoralisCameraSettings.generated.h"

/** One over-the-shoulder camera pose. Y offsets are for the right shoulder and get mirrored on the left. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisCameraProfile
{
	GENERATED_BODY()

	/** Boom length (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", Units = "Centimeters"))
	float ArmLength = 170.f;

	/** Offset at the camera end of the boom (cm). Y = shoulder side, Z = height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector SocketOffset = FVector(0.f, 75.f, 55.f);

	/** Offset of the boom pivot from the character (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector TargetOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "5", ClampMax = "170", Units = "Degrees"))
	float FieldOfView = 85.f;

	/** How fast the camera blends into this profile (1/s, exponential). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float BlendSpeed = 8.f;

	/** Boom position lag speed. 0 = no lag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float PositionLagSpeed = 12.f;

	/** Camera bob amplitude while moving (cm): X = side sway, Y = vertical. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Shake")
	FVector2D BobAmplitude = FVector2D::ZeroVector;

	/** Vertical bob cycles per second at the reference speed (side sway runs at half this). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Shake", meta = (ClampMin = "0", Units = "Hertz"))
	float BobFrequency = 0.f;

	/** Ground speed at which the bob is at full amplitude and BobFrequency (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Shake", meta = (ClampMin = "1", Units = "CentimetersPerSecond"))
	float BobReferenceSpeed = 600.f;

	/** Multiplier on look input in this state (e.g. slower while aiming). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input", meta = (ClampMin = "0"))
	float LookSensitivityScale = 1.f;

	/**
	 * How strongly the view swings toward the direction the character faces (1/s). 0 = free camera.
	 * Used by the roadie run: steering with the stick or keys carries the camera along, while look input still turns it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input", meta = (ClampMin = "0"))
	float HeadingFollowSpeed = 0.f;
};

/** A profile that applies while the character has StateTag. */
USTRUCT(BlueprintType)
struct WAVESHOOTERPROTO_API FCoralisCameraStateProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FGameplayTag StateTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FCoralisCameraProfile Profile;
};

/** Camera rig tuning. State profiles are checked top to bottom; the first whose tag the character has wins. */
UCLASS(BlueprintType)
class WAVESHOOTERPROTO_API UCoralisCameraSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UCoralisCameraSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FCoralisCameraProfile DefaultProfile;

	/** Ordered by priority (e.g. Dodging, Aiming, Sprinting, Crouching, later InCover). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (TitleProperty = "StateTag"))
	TArray<FCoralisCameraStateProfile> StateProfiles;

	/** Heading follow pauses this long after any look input, so the player's own aim always wins (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input", meta = (ClampMin = "0", Units = "Seconds"))
	float HeadingFollowLookPause = 0.5f;

	/** After the pause, heading follow fades back in over this time (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input", meta = (ClampMin = "0", Units = "Seconds"))
	float HeadingFollowResumeTime = 0.4f;

	/** Shoulder swap blend speed (1/s, exponential). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float ShoulderSwapSpeed = 10.f;

	/** Start on the right shoulder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bStartOnRightShoulder = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "-89", ClampMax = "0", Units = "Degrees"))
	float PitchMin = -70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "89", Units = "Degrees"))
	float PitchMax = 60.f;

	/** Camera collision probe radius (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", Units = "Centimeters"))
	float ProbeSize = 12.f;

	/**
	 * Lag speed that stands in for "no lag" (a profile with Position Lag Speed 0) while blending between profiles.
	 * Blending toward a real stiff value instead of 0 avoids the camera hanging back and then popping. Higher = tighter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1"))
	float NoLagBlendSpeed = 40.f;

	/** Profile for a tag set, or DefaultProfile. */
	const FCoralisCameraProfile& FindProfile(const FGameplayTagContainer& StateTags) const;
};
