// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DPOCGameCharacterAnimInstance.generated.h"

class UCustomMovementComponent;
class ADPOCGGameCharacter;
/**
 * 
 */
UCLASS()
class DPOCGGAME_API UDPOCGameCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY()
	ADPOCGGameCharacter* GameCharacter;

	UPROPERTY()
	UCustomMovementComponent* CustomMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Reference, meta=(AllowPrivateAccess="true"))
	float GroundSpeed;
	void GetGroundSpeed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Reference, meta=(AllowPrivateAccess="true"))
	float AirSpeed;
	void GetAirSpeed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Reference, meta=(AllowPrivateAccess="true"))
	bool bShouldMove;
	void GetShouldMove();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Reference, meta=(AllowPrivateAccess="true"))
	bool bIsFalling;
	void GetIsFalling();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Reference, meta=(AllowPrivateAccess="true"))
	bool bIsClimbing;
	void GetIsClimbing();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Reference, meta=(AllowPrivateAccess="true"))
	FVector ClimbVelocity;
	void GetClimbVelocity();
};
