// Copyright Acolyte Games. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/** Code-owned state tags. Systems publish these; camera, animation and UI read them. */
namespace CoralisTags
{
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Sprinting);
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Aiming);
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Crouching);
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Dodging);
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Recovering);
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Falling);
	/** Reserved for the cover system (camera profile and anim state already key off it). */
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_InCover);
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Venting);
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Jammed);
	WAVESHOOTERPROTO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Buffed);
}
