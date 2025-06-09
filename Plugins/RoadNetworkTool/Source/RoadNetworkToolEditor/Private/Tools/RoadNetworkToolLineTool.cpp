// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadNetworkToolLineTool.h"
#include "DrawDebugHelpers.h"
#include "InteractiveToolManager.h"
#include "BaseBehaviors/ClickDragBehavior.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "RoadActor.h"
#include "RoadNetworkTool/Public/RoadActor.h"
#include "Editor/UnrealEd/Public/Selection.h"
#include "RoadNetworkToolLineToolCustomization.h"
#include "Kismet/GameplayStatics.h"

// for raycast into World
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "SceneManagement.h"

// localization namespace
#define LOCTEXT_NAMESPACE "URoadNetworkToolLineTool"

// ToolBuilder
UInteractiveTool* URoadNetworkToolLineToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
    URoadNetworkToolLineTool* NewTool = NewObject<URoadNetworkToolLineTool>(SceneState.ToolManager);
    NewTool->SetWorld(SceneState.World);
    return NewTool;
}

URoadNetworkToolLineToolProperties::URoadNetworkToolLineToolProperties()
{
}

void URoadNetworkToolLineTool::SetWorld(UWorld* World)
{
    check(World);
    this->TargetWorld = World;
}

void URoadNetworkToolLineTool::Setup()
{
    UInteractiveTool::Setup();

    // Setup click behavior
    UClickDragInputBehavior* ClickBehavior = NewObject<UClickDragInputBehavior>();
    ClickBehavior->Initialize(this);
    AddInputBehavior(ClickBehavior);

    // Create the property set and register it with the tool
    Properties = NewObject<URoadNetworkToolLineToolProperties>(this);
    AddToolPropertySource(Properties);

    // Register the custom detail customization
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout(
        URoadNetworkToolLineToolProperties::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FRoadNetworkToolLineToolCustomization::MakeInstance)
    );

    // Get the currently selected actor
    SplineActor = Cast<ARoadActor>(GetSelectedActor());
    if (SplineActor)
    {
        ApplyProperties(SplineActor);
        SplineComponents = SplineActor->GetSplineComponents();
        CurrentSplineState = ESplineCreationState::Extending;
    }

    bHasOriginPoint = false;
}

FInputRayHit URoadNetworkToolLineTool::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
    FVector Temp;
    FInputRayHit Result = FindRayHit(PressPos.WorldRay, Temp);
    return Result;
}

void URoadNetworkToolLineTool::OnClickPress(const FInputDeviceRay& PressPos)
{
    FVector ClickLocation;
    FInputRayHit HitResult = FindRayHit(PressPos.WorldRay, ClickLocation);

    ARoadActor* NewSelectedActor = Cast<ARoadActor>(GetSelectedActor());
    if (NewSelectedActor && NewSelectedActor != SplineActor)
    {
        SplineActor = NewSelectedActor;
        SplineComponents = SplineActor->GetSplineComponents();
        CurrentSplineComponent = nullptr;
        CurrentSplineState = ESplineCreationState::Extending;
    }

    if (GEditor)
    {
        GEditor->SelectNone(true, true, false);
        GEditor->SelectActor(SplineActor, true, true);
    }

    if (CurrentSplineState == ESplineCreationState::SettingOrigin)
    {
        OriginPoint = ClickLocation;
        DrawDebugSphereEditor(OriginPoint);
        CurrentSplineState = ESplineCreationState::SettingEndPoint;
    }
    else if (CurrentSplineState == ESplineCreationState::SettingEndPoint)
    {
        int32 NearPointIndex;
        FVector NearPointLocation;

        if (GetNearSplinePoint(ClickLocation, NearPointIndex, NearPointLocation, Properties->SnapThreshold))
        {
            EndPoint = NearPointLocation;
        }
        else
        {
            EndPoint = ClickLocation;
        }

        DrawDebugSphereEditor(EndPoint);

        if (ArePointsOnSameSpline(OriginPoint, EndPoint))
        {
            UE_LOG(LogTemp, Warning, TEXT("Origin and End Points are on the same spline. Spline not created."));
            OriginPoint = EndPoint;
            CurrentSplineState = ESplineCreationState::SettingEndPoint;
            return;
        }

        if (OriginPoint.Equals(EndPoint, 1.0f))
        {
            UE_LOG(LogTemp, Warning, TEXT("Origin and End Point are the same. Spline not created."));
            OriginPoint = EndPoint;
            CurrentSplineState = ESplineCreationState::SettingEndPoint;
            return;
        }

        if (IsLineIntersectingSpline(OriginPoint, EndPoint))
        {
            UE_LOG(LogTemp, Warning, TEXT("Line intersects with an existing spline. Spline not created."));
            OriginPoint = EndPoint;
            CurrentSplineState = ESplineCreationState::Extending;
            return;
        }

        CreateSpline();
        CurrentSplineState = ESplineCreationState::Extending;
    }
    else if (CurrentSplineState == ESplineCreationState::Extending)
    {
        int32 NearPointIndex;
        FVector NearPointLocation;

        if (GetNearSplinePoint(ClickLocation, NearPointIndex, NearPointLocation, Properties->SnapThreshold))
        {
            OriginPoint = NearPointLocation;
            DrawDebugSphereEditor(OriginPoint);
            CurrentSplineState = ESplineCreationState::SettingEndPoint;
        }
    }
}

FInputRayHit URoadNetworkToolLineTool::FindRayHit(const FRay& WorldRay, FVector& HitPos)
{
    FCollisionObjectQueryParams QueryParams(FCollisionObjectQueryParams::AllObjects);
    FHitResult Result;
    bool bHitWorld = TargetWorld->LineTraceSingleByObjectType(Result, WorldRay.Origin, WorldRay.PointAt(999999), QueryParams);
    if (bHitWorld)
    {
        HitPos = Result.ImpactPoint;
    }
    return FInputRayHit(Result.Distance);
}

void URoadNetworkToolLineTool::DrawDebugSphereEditor(const FVector& Location)
{
    if (TargetWorld)
    {
        DrawDebugSphere(TargetWorld, Location, 25.0f, 12, FColor::Red, false, 1.0f);
    }
}

void URoadNetworkToolLineTool::CreateSpline()
{
    if (!TargetWorld) return;

    if (!SplineActor)
    {
        SplineActor = TargetWorld->SpawnActor<ARoadActor>();

        FName UniqueName = MakeUniqueObjectName(TargetWorld, ARoadActor::StaticClass(), FName(TEXT("RoadNetwork")));
        SplineActor->SetActorLabel(UniqueName.ToString());
    }

    USplineComponent* SplineComponent = NewObject<USplineComponent>(SplineActor);
    SplineComponent->SetupAttachment(SplineActor->GetRootComponent());
    SplineComponent->RegisterComponentWithWorld(TargetWorld);
    SplineComponent->ClearSplinePoints();

    SplineComponent->AddSplinePoint(OriginPoint, ESplineCoordinateSpace::World);
    SplineComponent->AddSplinePoint(EndPoint, ESplineCoordinateSpace::World);
    SplineComponent->UpdateSpline();

    if (GEditor)
    {
        GEditor->SelectNone(true, true, false);
        GEditor->SelectActor(SplineActor, true, true);
    }

    OriginPoint = FVector::ZeroVector;
    CurrentSplineComponent = SplineComponent;
    SplineComponents = SplineActor->GetSplineComponents();
    ApplyProperties(SplineActor);
}

bool URoadNetworkToolLineTool::GetNearSplinePoint(const FVector& Location, int32& OutPointIndex, FVector& OutPointLocation, float Threshold)
{
    if (!SplineActor)
    {
        return false;
    }

    float MinDistance = Threshold;
    bool bFoundNearPoint = false;
    SplineComponents = SplineActor->GetSplineComponents();

    for (USplineComponent* SplineComponent : SplineComponents)
    {
        for (int32 i = 0; i < SplineComponent->GetNumberOfSplinePoints(); ++i)
        {
            FVector SplinePoint = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
            float Distance = FVector::Dist(Location, SplinePoint);
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                OutPointIndex = i;
                OutPointLocation = SplinePoint;
                bFoundNearPoint = true;
            }
        }
    }

    return bFoundNearPoint;
}

bool URoadNetworkToolLineTool::ArePointsOnSameSpline(const FVector& Point1, const FVector& Point2)
{
    if (!SplineActor)
    {
        return false;
    }

    TArray<USplineComponent*> SplineComponentsArray;
    SplineActor->GetComponents(SplineComponentsArray);

    for (USplineComponent* SplineComponent : SplineComponentsArray)
    {
        int32 Point1Index = INDEX_NONE;
        int32 Point2Index = INDEX_NONE;

        for (int32 i = 0; i < SplineComponent->GetNumberOfSplinePoints(); ++i)
        {
            FVector SplinePoint = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
            if (Point1Index == INDEX_NONE && FVector::Dist(Point1, SplinePoint) < Properties->SnapThreshold)
            {
                Point1Index = i;
            }

            if (Point2Index == INDEX_NONE && FVector::Dist(Point2, SplinePoint) < Properties->SnapThreshold)
            {
                Point2Index = i;
            }

            if (Point1Index != INDEX_NONE && Point2Index != INDEX_NONE)
            {
                return true;
            }
        }
    }

    return false;
}

AActor* URoadNetworkToolLineTool::GetSelectedActor()
{
    if (GEditor)
    {
        USelection* SelectedActors = GEditor->GetSelectedActors();
        if (SelectedActors->Num() > 0)
        {
            return Cast<AActor>(SelectedActors->GetSelectedObject(0));
        }
    }
    return nullptr;
}

bool URoadNetworkToolLineTool::IsLineIntersectingSpline(const FVector& LineStart, const FVector& LineEnd)
{
    if (!SplineActor)
    {
        return false;
    }

    SplineComponents = SplineActor->GetSplineComponents();

    for (USplineComponent* SplineComponent : SplineComponents)
    {
        int32 NumSplinePoints = SplineComponent->GetNumberOfSplinePoints();
        if (NumSplinePoints < 2) continue;

        FVector SplineStart = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
        FVector SplineEnd = SplineComponent->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World);

        if (DoLinesIntersect(LineStart, LineEnd, SplineStart, SplineEnd))
        {
            return true;
        }
    }

    return false;
}

bool URoadNetworkToolLineTool::DoLinesIntersect(const FVector& A1, const FVector& A2, const FVector& B1, const FVector& B2)
{
    FVector2D A1_2D(A1.X, A1.Y);
    FVector2D A2_2D(A2.X, A2.Y);
    FVector2D B1_2D(B1.X, B1.Y);
    FVector2D B2_2D(B2.X, B2.Y);

    FVector2D DirA = A2_2D - A1_2D;
    FVector2D DirB = B2_2D - B1_2D;

    auto PerpDot = [](const FVector2D& V1, const FVector2D& V2) {
        return V1.X * V2.Y - V1.Y * V2.X;
    };

    float Denominator = PerpDot(DirA, DirB);
    if (FMath::IsNearlyZero(Denominator))
    {
        return false;
    }

    FVector2D Diff = B1_2D - A1_2D;
    float t = PerpDot(Diff, DirB) / Denominator;
    float u = PerpDot(Diff, DirA) / Denominator;

    return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
}

void URoadNetworkToolLineTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
    if (SplineActor)
    {
        ApplyProperties(SplineActor);
    }
    else
    {
        ARoadActor* FoundSplineActor = GetFirstRoadActor(TargetWorld);
        if (FoundSplineActor)
        {
            ApplyProperties(FoundSplineActor);
        }
    }
}

ARoadActor* URoadNetworkToolLineTool::GetFirstRoadActor(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, ARoadActor::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0)
    {
        return Cast<ARoadActor>(FoundActors[0]);
    }

    return nullptr;
}

void URoadNetworkToolLineTool::ApplyProperties(ARoadActor* RoadActor)
{
    if (RoadActor)
    {
        RoadActor->EnableRoadDebugLine = Properties->EnableDebugLine;
        RoadActor->RoadWidth = Properties->Width;
        RoadActor->RoadThickness = Properties->Thickness;
    }
}

#undef LOCTEXT_NAMESPACE
