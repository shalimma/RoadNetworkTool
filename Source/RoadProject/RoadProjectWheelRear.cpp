// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadProjectWheelRear.h"
#include "UObject/ConstructorHelpers.h"

URoadProjectWheelRear::URoadProjectWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}