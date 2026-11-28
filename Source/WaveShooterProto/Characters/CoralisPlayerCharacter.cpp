// Copyright Acolyte Games. All Rights Reserved.

#include "Characters/CoralisPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/CoralisCameraRigComponent.h"
#include "Combat/CoralisHealthComponent.h"
#include "Combat/CoralisWeaponComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/CoralisInputConfig.h"
#include "InputActionValue.h"
#include "Movement/CoralisMovementComponent.h"
#include "Movement/CoralisMovementSettings.h"
#include "TimerManager.h"
#include "UI/CoralisHealthHUDComponent.h"
#include "UI/CoralisWaveHUDComponent.h"
#include "UI/CoralisWeaponHUDComponent.h"

ACoralisPlayerCharacter::ACoralisPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CameraRig = CreateDefaultSubobject<UCoralisCameraRigComponent>(TEXT("CameraRig"));

	WeaponHUD = CreateDefaultSubobject<UCoralisWeaponHUDComponent>(TEXT("WeaponHUD"));
	HealthHUD = CreateDefaultSubobject<UCoralisHealthHUDComponent>(TEXT("HealthHUD"));
	WaveHUD = CreateDefaultSubobject<UCoralisWaveHUDComponent>(TEXT("WaveHUD"));

	// Players recover out of combat. Starting values; edit them on the Blueprint's Health component.
	Health->RegenRate = 20.f;
	Health->RegenDelay = 5.f;
}

void ACoralisPlayerCharacter::HandleDeath(AController* Killer)
{
	if (RespawnDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::Respawn, RespawnDelay, false);
	}
}

void ACoralisPlayerCharacter::CancelPendingRespawn()
{
	GetWorldTimerManager().ClearTimer(RespawnTimer);
}

void ACoralisPlayerCharacter::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}
	CancelPendingRespawn();
	UWorld* World = GetWorld();
	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	AController* OwningController = GetController();
	if (!GameMode || !OwningController)
	{
		return;
	}
	OwningController->UnPossess();
	GameMode->RestartPlayer(OwningController);
	Destroy();
}

void ACoralisPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	bMouseYInverted = InputConfig && InputConfig->bInvertMouseY;
	AddMappingContexts();
	if (CameraRig)
	{
		CameraRig->ApplyViewLimits();
	}
}

void ACoralisPlayerCharacter::AddMappingContexts()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController() || !InputConfig)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		for (const FCoralisMappingContext& Entry : InputConfig->MappingContexts)
		{
			if (Entry.Context && !Subsystem->HasMappingContext(Entry.Context))
			{
				Subsystem->AddMappingContext(Entry.Context, Entry.Priority);
			}
		}
	}
}

void ACoralisPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input || !InputConfig)
	{
		return;
	}
	const UCoralisInputConfig* C = InputConfig;
	if (C->MoveAction)
	{
		Input->BindAction(C->MoveAction, ETriggerEvent::Triggered, this, &ThisClass::OnMove);
		Input->BindAction(C->MoveAction, ETriggerEvent::Completed, this, &ThisClass::OnMove);
	}
	if (C->LookAction) { Input->BindAction(C->LookAction, ETriggerEvent::Triggered, this, &ThisClass::OnLook); }
	if (C->MouseLookAction) { Input->BindAction(C->MouseLookAction, ETriggerEvent::Triggered, this, &ThisClass::OnMouseLook); }
	if (C->SprintAction)
	{
		Input->BindAction(C->SprintAction, ETriggerEvent::Started, this, &ThisClass::OnSprintStarted);
		Input->BindAction(C->SprintAction, ETriggerEvent::Completed, this, &ThisClass::OnSprintCompleted);
	}
	if (C->DodgeAction) { Input->BindAction(C->DodgeAction, ETriggerEvent::Started, this, &ThisClass::OnDodge); }
	if (C->EvadeRunAction)
	{
		Input->BindAction(C->EvadeRunAction, ETriggerEvent::Started, this, &ThisClass::OnEvadeRunStarted);
		Input->BindAction(C->EvadeRunAction, ETriggerEvent::Completed, this, &ThisClass::OnEvadeRunCompleted);
	}
	if (C->AimAction)
	{
		Input->BindAction(C->AimAction, ETriggerEvent::Started, this, &ThisClass::OnAimStarted);
		Input->BindAction(C->AimAction, ETriggerEvent::Completed, this, &ThisClass::OnAimCompleted);
	}
	if (C->FireAction)
	{
		Input->BindAction(C->FireAction, ETriggerEvent::Started, this, &ThisClass::OnFireStarted);
		Input->BindAction(C->FireAction, ETriggerEvent::Completed, this, &ThisClass::OnFireCompleted);
		Input->BindAction(C->FireAction, ETriggerEvent::Canceled, this, &ThisClass::OnFireCompleted);
	}
	if (C->VentAction) { Input->BindAction(C->VentAction, ETriggerEvent::Started, this, &ThisClass::OnVent); }
	if (C->CrouchAction) { Input->BindAction(C->CrouchAction, ETriggerEvent::Started, this, &ThisClass::OnCrouch); }
	if (C->ShoulderSwapAction) { Input->BindAction(C->ShoulderSwapAction, ETriggerEvent::Started, this, &ThisClass::OnShoulderSwap); }
	if (C->InvertMouseLookAction) { Input->BindAction(C->InvertMouseLookAction, ETriggerEvent::Started, this, &ThisClass::OnInvertMouseLook); }
}

void ACoralisPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CameraRig && IsLocallyControlled())
	{
		CameraRig->ApplyHeadingFollow(DeltaSeconds);
		if (bSwitchHandsWithShoulder)
		{
			SetLeftHanded(!CameraRig->IsOnRightShoulder());
		}
	}

#if !UE_BUILD_SHIPPING
	if (!DebugMoveInput.IsNearlyZero() && IsLocallyControlled())
	{
		ApplyMoveInput(DebugMoveInput);
	}
#endif

	// Hold on the shared button turns into a roadie run once past the threshold.
	if (InputConfig && EvadePressTime >= 0.0 && !bEvadeHeldIntoSprint && GetWorld())
	{
		if (GetWorld()->GetTimeSeconds() - EvadePressTime >= InputConfig->EvadeHoldThreshold)
		{
			bEvadeHeldIntoSprint = true;
			SetSprinting(true);
		}
	}
}

void ACoralisPlayerCharacter::SetDebugMoveInput(FVector2D Input)
{
#if !UE_BUILD_SHIPPING
	DebugMoveInput = Input;
	LastMoveInput = Input;
#endif
}

void ACoralisPlayerCharacter::ApplyMoveInput(const FVector2D& Input)
{
	if (!Controller)
	{
		return;
	}
	const UCoralisMovementComponent* Move = GetCoralisMovement();
	// Angle of the stick from straight forward: 0 = forward, 90 = pure sideways, 180 = straight back.
	const float StickAngle = FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(Input.X, Input.Y)));
	if (Move && Move->IsSprinting() && StickAngle > Move->GetSettings()->SprintLiveSteerAngle)
	{
		if (!bSprintInputFrameLatched)
		{
			SprintInputFrameYaw = Controller->GetControlRotation().Yaw;
			bSprintInputFrameLatched = true;
		}
	}
	else
	{
		bSprintInputFrameLatched = false;
	}
	const float FrameYaw = bSprintInputFrameLatched ? SprintInputFrameYaw : Controller->GetControlRotation().Yaw;
	const FRotator YawRotation(0.f, FrameYaw, 0.f);
	const FRotationMatrix YawMatrix(YawRotation);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Input.X);
}

void ACoralisPlayerCharacter::OnMove(const FInputActionValue& Value)
{
	LastMoveInput = Value.Get<FVector2D>();
	ApplyMoveInput(LastMoveInput);
}

void ACoralisPlayerCharacter::AddLook(float YawDegrees, float PitchDegrees)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || PC->IsLookInputIgnored())
	{
		return;
	}
	const float Scale = CameraRig ? CameraRig->GetLookSensitivityScale() : 1.f;
	// Written as degrees straight into the controller's rotation input (positive pitch = look up), so the legacy
	// input scales don't apply; the camera manager still clamps pitch.
	if (CameraRig && !FMath::IsNearlyZero(YawDegrees))
	{
		CameraRig->NotifyLookInput();
	}
	PC->RotationInput.Yaw += YawDegrees * Scale;
	if (bSprintInputFrameLatched)
	{
		SprintInputFrameYaw = FRotator::NormalizeAxis(SprintInputFrameYaw + YawDegrees * Scale);
	}
	PC->RotationInput.Pitch += PitchDegrees * Scale;
}

void ACoralisPlayerCharacter::OnLook(const FInputActionValue& Value)
{
	if (!InputConfig || !GetWorld())
	{
		return;
	}
	const FVector2D V = Value.Get<FVector2D>();
	const float Dt = GetWorld()->GetDeltaSeconds();
	const float Exp = InputConfig->GamepadLookExponent;
	const float X = FMath::Sign(V.X) * FMath::Pow(FMath::Abs(V.X), Exp);
	const float Y = FMath::Sign(V.Y) * FMath::Pow(FMath::Abs(V.Y), Exp);
	// The stick's raw Y has the opposite sign to the mouse's for "up" (the template negates only the mouse), so flip it here.
	const float PitchSign = InputConfig->bInvertGamepadY ? 1.f : -1.f;
	AddLook(X * InputConfig->GamepadYawRate * Dt, Y * InputConfig->GamepadPitchRate * Dt * PitchSign);
}

void ACoralisPlayerCharacter::OnMouseLook(const FInputActionValue& Value)
{
	if (!InputConfig)
	{
		return;
	}
	const FVector2D V = Value.Get<FVector2D>();
	const float PitchSign = bMouseYInverted ? -1.f : 1.f;
	AddLook(V.X * InputConfig->MouseDegreesPerUnit, V.Y * InputConfig->MouseDegreesPerUnit * PitchSign);
}

void ACoralisPlayerCharacter::OnSprintStarted(const FInputActionValue& Value)
{
	bSprintHeld = true;
	SetSprinting(true);
}

void ACoralisPlayerCharacter::OnSprintCompleted(const FInputActionValue& Value)
{
	bSprintHeld = false;
	if (!bEvadeHeldIntoSprint)
	{
		SetSprinting(false);
	}
}

void ACoralisPlayerCharacter::OnDodge(const FInputActionValue& Value)
{
	Dodge();
}

void ACoralisPlayerCharacter::OnEvadeRunStarted(const FInputActionValue& Value)
{
	EvadePressTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	bEvadeHeldIntoSprint = false;
}

void ACoralisPlayerCharacter::OnEvadeRunCompleted(const FInputActionValue& Value)
{
	if (bEvadeHeldIntoSprint)
	{
		if (!bSprintHeld)
		{
			SetSprinting(false);
		}
	}
	else if (InputConfig && LastMoveInput.Size() >= InputConfig->EvadeMinMoveInput)
	{
		Dodge();
	}
	EvadePressTime = -1.0;
	bEvadeHeldIntoSprint = false;
}

void ACoralisPlayerCharacter::OnAimStarted(const FInputActionValue& Value)
{
	SetAiming(true);
}

void ACoralisPlayerCharacter::OnAimCompleted(const FInputActionValue& Value)
{
	SetAiming(false);
}

void ACoralisPlayerCharacter::OnFireStarted(const FInputActionValue& Value)
{
	GetWeapon()->StartFire();
}

void ACoralisPlayerCharacter::OnFireCompleted(const FInputActionValue& Value)
{
	GetWeapon()->StopFire();
}

void ACoralisPlayerCharacter::OnVent(const FInputActionValue& Value)
{
	GetWeapon()->Vent();
}

void ACoralisPlayerCharacter::OnCrouch(const FInputActionValue& Value)
{
	ToggleCrouch();
}

void ACoralisPlayerCharacter::OnInvertMouseLook(const FInputActionValue& Value)
{
	bMouseYInverted = !bMouseYInverted;
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Cyan, bMouseYInverted ? TEXT("Mouse look: inverted") : TEXT("Mouse look: normal"));
	}
#endif
}

void ACoralisPlayerCharacter::OnShoulderSwap(const FInputActionValue& Value)
{
	if (CameraRig)
	{
		CameraRig->ToggleShoulder();
	}
}
