// Copyright Epic Games, Inc. All Rights Reserved.

#include "DPOCGGameGameMode.h"
#include "DPOCGGameCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADPOCGGameGameMode::ADPOCGGameGameMode() {
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL) {
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
