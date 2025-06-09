#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "RoadActor.generated.h"

UCLASS()
class ROADNETWORKTOOL_API ARoadActor : public AActor
{
    GENERATED_BODY()

public:
    ARoadActor();

    static bool EnableRoadDebugLine;
    static float RoadWidth;
    static float RoadThickness;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Splines")
    TArray<USplineComponent*> SplineComponents;

    void AddSplineComponent(USplineComponent* SplineComponent);
    const TArray<USplineComponent*>& GetSplineComponents() const;

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
};
