// Copyright Acolyte Games. All Rights Reserved.

#include "Core/CoralisGameplayTags.h"

namespace CoralisTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Sprinting, "Coralis.Movement.Sprinting", "Roadie run active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Aiming, "Coralis.Movement.Aiming", "Aiming down the shoulder.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Crouching, "Coralis.Movement.Crouching", "Crouched.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Dodging, "Coralis.Movement.Dodging", "Dodge roll in progress.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Recovering, "Coralis.Movement.Recovering", "Recovery window after a dodge.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Falling, "Coralis.Movement.Falling", "Airborne.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_InCover, "Coralis.Movement.InCover", "Attached to cover.");
}
