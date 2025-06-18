#include "RoadActor.h"
#include "RoadHelper.h"
#include "Components/SplineComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"

bool ARoadActor::bIsInRoadNetworkMode = false;
bool ARoadActor::EnableRoadDebugLine = false;
float ARoadActor::RoadWidth = 500.0f;
float ARoadActor::RoadThickness = 20.0f;

ARoadActor::ARoadActor()
{
    PrimaryActorTick.bCanEverTick = true;

#if WITH_EDITOR
    FRoadHelper::SetIsRoadActor(this, true);
#endif

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;
}

void ARoadActor::BeginPlay()
{
    Super::BeginPlay();
}

void ARoadActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

#if WITH_EDITOR
    if (GEditor && !GetWorld()->IsGameWorld())
    {
        if (bIsInRoadNetworkMode && EnableRoadDebugLine)
        {
            DrawDebugRoadWidth(RoadWidth, RoadThickness, FColor::Green, 0.0f);
        }
    }
#endif
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

void ARoadActor::DrawDebugRoadWidth(float Width, float Thickness, FColor Color, float Duration)
{
    for (USplineComponent* SplineComponent : SplineComponents)
    {
        if (SplineComponent)
        {
            FVector BoxCenter = FVector::ZeroVector;
            FVector BoxExtent = FVector::ZeroVector;
            FRotator Rotation;

            const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
            for (int32 i = 0; i < NumPoints; ++i)
            {
                BoxCenter = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
                const FVector Tangent = SplineComponent->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);

                BoxExtent = FVector(SplineComponent->GetSplineLength() / 2, RoadWidth * 0.5f, RoadThickness * 0.5f);
                Rotation = Tangent.Rotation();

                BoxCenter += FVector::UpVector * RoadThickness * 0.5f;

                DrawDebugBox(GetWorld(), BoxCenter, BoxExtent, Rotation.Quaternion(), Color, false, Duration);
            }
        }
    }
}

TArray<FIntersectionNode> ARoadActor::FindSplineIntersectionNodes() const
{
    TArray<FIntersectionNode> IntersectionNodes;
    const float IntersectionThreshold = 1.0f;

    for (int32 i = 0; i < SplineComponents.Num(); i++)
    {
        USplineComponent* SplineA = SplineComponents[i];
        if (!SplineA) continue;

        for (int32 j = i + 1; j < SplineComponents.Num(); j++)
        {
            USplineComponent* SplineB = SplineComponents[j];
            if (!SplineB) continue;

            const int32 NumPointsA = SplineA->GetNumberOfSplinePoints();
            const int32 NumPointsB = SplineB->GetNumberOfSplinePoints();

            for (int32 PointIndexA = 0; PointIndexA < NumPointsA - 1; PointIndexA++)
            {
                FVector StartA = SplineA->GetLocationAtSplinePoint(PointIndexA, ESplineCoordinateSpace::World);
                FVector EndA = SplineA->GetLocationAtSplinePoint(PointIndexA + 1, ESplineCoordinateSpace::World);

                for (int32 PointIndexB = 0; PointIndexB < NumPointsB - 1; PointIndexB++)
                {
                    FVector StartB = SplineB->GetLocationAtSplinePoint(PointIndexB, ESplineCoordinateSpace::World);
                    FVector EndB = SplineB->GetLocationAtSplinePoint(PointIndexB + 1, ESplineCoordinateSpace::World);

                    FVector IntersectionPoint;
                    if (FMath::SegmentIntersection2D(StartA, EndA, StartB, EndB, IntersectionPoint))
                    {
                        bool bIsNearExistingNode = false;
                        for (FIntersectionNode& Node : IntersectionNodes)
                        {
                            if (FVector::DistSquared(IntersectionPoint, Node.IntersectionPoint) <= FMath::Square(IntersectionThreshold))
                            {
                                Node.IntersectingSplines.AddUnique(SplineA);
                                Node.IntersectingSplines.AddUnique(SplineB);
                                bIsNearExistingNode = true;
                                break;
                            }
                        }

                        if (!bIsNearExistingNode)
                        {
                            FIntersectionNode NewNode(IntersectionPoint);
                            NewNode.IntersectingSplines.AddUnique(SplineA);
                            NewNode.IntersectingSplines.AddUnique(SplineB);
                            IntersectionNodes.Add(NewNode);
                        }
                    }
                }
            }
        }
    }

    return IntersectionNodes;
}

TArray<FNonIntersectionNode> ARoadActor::FindSplineNonIntersectionNodes() const
{
    TArray<FNonIntersectionNode> NonIntersectionNodes;
    const float IntersectionThreshold = 1.0f;
    TArray<FIntersectionNode> IntersectionNodes = FindSplineIntersectionNodes();

    for (USplineComponent* Spline : SplineComponents)
    {
        if (!Spline) continue;

        const int32 NumPoints = Spline->GetNumberOfSplinePoints();
        for (int32 PointIndex = 0; PointIndex < NumPoints; PointIndex++)
        {
            FVector SplinePoint = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);

            bool bIsIntersecting = false;
            for (const FIntersectionNode& Node : IntersectionNodes)
            {
                if (FVector::DistSquared(SplinePoint, Node.IntersectionPoint) <= FMath::Square(IntersectionThreshold))
                {
                    bIsIntersecting = true;
                    break;
                }
            }

            if (!bIsIntersecting)
            {
                FNonIntersectionNode NewNode(SplinePoint);
                NewNode.NonIntersectingSplines.Add(Spline);
                NonIntersectionNodes.Add(NewNode);
            }
        }
    }

    return NonIntersectionNodes;
}

TArray<FLineSegment> ARoadActor::GenerateRectangularRoadSections(const TArray<USplineComponent*>& RoadSplineComponents, float Width)
{
    TArray<FLineSegment> LineSegments;

    for (USplineComponent* SplineComponent : RoadSplineComponents)
    {
        if (!SplineComponent) continue;

        const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();

        for (int32 i = 0; i < NumPoints - 1; ++i)
        {
            float LineThickness = 2.5f;

            FVector Start = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
            FVector End = SplineComponent->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

            FVector Tangent = (End - Start).GetSafeNormal();
            FVector RightVector = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal() * Width * 0.5f;

            FVector Corner1 = Start - RightVector;
            FVector Corner2 = Start + RightVector;
            FVector Corner3 = End + RightVector;
            FVector Corner4 = End - RightVector;

            LineSegments.Add(FLineSegment(Corner2, Corner3, SplineComponent));
            LineSegments.Add(FLineSegment(Corner4, Corner1, SplineComponent));

            // Optional debug lines:
            /*
            DrawDebugLine(GetWorld(), Corner2, Corner3, FColor::Green, false, 5.0f, 0, LineThickness);
            DrawDebugLine(GetWorld(), Corner4, Corner1, FColor::Green, false, 5.0f, 0, LineThickness);
            */
        }
    }

    return LineSegments;
}

bool ARoadActor::LineIntersection(const FVector& Line1Start, const FVector& Line1End, const FVector& Line2Start, const FVector& Line2End, FVector& OutIntersection)
{
    FVector Line1Dir = Line1End - Line1Start;
    FVector Line2Dir = Line2End - Line2Start;

    float A1 = Line1Dir.Y;
    float B1 = -Line1Dir.X;
    float C1 = A1 * Line1Start.X + B1 * Line1Start.Y;

    float A2 = Line2Dir.Y;
    float B2 = -Line2Dir.X;
    float C2 = A2 * Line2Start.X + B2 * Line2Start.Y;

    float Determinant = A1 * B2 - A2 * B1;

    if (FMath::Abs(Determinant) < KINDA_SMALL_NUMBER)
    {
        // Lines are parallel
        return false;
    }

    OutIntersection.X = (B2 * C1 - B1 * C2) / Determinant;
    OutIntersection.Y = (A1 * C2 - A2 * C1) / Determinant;
    OutIntersection.Z = Line1Start.Z;

    if (FVector::DotProduct(Line1Dir, OutIntersection - Line1Start) < 0 || FVector::DotProduct(Line1Dir, OutIntersection - Line1End) > 0)
        return false;

    if (FVector::DotProduct(Line2Dir, OutIntersection - Line2Start) < 0 || FVector::DotProduct(Line2Dir, OutIntersection - Line2End) > 0)
        return false;

    return true;
}

TArray<FVector> ARoadActor::FindInterPointsFromInterNode(const TArray<FLineSegment>& LineSegments, const FIntersectionNode& IntersectionNode)
{
    TArray<FVector> IntersectionPoints;
    TMap<int32, FVector> FurthestIntersectionPerSegment;
    FVector IntersectionNodePosition = IntersectionNode.IntersectionPoint;

    for (int32 i = 0; i < LineSegments.Num(); ++i)
    {
        for (int32 j = i + 1; j < LineSegments.Num(); ++j)
        {
            FVector Intersection;
            if (LineIntersection(LineSegments[i].Start, LineSegments[i].End, LineSegments[j].Start, LineSegments[j].End, Intersection))
            {
                float Distance = FVector::Dist(IntersectionNodePosition, Intersection);

                if (!FurthestIntersectionPerSegment.Contains(i) || FVector::Dist(IntersectionNodePosition, FurthestIntersectionPerSegment[i]) < Distance)
                    FurthestIntersectionPerSegment.Add(i, Intersection);

                if (!FurthestIntersectionPerSegment.Contains(j) || FVector::Dist(IntersectionNodePosition, FurthestIntersectionPerSegment[j]) < Distance)
                    FurthestIntersectionPerSegment.Add(j, Intersection);
            }
        }
    }

    for (const TPair<int32, FVector>& Pair : FurthestIntersectionPerSegment)
    {
        IntersectionPoints.Add(Pair.Value);
    }

    return IntersectionPoints;
}

bool ARoadActor::ShouldTickIfViewportsOnly() const
{
    return true;
}
