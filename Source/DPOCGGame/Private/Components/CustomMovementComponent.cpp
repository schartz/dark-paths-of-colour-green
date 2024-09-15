// Fill out your copyright notice in the Description page of Project Settings.


#include "DPOCGGame/Public/Components/CustomMovementComponent.h"

#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "DPOCGGame/DebugHelper.h"
#include "DPOCGGame/DPOCGGameCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DPOCGGame/DPOCGGameCharacter.h"

// Sets default values for this component's properties
UCustomMovementComponent::UCustomMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCustomMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	OwningPlayerInstance = CharacterOwner->GetMesh()->GetAnimInstance();
	OwningPlayerCharacter = Cast<ADPOCGGameCharacter>(CharacterOwner);
	if (OwningPlayerInstance)
	{
		OwningPlayerInstance->OnMontageEnded.AddDynamic(this, &UCustomMovementComponent::OnCLimbMontageEnded);
		OwningPlayerInstance->OnMontageBlendingOut.AddDynamic(this, &UCustomMovementComponent::OnCLimbMontageEnded);
	}
}

// Called every frame
void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// CheckCanClimbDownLedge();
	// TraceClimbableSurfaces();
	// TraceFromEyeHeight(100.f);
}

/**
 * We override this function here to handle transitions from other movement modes into climbing mode.
 * As well as the inverse case.
 * @param PreviousMovementMode 
 * @param PreviousCustomMode 
 */
void UCustomMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(43.f);
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
	{
		bOrientRotationToMovement = true;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(86.f);

		const FRotator oldRotation = UpdatedComponent->GetComponentRotation();
		const FRotator newRotation = FRotator(0.f, oldRotation.Yaw, 0.f);
		UpdatedComponent->SetRelativeRotation(newRotation);

		StopMovementImmediately();
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UCustomMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (IsClimbing())
	{
		PhysClimb(deltaTime, Iterations);
	}
	Super::PhysCustom(deltaTime, Iterations);
}

float UCustomMovementComponent::GetMaxAcceleration() const
{
	if (IsClimbing())
	{
		return MaxClimbAcceleration;
	}
	return Super::GetMaxAcceleration();
}

float UCustomMovementComponent::GetMaxSpeed() const
{
	if (IsClimbing())
	{
		return MaxClimbSpeed;
	}
	return Super::GetMaxSpeed();
}

/**
 * This is the overriden version of `ConstrainAnimRootMotionVelocity` from original `UCharacterMovementComponent`
 * We override this function here to make sure that `IF` we are playing any animation montage
 * then the default function should NOT handle the root motion velocity.
 * in the `ELSE` case we let the parent version of this function
 * take over.
 * @param RootMotionVelocity 
 * @param CurrentVelocity 
 * @return 
 */
FVector UCustomMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity,
                                                                  const FVector& CurrentVelocity) const
{
	const bool bIsPlayingRMMontage = IsFalling() && OwningPlayerInstance && OwningPlayerInstance->IsAnyMontagePlaying();
	if (bIsPlayingRMMontage)
	{
		return RootMotionVelocity;
	}
	return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
}

#pragma region ClimbTraces
FVector UCustomMovementComponent::GetUnrotatedClimbVelocity() const
{
	return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
}

TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End,
                                                                         bool bShowDebugShape,
                                                                         bool bDrawPersistentTraces)
{
	TArray<FHitResult> outCapsuleTraceHitResults;
	EDrawDebugTrace::Type debugTraceType = EDrawDebugTrace::None;
	if (bShowDebugShape) debugTraceType = EDrawDebugTrace::ForOneFrame;
	if (bShowDebugShape && bDrawPersistentTraces) debugTraceType = EDrawDebugTrace::Persistent;

	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this,
		Start,
		End,
		ClimbCapsuleTraceRadius,
		ClimbCapsuleTraceHalfHeight,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		debugTraceType,
		outCapsuleTraceHitResults,
		false
	);
	return outCapsuleTraceHitResults;
}


FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, const FVector& End,
                                                               bool bShowDebugShape, bool bDrawPersistentTraces)
{
	FHitResult hitResult;
	EDrawDebugTrace::Type debugTraceType = EDrawDebugTrace::None;
	if (bShowDebugShape) debugTraceType = EDrawDebugTrace::ForOneFrame;
	if (bShowDebugShape && bDrawPersistentTraces) debugTraceType = EDrawDebugTrace::Persistent;


	UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		debugTraceType,
		hitResult,
		false);

	return hitResult;
}
#pragma endregion

#pragma region ClimbCore

void UCustomMovementComponent::ToggleClimbing(bool bEnableClimbing)
{
	if (bEnableClimbing)
	{
		if (CanStartClimbing())
		{
			PlayClimbMontage(IdleToClimbMontage);
		}
		else if (CheckCanClimbDownLedge())
		{
			PlayClimbMontage(ClimbDownLedgeMontage);
		}
		else
		{
			TryStartVaulting();
		}
	}
	else
	{
		// stop climbing, monkey
		StopClimbing();
	}
}

bool UCustomMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
}

bool UCustomMovementComponent::CanStartClimbing()
{
	if (IsFalling()) return false;
	if (!TraceClimbableSurfaces()) return false;
	if (!TraceFromEyeHeight(100.f).bBlockingHit) return false;
	return true;
}

inline void UCustomMovementComponent::StartClimbing()
{
	SetMovementMode(MOVE_Custom, ECustomMovementMode::MOVE_Climb);
}

bool UCustomMovementComponent::CheckCanClimbDownLedge()
{
	if (IsFalling()) return false;
	const FVector componentLocation = UpdatedComponent->GetComponentLocation();
	const FVector forward = UpdatedComponent->GetForwardVector();
	const FVector down = -UpdatedComponent->GetUpVector();

	const FVector walkableSurfaceTraceStart = componentLocation + forward * ClimbDownSurfaceTraceOffset;
	const FVector walkableSurfaceTraceEnd = walkableSurfaceTraceStart + down * 100.f;

	FHitResult walkableSurfaceHit = DoLineTraceSingleByObject(walkableSurfaceTraceStart, walkableSurfaceTraceEnd, true);


	FVector ledgeTraceStart = walkableSurfaceHit.TraceStart + forward * ClimbDownLedgeTraceOffset;
	FVector ledgeTraceEnd = ledgeTraceStart + down * 300.f;

	FHitResult ledgeTraceHit = DoLineTraceSingleByObject(ledgeTraceStart, ledgeTraceEnd, true);
	if (walkableSurfaceHit.bBlockingHit && !ledgeTraceHit.bBlockingHit)
	{
		return true;
	}

	return false;
}

inline void UCustomMovementComponent::StopClimbing()
{
	SetMovementMode(MOVE_Falling);
}

void UCustomMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	/*Process climbable surfaces information*/
	TraceClimbableSurfaces();
	ProcessClimbableSurfaceInformation();

	/*Check if we should stop climbing*/
	if (CheckShouldStopClimbing() || CheckHasReachedFloor())
	{
		StopClimbing();
	}

	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		// define the max climbing speed and accelration
		CalcVelocity(deltaTime, 0.f, true, MaxBreakCLimbDeceleration);
	}

	ApplyRootMotionToVelocity(deltaTime);

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);

	// Handle the climbing rotation
	SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		//adjust and try again
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	/*Snap movement to climbable surfaces*/
	SnapMovementToClimbableSurfaces(deltaTime);

	if (ShouldClimbLedge())
	{
		PlayClimbMontage(ClimbToTopMontage);
	}
}

bool UCustomMovementComponent::CheckShouldStopClimbing()
{
	if (ClimbableSurfacesTracedResults.IsEmpty()) return true;

	const float dotResult = FVector::DotProduct(CurrentClimbableSurfaceNormal, FVector::UpVector);
	const float degreeDiff = FMath::RadiansToDegrees(FMath::Acos(dotResult));
	//Debug::Print(TEXT("Degree difference: ") + FString::SanitizeFloat(degreeDiff), FColor::Cyan, 1);
	if (degreeDiff <= 60.f) return true;
	return false;
}

bool UCustomMovementComponent::CheckHasReachedFloor()
{
	const FVector DownVector = -UpdatedComponent->GetUpVector();
	const FVector StartOffset = DownVector * 50.f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + DownVector;

	TArray<FHitResult> PossibleFloorHits = DoCapsuleTraceMultiByObject(Start, End, false);
	if (PossibleFloorHits.IsEmpty())
	{
		return false;
	}

	for (const FHitResult& PossibleFloorHit : PossibleFloorHits)
	{
		const bool bFloorReached = FVector::Parallel(-PossibleFloorHit.ImpactNormal, FVector::UpVector) &&
			GetUnrotatedClimbVelocity().Z < -10.f;
		if (bFloorReached)
		{
			return true;
		}
	}

	return false;
}

bool UCustomMovementComponent::CheckHasReachedLedge()
{
	const FHitResult eyeTraceHit = TraceFromEyeHeight(100.f, ClimbUpEyeTraceOffset);
	if (!eyeTraceHit.bBlockingHit)
	{
		const FVector Start = eyeTraceHit.TraceEnd;
		const FVector End = Start + -UpdatedComponent->GetUpVector() * 100.f;
		const FHitResult LedgeSurfaceHit = DoLineTraceSingleByObject(Start, End, false);
		if (LedgeSurfaceHit.bBlockingHit)
		{
			return true;
		}
	}
	return false;
}

void UCustomMovementComponent::TryStartVaulting()
{
	FVector vaultStart;
	FVector vaultEnd;
	if (CanStartVaulting(vaultStart, vaultEnd))
	{
		Debug::Print(TEXT("fuck it, we vault. From: ") +
			vaultStart.ToCompactString() +
			TEXT(" to: ") + vaultEnd.ToCompactString(),
			FColor::Green);
		SetMotionWarpTarget(FName("VaultStartPoint"), vaultStart);
		SetMotionWarpTarget(FName("VaultEndPoint"), vaultEnd);
		StartClimbing();
		PlayClimbMontage(VaultMontage);
	}
	else
	{
		Debug::Print(TEXT("Can't climb down ledge"), FColor::Red, 1);
		Debug::Print(TEXT("Can't start climbing"), FColor::Red, 2);
		Debug::Print(TEXT("Can't even vault"), FColor::Red, 2);
	}
}

bool UCustomMovementComponent::CanStartVaulting(FVector& OutVaultStart, FVector& OutVaultEnd)
{
	if (IsFalling()) return false;

	OutVaultStart = FVector::ZeroVector;
	OutVaultEnd = FVector::ZeroVector;
	
	const FVector componentLocation = UpdatedComponent->GetComponentLocation();
	const FVector componentForward = UpdatedComponent->GetForwardVector();
	const FVector upVector = UpdatedComponent->GetUpVector();
	const FVector downVector = -UpdatedComponent->GetUpVector();

	for (int32 i = 0; i < 5; i++)
	{
		const FVector start = (componentLocation + upVector * 100.f) + (componentForward * 100.f) * (i + 1);
		const FVector end = start + (downVector * 100.f * (i + 1));

		FHitResult vaultTraceHit = DoLineTraceSingleByObject(start, end, true, true);
		if (i == 0 && vaultTraceHit.bBlockingHit)
		{
			OutVaultStart = vaultTraceHit.ImpactPoint;
		}

		if (i == 4 && vaultTraceHit.bBlockingHit)
		{
			OutVaultEnd = vaultTraceHit.ImpactPoint;
		}
	}
	return  OutVaultStart != FVector::ZeroVector && OutVaultEnd != FVector::ZeroVector;
}

bool UCustomMovementComponent::ShouldClimbLedge()
{
	return CheckHasReachedLedge() && GetUnrotatedClimbVelocity().Z > 10.f;
}

void UCustomMovementComponent::ProcessClimbableSurfaceInformation()
{
	CurrentClimbableSurfaceLocation = FVector::ZeroVector;
	CurrentClimbableSurfaceNormal = FVector::ZeroVector;

	if (ClimbableSurfacesTracedResults.IsEmpty()) return;

	for (const FHitResult tracedHitResult : ClimbableSurfacesTracedResults)
	{
		CurrentClimbableSurfaceLocation += tracedHitResult.ImpactPoint;
		CurrentClimbableSurfaceNormal += tracedHitResult.ImpactNormal;
	}

	CurrentClimbableSurfaceLocation /= ClimbableSurfacesTracedResults.Num();
	CurrentClimbableSurfaceNormal = CurrentClimbableSurfaceNormal.GetSafeNormal();

	//Debug::Print(TEXT("Climbable Surface location: ") + CurrentClimbableSurfaceLocation.ToString(), FColor::Green, 1);
	//Debug::Print(TEXT("Climbable Surface normal: ") + CurrentClimbableSurfaceNormal.ToString(), FColor::Magenta, 2);
}

FQuat UCustomMovementComponent::GetClimbRotation(float DeltaTime)
{
	const FQuat currentQuat = UpdatedComponent->GetComponentQuat();
	if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		return currentQuat;
	}

	const FQuat targetQuat = FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat();
	return FMath::QInterpTo(currentQuat, targetQuat, DeltaTime, 5.f);
}

void UCustomMovementComponent::SnapMovementToClimbableSurfaces(float DeltaTime)
{
	const FVector componentForward = UpdatedComponent->GetForwardVector();
	const FVector componentLocation = UpdatedComponent->GetComponentLocation();

	const FVector projectedCharacterToSurface = (CurrentClimbableSurfaceLocation - componentLocation).ProjectOnTo(
		componentForward);
	const FVector snapVector = -CurrentClimbableSurfaceNormal * projectedCharacterToSurface.Length();

	UpdatedComponent->MoveComponent(
		snapVector * DeltaTime * MaxClimbSpeed,
		UpdatedComponent->GetComponentQuat(),
		true
	);
}

/**
 * TraceClimbableSurfaces traces for avaiable climbable surfaces using capsule traces.
 * Returns true if a climbable surface is found, else returns false.
 * Also fills the internal variable `ClimbableSurfacesTracedResults` with the traced points on the climbable surfaces,
 * if a climbable surface is found.
 * 
 * @return bool
 */
bool UCustomMovementComponent::TraceClimbableSurfaces()
{
	const FVector startOffset = UpdatedComponent->GetForwardVector() * 30.f;
	const FVector start = UpdatedComponent->GetComponentLocation() + startOffset;
	const FVector end = start + UpdatedComponent->GetForwardVector();
	ClimbableSurfacesTracedResults = DoCapsuleTraceMultiByObject(start, end, false, false);
	return !ClimbableSurfacesTracedResults.IsEmpty();
}


/**
 * TraceFromEyeHeight traces from eye height of the character in a single line in forward direction.
 * 
 * @param TraceDistance how far the tracing line should go.
 * @param TraceStartOffset tells the offset of the trace starting point.
 * @return `FHitResult` containing the trace information
 */
FHitResult UCustomMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset)
{
	const FVector componentLocation = UpdatedComponent->GetComponentLocation();
	const FVector eyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight +
		TraceStartOffset);
	const FVector start = componentLocation + eyeHeightOffset;
	const FVector end = start + UpdatedComponent->GetForwardVector() * TraceDistance;
	return DoLineTraceSingleByObject(start, end, false, false);
}


void UCustomMovementComponent::PlayClimbMontage(UAnimMontage* MontageToPlay)
{
	if (!MontageToPlay) return;
	if (!OwningPlayerInstance) return;
	if (OwningPlayerInstance->IsAnyMontagePlaying()) return;
	OwningPlayerInstance->Montage_Play(MontageToPlay);
}

void UCustomMovementComponent::OnCLimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Debug::Print(TEXT("Climb Montage Ended"));
	if (Montage == IdleToClimbMontage || Montage == ClimbDownLedgeMontage)
	{
		StartClimbing();
		// StopMovementImmediately();
	}
	else if (Montage == ClimbToTopMontage || Montage == VaultMontage)
	{
		SetMovementMode(MOVE_Walking);
	}
}

void UCustomMovementComponent::SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition)
{
	if(!OwningPlayerCharacter) return;
	OwningPlayerCharacter->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocation(InWarpTargetName, InTargetPosition);
}

#pragma endregion
