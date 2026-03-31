// Copyright CrystalVapor 2026, All rights reserved.

#include "Components/CRRecoilComponent.h"
#include "Data/CRRecoilPattern.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

UCRRecoilComponent::UCRRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UCRRecoilComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UWorld* World = GetWorld();
	AController* Controller = GetTargetController();

	if (!World || !Controller || !RecoilPattern)
	{
		SetComponentTickEnabled(false);
		return;
	}

	FRotator DeltaRecoilRotation = FRotator::ZeroRotator;
	FRotator DeltaRecoveryRotation = FRotator::ZeroRotator;
	const FRotator CurrentRotation = Controller->GetControlRotation();

	const FRotator RotationDelta = (CurrentRotation - CachedControllerRotation).GetNormalized();
	FRotator InputLastFrame = RotationDelta - RecoilInputGeneratedLastFrame;
	InputLastFrame.Normalize();

	CachedControllerRotation = Controller->GetControlRotation();

	// Apply recoil uplift
	if (!RecoilToApply.IsNearlyZero())
	{
		CurrentRecoilSpeed = FMath::Max(0.f, CurrentRecoilSpeed - CurrentUpliftDeceleration * DeltaTime);

		const float DeltaMove = CurrentRecoilSpeed * DeltaTime;
		const float RemainingMagnitude = FMath::Sqrt(RecoilToApply.Pitch * RecoilToApply.Pitch + RecoilToApply.Yaw * RecoilToApply.Yaw);

		if (DeltaMove >= RemainingMagnitude || FMath::IsNearlyZero(CurrentRecoilSpeed))
		{
			DeltaRecoilRotation = RecoilToApply;
		}
		else
		{
			const float Alpha = DeltaMove / RemainingMagnitude;
			DeltaRecoilRotation = FRotator(RecoilToApply.Pitch * Alpha, RecoilToApply.Yaw * Alpha, 0.f);
		}

		if (ProcessDeltaRecoilRotation(DeltaRecoilRotation))
		{
			ApplyInputToController(Controller, DeltaRecoilRotation);
			RecoilToApply -= DeltaRecoilRotation;
			RecoilToRecover += DeltaRecoilRotation;
		}
	}

	// Always try to compensate if player is pulling against accumulated recoil
	if (!RecoilToRecover.IsNearlyZero(0.001))
	{
		ReduceRecoveryByPlayerInput(InputLastFrame);
	}

	// Accumulate player input during RecoveryDelay wait, but not during uplift
	if (bTrackingInputDuringFire && RecoilToApply.IsNearlyZero() && LastFireTime + RecoilPattern->RecoveryDelay >= World->GetTimeSeconds())
	{
		AccumulatedInputDuringFire.Pitch += InputLastFrame.Pitch;
		AccumulatedInputDuringFire.Yaw += InputLastFrame.Yaw;
	}

	// Apply recoil recovery - only after uplift is fully complete
	if (RecoilPattern->RecoveryDelay >= 0.f && RecoilToApply.IsNearlyZero() && !RecoilToRecover.IsNearlyZero(0.001))
	{
		if (LastFireTime + RecoilPattern->RecoveryDelay < World->GetTimeSeconds())
		{
			// Cancel recovery if player made large aiming movements during burst
			if (bTrackingInputDuringFire && RecoilPattern->RecoveryCancelThreshold > 0.f)
			{
				bTrackingInputDuringFire = false; // Stop tracking once we check
				const bool bPlayerAimedAway = FMath::Abs(AccumulatedInputDuringFire.Pitch) > RecoilPattern->RecoveryCancelThreshold || FMath::Abs(AccumulatedInputDuringFire.Yaw) > RecoilPattern->RecoveryCancelThreshold;

				if (bPlayerAimedAway)
				{
					// Player took manual control - cancel and reset recovery
					RecoilToRecover = FRotator::ZeroRotator;
					SetComponentTickEnabled(false);
					return;
				}
			}

			CurrentRecoverySpeed = FMath::FInterpConstantTo(CurrentRecoverySpeed, RecoilPattern->MaxRecoverySpeed, DeltaTime, RecoilPattern->RecoveryAcceleration);
			DeltaRecoveryRotation = FMath::RInterpTo(FRotator::ZeroRotator, RecoilToRecover, DeltaTime, CurrentRecoverySpeed);
			DeltaRecoveryRotation = FRotator(-DeltaRecoveryRotation.Pitch, -DeltaRecoveryRotation.Yaw, 0.f);

			if (ProcessDeltaRecoveryRotation(DeltaRecoveryRotation))
			{
				ApplyInputToController(Controller, DeltaRecoveryRotation);
				RecoilToRecover += DeltaRecoveryRotation;
			}

			if (RecoilToRecover.IsNearlyZero(0.001))
			{
				RecoilToRecover = FRotator::ZeroRotator;
				SetComponentTickEnabled(false);
			}
		}
	}
	else if (RecoilToApply.IsNearlyZero() && RecoilToRecover.IsNearlyZero())
	{
		// Nothing to process - disable tick only if we're past the recovery delay window
		if (World->GetTimeSeconds() > LastFireTime + RecoilPattern->RecoveryDelay)
		{
			SetComponentTickEnabled(false);
		}
	}

	// Negate pitch because ApplyInputToController does Pitch -= Input.Pitch (inverted), but Yaw is additive (Yaw += Input.Yaw), so it keeps its sign.
	// Without this, the sign mismatch causes InputLastFrame to see double the recoil as phantom player input, which incorrectly triggers compensation
	RecoilInputGeneratedLastFrame = FRotator(-DeltaRecoilRotation.Pitch - DeltaRecoveryRotation.Pitch, DeltaRecoilRotation.Yaw + DeltaRecoveryRotation.Yaw, 0.f);
}

void UCRRecoilComponent::ApplyShot()
{
	const AController* Controller = GetTargetController();
	if (!Controller || !Controller->IsLocalPlayerController() || !RecoilPattern)
	{
		return;
	}

	const FVector2f RecoilPositionDelta = RecoilPattern->ConsumeShot(CurrentShotIndex) * RecoilStrength;
	const float RecoilDeltaLength = RecoilPositionDelta.Size();

	// Map UpliftSpeed (0-1) to duration: high sharpness = short = snappy
	// Lerp in speed space (1/T) instead of time space so sharpness feels linear
	// At 0.0: 1/2 = 0.5s (floaty)
	// At 0.75: ~20ms (fast)
	// At 1.0: 1/66 = 0.015s (instant)
	constexpr float MinRate = 1.f / 0.5f;
	constexpr float MaxRate = 1.f / 0.025f;
	const float UpliftDuration = 1.f / FMath::Lerp(MinRate, MaxRate, RecoilPattern->UpliftSpeed);

	// Kinematics: v0 = 2d/T, a = 2d/T^2
	// Guarantees camera travels exactly DeltaRecoilLength in exactly UpliftDuration
	CurrentUpliftDeceleration = (2.f * RecoilDeltaLength) / (UpliftDuration * UpliftDuration);
	CurrentRecoilSpeed = 2.f * RecoilDeltaLength / UpliftDuration;

	RecoilToApply = FRotator(-RecoilPositionDelta.Y, RecoilPositionDelta.X, 0.0);
	CurrentRecoverySpeed = RecoilPattern->InitialRecoverySpeed;
	LastFireTime = GetWorld()->GetTimeSeconds();
}

void UCRRecoilComponent::ReduceRecoveryByPlayerInput(const FRotator& LastFrameInput)
{
	// Only compensate if Player is actively countering recoil (threshold to avoid noise and normal aiming)
	constexpr double InputThreshold = 0.01;

	// Reduce recovery by player input amount when opposing, but don't go past zero
	auto CompensateAxis = [InputThreshold](double& RecoverAxis, const double InputAxis)
	{
		// Player is pulling in the same direction as recovery needs to go
		if (FMath::Abs(InputAxis) > InputThreshold && FMath::Sign(InputAxis) == FMath::Sign(RecoverAxis))
		{
			const double NewRecovery = RecoverAxis - InputAxis;
			RecoverAxis = FMath::Sign(NewRecovery) == FMath::Sign(RecoverAxis) ? NewRecovery : 0.0;
		}
	};

	CompensateAxis(RecoilToRecover.Pitch, LastFrameInput.Pitch); // Vertical
	CompensateAxis(RecoilToRecover.Yaw, LastFrameInput.Yaw);     // Horizontal
}

void UCRRecoilComponent::SetTargetController(AController* InController)
{
	TargetController = InController;
}

void UCRRecoilComponent::ApplyInputToController(AController* InTargetController, const FRotator& Input)
{
	if (!InTargetController)
	{
		return;
	}

	FRotator CurrentRotation = InTargetController->GetControlRotation();

	// Apply the recoil delta
	CurrentRotation.Pitch -= Input.Pitch;
	CurrentRotation.Yaw += Input.Yaw;

	// Clamp pitch before normalizing to prevent gimbal lock
	CurrentRotation.Pitch = FMath::ClampAngle(CurrentRotation.Pitch, -89.9f, 89.9f);

	// Normalize yaw to keep it in -180 to 180 range
	CurrentRotation.Normalize();

	InTargetController->SetControlRotation(CurrentRotation);
}

void UCRRecoilComponent::StartShooting()
{
	const AController* Controller = GetTargetController();
	if (!Controller || !Controller->IsLocalPlayerController())
	{
		return;
	}

	CurrentShotIndex = 0;
	AccumulatedInputDuringFire = FRotator::ZeroRotator;

	if (RecoilPattern)
	{
		bTrackingInputDuringFire = RecoilPattern->RecoveryDelay > 0.f && RecoilPattern->RecoveryCancelThreshold > 0.f;
		SetComponentTickEnabled(true);
	}
}

void UCRRecoilComponent::SetRecoilPattern(UCRRecoilPattern* InRecoilPattern)
{
	if (InRecoilPattern)
	{
		RecoilPattern = InRecoilPattern;
	}
}

void UCRRecoilComponent::SetRecoilStrength(const float InRecoilStrength)
{
	RecoilStrength = FMath::Max(0.f, InRecoilStrength);
}

float UCRRecoilComponent::GetRecoilStrength() const
{
	return RecoilStrength;
}

AController* UCRRecoilComponent::GetTargetController() const
{
	if (!TargetController.IsValid())
	{
		TargetController = GetWorld()->GetFirstPlayerController();
	}
	return TargetController.Get();
}

bool UCRRecoilComponent::ProcessDeltaRecoilRotation(FRotator& DeltaRecoilRotation)
{
	// Example: Clamp pitch to prevent recoil pushing past a maximum angle
	//
	// constexpr float MaxPitch = -60.f; // Negative because upward pitch is negative in UE
	//
	// const AController* TargetController = GetTargetController();
	// if (!TargetController)
	// {
	// 	return true;
	// }
	//
	// const float CurrentPitch = TargetController->GetControlRotation().Pitch;
	// const float NewPitch = CurrentPitch + DeltaRecoilRotation.Pitch;
	//
	// if (NewPitch < MaxPitch)
	// {
	// 	// Clamp the delta so we don't exceed the max pitch
	// 	DeltaRecoilRotation.Pitch = MaxPitch - CurrentPitch;
	// }

	return true;
}

bool UCRRecoilComponent::ProcessDeltaRecoveryRotation(FRotator& DeltaRecoveryRotation)
{
	// Example: Block recovery entirely during cinematics
	// return !bIsPlayingCinematic;

	return true;
}
