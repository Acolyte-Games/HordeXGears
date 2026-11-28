// Copyright Acolyte Games. All Rights Reserved.

#include "Camera/CoralisCameraSettings.h"

#include "Core/CoralisGameplayTags.h"

UCoralisCameraSettings::UCoralisCameraSettings()
{
	// Placeholder starting points; the data asset is the place to tune.
	FCoralisCameraStateProfile Aim;
	Aim.StateTag = CoralisTags::Movement_Aiming;
	Aim.Profile.ArmLength = 120.f;
	Aim.Profile.SocketOffset = FVector(0.f, 70.f, 60.f);
	Aim.Profile.FieldOfView = 60.f;
	Aim.Profile.BlendSpeed = 14.f;
	Aim.Profile.PositionLagSpeed = 0.f;
	Aim.Profile.LookSensitivityScale = 0.55f;

	FCoralisCameraStateProfile Sprint;
	Sprint.StateTag = CoralisTags::Movement_Sprinting;
	Sprint.Profile.ArmLength = 200.f;
	Sprint.Profile.SocketOffset = FVector(0.f, 75.f, 50.f);
	Sprint.Profile.FieldOfView = 90.f;
	Sprint.Profile.BlendSpeed = 2.5f;
	Sprint.Profile.PositionLagSpeed = 8.f;
	Sprint.Profile.BobAmplitude = FVector2D(2.5f, 3.5f);
	Sprint.Profile.BobFrequency = 2.6f;
	Sprint.Profile.LookSensitivityScale = 0.5f;
	Sprint.Profile.HeadingFollowSpeed = 5.f;

	FCoralisCameraStateProfile Crouch;
	Crouch.StateTag = CoralisTags::Movement_Crouching;
	Crouch.Profile.ArmLength = 160.f;
	Crouch.Profile.SocketOffset = FVector(0.f, 75.f, 35.f);

	StateProfiles = { Aim, Sprint, Crouch };
}

const FCoralisCameraProfile& UCoralisCameraSettings::FindProfile(const FGameplayTagContainer& StateTags) const
{
	for (const FCoralisCameraStateProfile& Entry : StateProfiles)
	{
		if (Entry.StateTag.IsValid() && StateTags.HasTag(Entry.StateTag))
		{
			return Entry.Profile;
		}
	}
	return DefaultProfile;
}
