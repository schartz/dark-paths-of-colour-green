// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/DPOCGameCharacterAnimInstance.h"

#include "DPOCGGame/DPOCGGameCharacter.h"
#include "Components/CustomMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UDPOCGameCharacterAnimInstance::NativeInitializeAnimation() {
	Super::NativeInitializeAnimation();
	GameCharacter = Cast<ADPOCGGameCharacter>(TryGetPawnOwner());

	if (GameCharacter) {
		CustomMovementComponent = GameCharacter->GetCustomMovementComponent();
	}
}

void UDPOCGameCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds) {
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!GameCharacter || !CustomMovementComponent) return;

	GetGroundSpeed();
	GetAirSpeed();
	GetShouldMove();
	GetIsFalling();
	GetIsClimbing();
	GetClimbVelocity();
}

void UDPOCGameCharacterAnimInstance::GetGroundSpeed() {
	GroundSpeed = UKismetMathLibrary::VSizeXY(GameCharacter->GetVelocity());
}

void UDPOCGameCharacterAnimInstance::GetAirSpeed() {
	AirSpeed = GameCharacter->GetVelocity().Z;
}

void UDPOCGameCharacterAnimInstance::GetShouldMove() {
	bShouldMove = CustomMovementComponent->GetCurrentAcceleration().Size() > 0 && GroundSpeed > 5.0f && !bIsFalling;
}

void UDPOCGameCharacterAnimInstance::GetIsFalling() {
	bIsFalling = CustomMovementComponent->IsFalling();
}

void UDPOCGameCharacterAnimInstance::GetIsClimbing() {
	bIsClimbing = CustomMovementComponent->IsClimbing();
}

void UDPOCGameCharacterAnimInstance::GetClimbVelocity() {
	ClimbVelocity = CustomMovementComponent->GetUnrotatedClimbVelocity();
}
