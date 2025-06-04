// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadProjectWheelFront.h"
#include "UObject/ConstructorHelpers.h"

URoadProjectWheelFront::URoadProjectWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}