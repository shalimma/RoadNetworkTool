// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadProjectGameMode.h"
#include "RoadProjectPlayerController.h"

ARoadProjectGameMode::ARoadProjectGameMode()
{
	PlayerControllerClass = ARoadProjectPlayerController::StaticClass();
}
