﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

class UAnimMontage;
class UAnimInstance;
class ADPOCGGameCharacter;

UENUM(BlueprintType)
namespace ECustomMovementMode {
	enum Type {
		MOVE_Climb UMETA(DisplayName="Climb Mode")
	};
}


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DPOCGGAME_API UCustomMovementComponent : public UCharacterMovementComponent {
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCustomMovementComponent();

	void ToggleClimbing(bool bEnableClimbing);
	bool IsClimbing() const;
	FORCEINLINE FVector GetClimbableSurfaceNormal() const { return CurrentClimbableSurfaceNormal; }
	FVector GetUnrotatedClimbVelocity() const;

private:
#pragma region ClimbTraces
	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape,
	                                               bool bDrawPersistentTraces = false);
	FHitResult DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape,
	                                     bool bDrawPersistentTraces = false);
#pragma endregion

#pragma region ClimbCoreVariables
	TArray<FHitResult> ClimbableSurfacesTracedResults;
	FVector CurrentClimbableSurfaceLocation;
	FVector CurrentClimbableSurfaceNormal;
	UPROPERTY()
	UAnimInstance* OwningPlayerInstance;

	UPROPERTY()
	ADPOCGGameCharacter* OwningPlayerCharacter;
#pragma endregion

#pragma region ClimbVariablesBlueprintable
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	TArray<TEnumAsByte<EObjectTypeQuery>> ClimbableSurfaceTraceTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	float ClimbCapsuleTraceRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	float ClimbCapsuleTraceHalfHeight = 72.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	float MaxBreakCLimbDeceleration = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	float MaxClimbSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	float MaxClimbAcceleration = 300.f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	float ClimbDownSurfaceTraceOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	float ClimbDownLedgeTraceOffset = 50.f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	float ClimbUpEyeTraceOffset = 20.f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	UAnimMontage* IdleToClimbMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	UAnimMontage* ClimbToTopMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	UAnimMontage* ClimbDownLedgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Movement: Climbing",
		meta=(AllowPrivateAccess="true"))
	UAnimMontage* VaultMontage;

#pragma endregion

#pragma region ClimbCore
	bool TraceClimbableSurfaces();
	FHitResult TraceFromEyeHeight(float TraceDistance, float TraceStartOffset = 0.f);
	bool CanStartClimbing();
	void StartClimbing();
	bool CheckCanClimbDownLedge();
	void StopClimbing();
	void PhysClimb(float deltaTime, int32 Iterations);
	bool CheckShouldStopClimbing();
	bool CheckHasReachedFloor();
	bool CheckHasReachedLedge();
	void TryStartVaulting();
	bool CanStartVaulting(FVector& OutVaultStart, FVector& OutVaultEnd);
	bool ShouldClimbLedge();
	void ProcessClimbableSurfaceInformation();
	FQuat GetClimbRotation(float DeltaTime);
	void SnapMovementToClimbableSurfaces(float DeltaTime);
	void PlayClimbMontage(UAnimMontage* MontageToPlay);

	UFUNCTION()
	void OnCLimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition);
#pragma endregion

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxSpeed() const override;
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity,
	                                                const FVector& CurrentVelocity) const override;
};
