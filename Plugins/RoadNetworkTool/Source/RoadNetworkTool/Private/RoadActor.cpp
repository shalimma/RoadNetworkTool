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
            DrawDebugRoadWidth(RoadWidth, RoadThickness, FColor::Green, 10.0f);
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
            
            DrawDebugLine(GetWorld(), Corner2, Corner3, FColor::Green, false, 5.0f, 0, LineThickness);
            DrawDebugLine(GetWorld(), Corner4, Corner1, FColor::Green, false, 5.0f, 0, LineThickness);
            
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

TArray<FVector> ARoadActor::FindNonInterPointsFromInterNode(const TArray<FLineSegment>& LineSegments, const FIntersectionNode& IntersectionNode)
{
    TArray<FVector> NonIntersectionPoints;
    FVector IntersectionNodePosition = IntersectionNode.IntersectionPoint;
    TSet<int32> IntersectingSegments;

    // Highlight the intersection node itself
    DrawDebugSphere(GetWorld(), IntersectionNodePosition, 25.f, 12, FColor::Red, false, 5.f);

    // First pass: find intersecting segments
    for (int32 i = 0; i < LineSegments.Num(); ++i)
    {
        for (int32 j = i + 1; j < LineSegments.Num(); ++j)
        {
            FVector Intersection;
            if (LineIntersection(LineSegments[i].Start, LineSegments[i].End, LineSegments[j].Start, LineSegments[j].End, Intersection))
            {
                IntersectingSegments.Add(i);
                IntersectingSegments.Add(j);

                // can commnet out draw intersecting segment lines
                DrawDebugLine(GetWorld(), LineSegments[i].Start, LineSegments[i].End, FColor::Magenta, false, 5.f, 0, 2.f);
                DrawDebugLine(GetWorld(), LineSegments[j].Start, LineSegments[j].End, FColor::Magenta, false, 5.f, 0, 2.f);
            }
        }
    }

    // Second pass: get endpoint closest to the intersection node
    for (int32 i = 0; i < LineSegments.Num(); ++i)
    {
        if (IntersectingSegments.Contains(i))
        {
            float StartDistance = FVector::Dist(IntersectionNodePosition, LineSegments[i].Start);
            float EndDistance = FVector::Dist(IntersectionNodePosition, LineSegments[i].End);

            FVector ChosenPoint = (StartDistance < EndDistance) ? LineSegments[i].Start : LineSegments[i].End;
            NonIntersectionPoints.Add(ChosenPoint);

            // Debug: highlight chosen points
            DrawDebugSphere(GetWorld(), ChosenPoint, 20.f, 12, FColor::Black, false, 5.f);
            DrawDebugLine(GetWorld(), IntersectionNodePosition, ChosenPoint, FColor::Cyan, false, 5.f, 0, 1.f);
        }
    }

    return NonIntersectionPoints;
}


TArray<FVector> ARoadActor::FindPointsFromNonInterNode(const TArray<FNonIntersectionNode>& NonIntersectionNodes, float Width) const
{
    TArray<FVector> DeadEndPoints;
    const float DeadEndThreshold = 1.0f;

    for (const FNonIntersectionNode& Node : NonIntersectionNodes)
    {
        FVector SplinePoint = Node.NonIntersectionPoint;
        FVector Tangent = FVector::ZeroVector;
        FVector RightVector = FVector::ZeroVector;

        for (USplineComponent* Spline : Node.NonIntersectingSplines)
        {
            if (!Spline) continue;

            const int32 NumPoints = Spline->GetNumberOfSplinePoints();
            for (int32 PointIndex = 0; PointIndex < NumPoints; PointIndex++)
            {
                FVector Point = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);

                if (FVector::DistSquared(SplinePoint, Point) <= FMath::Square(DeadEndThreshold))
                {
                    Tangent = Spline->GetTangentAtSplinePoint(PointIndex, ESplineCoordinateSpace::World).GetSafeNormal();
                    RightVector = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal() * Width * 0.5f;
                    break;
                }
            }

            if (!Tangent.IsZero())
                break;
        }

        FVector LeftPoint = SplinePoint - RightVector;
        FVector RightPoint = SplinePoint + RightVector;
        DeadEndPoints.Add(LeftPoint);
        DeadEndPoints.Add(RightPoint);
    }

    return DeadEndPoints;
}


void OrderPointsClockwise(TArray<FVector>& Points)
{
    if (Points.Num() < 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough points to order."));
        return;
    }

    FVector Centroid(0, 0, 0);
    for (const FVector& Point : Points)
    {
        Centroid += Point;
    }
    Centroid /= Points.Num();

    Points.Sort([Centroid](const FVector& A, const FVector& B)
        {
            float AngleA = FMath::Atan2(A.Y - Centroid.Y, A.X - Centroid.X);
            float AngleB = FMath::Atan2(B.Y - Centroid.Y, B.X - Centroid.X);
            return AngleA > AngleB;
        });
}

TArray<FVector> ARoadActor::FindLineSegmentPoints(const TArray<FLineSegment>& LineSegments, USplineComponent* SplineComponent, const TArray<FVector>& InRoadPoints)
{
    TArray<FVector> OverlappingPoints;
    float Threshold = 10.0f;
    for (const FLineSegment& LineSegment : LineSegments)
    {
        if (LineSegment.SplineComponent != SplineComponent) continue;

        FVector LineDirection = LineSegment.End - LineSegment.Start;
        float LineLengthSquared = LineDirection.SizeSquared();
        if (LineLengthSquared <= KINDA_SMALL_NUMBER)
            continue;

        LineDirection.Normalize();

        for (const FVector& RoadPoint : InRoadPoints)
        {
            FVector StartToPoint = RoadPoint - LineSegment.Start;
            float Projection = FVector::DotProduct(StartToPoint, LineDirection);
            FVector ClosestPoint = LineSegment.Start + FMath::Clamp(Projection, 0.0f, FVector::Dist(LineSegment.Start, LineSegment.End)) * LineDirection;

            if (FVector::DistSquared(ClosestPoint, RoadPoint) <= FMath::Square(Threshold))
            {
                OverlappingPoints.Add(RoadPoint);
            }
        }
    }

    return OverlappingPoints;
}

void ARoadActor::DestroyProceduralMeshes()
{
    
    for (UProceduralMeshComponent* ProcMeshComponent : ProceduralMeshes)
    {
        if (ProcMeshComponent)
        {
            ProcMeshComponent->DestroyComponent();
        }
    }
    ProceduralMeshes.Empty();
}

void ARoadActor::GenerateMeshFromPoints(const TArray<FVector>& Points, float Thickness)
{
    if (Points.Num() < 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough points to create a mesh."));
        return;
    }

    TArray<FVector> OrderedPoints = Points;
    OrderPointsClockwise(OrderedPoints);

    UProceduralMeshComponent* ProcMeshComponent = NewObject<UProceduralMeshComponent>(this);
    ProcMeshComponent->SetupAttachment(RootComponent);
    ProcMeshComponent->RegisterComponentWithWorld(GetWorld());

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    int32 NumVertices = OrderedPoints.Num();
    float UVscale = 0.1f;

    // Add bottom vertices
    for (int32 i = 1; i < NumVertices - 1; ++i)
    {
        if (Thickness > 0.0f)
        {
            // First triangle vertex
            Vertices.Add(OrderedPoints[0]);
            Vertices.Add(OrderedPoints[i + 1]);
            Vertices.Add(OrderedPoints[i]);

            // Add UVs for these vertices
            UVs.Add(FVector2D(OrderedPoints[0].X, OrderedPoints[0].Y) * UVscale);
            UVs.Add(FVector2D(OrderedPoints[i + 1].X, OrderedPoints[i + 1].Y) * UVscale);
            UVs.Add(FVector2D(OrderedPoints[i].X, OrderedPoints[i].Y) * UVscale);

            // Triangles
            int32 VertIndex = Vertices.Num() - 3;
            Triangles.Add(VertIndex);
            Triangles.Add(VertIndex + 1);
            Triangles.Add(VertIndex + 2);

            // Calculate normal for this triangle
            FVector Edge1 = Vertices[VertIndex + 1] - Vertices[VertIndex];
            FVector Edge2 = Vertices[VertIndex + 2] - Vertices[VertIndex];
            FVector Normal = FVector::CrossProduct(Edge2, Edge1).GetSafeNormal();

            Normals.Add(Normal);
            Normals.Add(Normal);
            Normals.Add(Normal);

            // Calculate tangents for this triangle
            FVector TangentX = (Vertices[VertIndex + 1] - Vertices[VertIndex]).GetSafeNormal();
            FVector TangentY = FVector::CrossProduct(Normal, TangentX).GetSafeNormal();
            FProcMeshTangent Tangent = FProcMeshTangent(TangentX, false);

            Tangents.Add(Tangent);
            Tangents.Add(Tangent);
            Tangents.Add(Tangent);
        }
        else
        {
            // First triangle vertex
            Vertices.Add(OrderedPoints[0]);
            Vertices.Add(OrderedPoints[i]);
            Vertices.Add(OrderedPoints[i + 1]);

            // Triangles
            int32 VertIndex = Vertices.Num() - 3;
            Triangles.Add(VertIndex);
            Triangles.Add(VertIndex + 1);
            Triangles.Add(VertIndex + 2);
        }
    }

    // Create triangles for the top face (if thickness > 0)
    if (Thickness > 0.0f)
    {
        for (int32 i = 1; i < NumVertices - 1; ++i)
        {
            FVector TopOffset = FVector(0, 0, Thickness);

            // First triangle vertex
            Vertices.Add(OrderedPoints[0] + TopOffset);
            Vertices.Add(OrderedPoints[i] + TopOffset);
            Vertices.Add(OrderedPoints[i + 1] + TopOffset);

            // Add UVs for these vertices
            UVs.Add(FVector2D(OrderedPoints[0].X, OrderedPoints[0].Y) * UVscale);
            UVs.Add(FVector2D(OrderedPoints[i].X, OrderedPoints[i].Y) * UVscale);
            UVs.Add(FVector2D(OrderedPoints[i + 1].X, OrderedPoints[i + 1].Y) * UVscale);

            // Triangles
            int32 VertIndex = Vertices.Num() - 3;
            Triangles.Add(VertIndex);
            Triangles.Add(VertIndex + 1);
            Triangles.Add(VertIndex + 2);

            // Calculate normal for this triangle
            FVector Edge1 = Vertices[VertIndex + 1] - Vertices[VertIndex];
            FVector Edge2 = Vertices[VertIndex + 2] - Vertices[VertIndex];
            FVector Normal = FVector::CrossProduct(Edge2, Edge1).GetSafeNormal();

            Normals.Add(Normal);
            Normals.Add(Normal);
            Normals.Add(Normal);

            // Calculate tangents for this triangle
            FVector TangentX = (Vertices[VertIndex + 1] - Vertices[VertIndex]).GetSafeNormal();
            FVector TangentY = FVector::CrossProduct(Normal, TangentX).GetSafeNormal();
            FProcMeshTangent Tangent = FProcMeshTangent(TangentX, false);

            Tangents.Add(Tangent);
            Tangents.Add(Tangent);
            Tangents.Add(Tangent);
        }

        // Create side faces
        for (int32 i = 0; i < NumVertices; ++i)
        {
            int32 NextIndex = (i + 1) % NumVertices;
            FVector TopOffset = FVector(0, 0, Thickness);

            // First triangle
            Vertices.Add(OrderedPoints[i]);
            Vertices.Add(OrderedPoints[NextIndex]);
            Vertices.Add(OrderedPoints[NextIndex] + TopOffset);

            Vertices.Add(OrderedPoints[NextIndex] + TopOffset);
            Vertices.Add(OrderedPoints[i] + TopOffset);
            Vertices.Add(OrderedPoints[i]);

            // UVs
            UVs.Add(FVector2D(OrderedPoints[i].X, OrderedPoints[i].Y) * UVscale);
            UVs.Add(FVector2D(OrderedPoints[NextIndex].X, OrderedPoints[NextIndex].Y) * UVscale);
            UVs.Add(FVector2D(OrderedPoints[NextIndex].X, OrderedPoints[NextIndex].Y) * UVscale);

            UVs.Add(FVector2D(OrderedPoints[NextIndex].X, OrderedPoints[NextIndex].Y) * UVscale);
            UVs.Add(FVector2D(OrderedPoints[i].X, OrderedPoints[i].Y) * UVscale);
            UVs.Add(FVector2D(OrderedPoints[i].X, OrderedPoints[i].Y) * UVscale);

            // Triangles
            int32 VertIndex = Vertices.Num() - 6;
            Triangles.Add(VertIndex);
            Triangles.Add(VertIndex + 1);
            Triangles.Add(VertIndex + 2);
            Triangles.Add(VertIndex + 3);
            Triangles.Add(VertIndex + 4);
            Triangles.Add(VertIndex + 5);

            // Calculate normal for each face
            for (int j = 0; j < 2; ++j)
            {
                FVector FaceEdge1 = Vertices[VertIndex + j * 3 + 1] - Vertices[VertIndex + j * 3];
                FVector FaceEdge2 = Vertices[VertIndex + j * 3 + 2] - Vertices[VertIndex + j * 3];
                FVector FaceNormal = FVector::CrossProduct(FaceEdge2, FaceEdge1).GetSafeNormal();

                Normals.Add(FaceNormal);
                Normals.Add(FaceNormal);
                Normals.Add(FaceNormal);

                FVector TangentX = (Vertices[VertIndex + j * 3 + 1] - Vertices[VertIndex + j * 3]).GetSafeNormal();
                FVector TangentY = FVector::CrossProduct(FaceNormal, TangentX).GetSafeNormal();
                FProcMeshTangent Tangent = FProcMeshTangent(TangentX, false);

                Tangents.Add(Tangent);
                Tangents.Add(Tangent);
                Tangents.Add(Tangent);
            }
        }
    }

    ProcMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);

    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_Solid_Blue.MI_Solid_Blue"));
    if (Material)
    {
        ProcMeshComponent->SetMaterial(0, Material);
    }

    ProceduralMeshes.Add(ProcMeshComponent);
}

void ARoadActor::GenerateRoadMesh()
{
    DestroyProceduralMeshes();

    TArray<FIntersectionNode> IntersectionNodes = this->FindSplineIntersectionNodes();
    TArray<FVector> AllRoadPoints;
    TArray<FLineSegment> AllLineSegments = GenerateRectangularRoadSections(SplineComponents, RoadWidth);

    for (const FIntersectionNode& IntersectionNode : IntersectionNodes)
    {
        TArray<FLineSegment> LineSegments = GenerateRectangularRoadSections(IntersectionNode.IntersectingSplines, RoadWidth);

        TArray<FVector> IntersectionPoints = FindInterPointsFromInterNode(LineSegments, IntersectionNode.IntersectionPoint);
        TArray<FVector> NonIntersectionPoints = FindNonInterPointsFromInterNode(LineSegments, IntersectionNode.IntersectionPoint);

        TArray<FVector> IntersectionNodePoints;
        IntersectionNodePoints.Empty();
        IntersectionNodePoints.Append(IntersectionPoints);
        IntersectionNodePoints.Append(NonIntersectionPoints);

        AllRoadPoints.Append(IntersectionNodePoints);

        for (const FVector& IntersectionNodePoint : IntersectionNodePoints)
        {
            DrawDebugSphere(this->GetWorld(), IntersectionNodePoint, 25.0f, 12, FColor::Blue, false, 1.0f);
        }

        GenerateMeshFromPoints(IntersectionNodePoints, RoadThickness);
    }

    TArray<FNonIntersectionNode> NonIntersectionNodes = this->FindSplineNonIntersectionNodes();
    TArray<FVector> DeadEndPoints = FindPointsFromNonInterNode(NonIntersectionNodes, RoadWidth);
    AllRoadPoints.Append(DeadEndPoints);

    for (const FVector& DeadEndPoint : DeadEndPoints)
    {
        DrawDebugSphere(this->GetWorld(), DeadEndPoint, 25.0f, 12, FColor::Yellow, false, 1.0f);
    }

    for (USplineComponent* SplineComponent : SplineComponents)
    {
        TArray<FVector> LineSegmentPoints = FindLineSegmentPoints(AllLineSegments, SplineComponent, AllRoadPoints);
        GenerateMeshFromPoints(LineSegmentPoints, RoadThickness);
    }
}



bool ARoadActor::ShouldTickIfViewportsOnly() const
{
    return true;
}
