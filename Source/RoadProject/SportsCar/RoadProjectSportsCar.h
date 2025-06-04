// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RoadProjectPawn.h"
#include "RoadProjectSportsCar.generated.h"

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class ARoadProjectSportsCar : public ARoadProjectPawn
{
	GENERATED_BODY()
	
public:

	ARoadProjectSportsCar();
};
