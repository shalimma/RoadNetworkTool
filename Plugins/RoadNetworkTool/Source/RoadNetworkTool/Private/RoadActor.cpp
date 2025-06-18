#include "RoadActor.h"
#include "RoadHelper.h"

bool ARoadActor::EnableRoadDebugLine = false;
float ARoadActor::RoadWidth = 500.0f;
float ARoadActor::RoadThickness = 20.0f;

ARoadActor::ARoadActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARoadActor::BeginPlay()
{
    Super::BeginPlay();
}

void ARoadActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ARoadActor::AddSplineComponent(USplineComponent* SplineComponent)
{
    if (SplineComponent)
    {
        SplineComponents.AddUnique(SplineComponent);
    }
}

const TArray<USplineComponent*>& ARoadActor::GetSplineComponents() const
{
    return SplineComponents;
}
