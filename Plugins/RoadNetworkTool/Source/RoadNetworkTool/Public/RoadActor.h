#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "RoadActor.generated.h"

USTRUCT(BlueprintType)
struct FIntersectionNode
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Intersection")
    FVector IntersectionPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Intersection")
    TArray<USplineComponent*> IntersectingSplines;

    FIntersectionNode()
        : IntersectionPoint(FVector::ZeroVector)
    {
    }

    FIntersectionNode(const FVector& InIntersectionPoint)
        : IntersectionPoint(InIntersectionPoint)
    {
    }
};

USTRUCT(BlueprintType)
struct FNonIntersectionNode
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NonIntersection")
    FVector NonIntersectionPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NonIntersection")
    TArray<USplineComponent*> NonIntersectingSplines;

    FNonIntersectionNode()
        : NonIntersectionPoint(FVector::ZeroVector)
    {
    }

    FNonIntersectionNode(const FVector& InNonIntersectionPoint)
        : NonIntersectionPoint(InNonIntersectionPoint)
    {
    }
};

USTRUCT(BlueprintType)
struct FLineSegment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line Segment")
    FVector Start;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line Segment")
    FVector End;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Line Segment")
    USplineComponent* SplineComponent;

    FLineSegment()
        : Start(FVector::ZeroVector), End(FVector::ZeroVector), SplineComponent(nullptr)
    {
    }

    FLineSegment(const FVector& InStart, const FVector& InEnd, USplineComponent* InSplineComponent)
        : Start(InStart), End(InEnd), SplineComponent(InSplineComponent)
    {
    }
};

UCLASS()
class ROADNETWORKTOOL_API ARoadActor : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ARoadActor();

    static bool bIsInRoadNetworkMode;

    static bool EnableRoadDebugLine;
    static float RoadWidth;
    static float RoadThickness;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Splines")
    TArray<USplineComponent*> SplineComponents;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProceduralMesh")
    TArray<UProceduralMeshComponent*> ProceduralMeshes;

    TArray<FLineSegment> RectangleLineSegments;

    void AddSplineComponent(USplineComponent* SplineComponent);
    const TArray<USplineComponent*>& GetSplineComponents() const;

    void DrawDebugRoadWidth(float Width, float Thickness, FColor Color, float Duration);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Allow ticking in the editor
    virtual bool ShouldTickIfViewportsOnly() const override;

    TArray<FIntersectionNode> FindSplineIntersectionNodes() const;
    TArray<FNonIntersectionNode> FindSplineNonIntersectionNodes() const;

    TArray<FLineSegment> GenerateRectangularRoadSections(const TArray<USplineComponent*>& RoadSplineComponents, float Width);
    bool LineIntersection(const FVector& Line1Start, const FVector& Line1End, const FVector& Line2Start, const FVector& Line2End, FVector& OutIntersection);

    TArray<FVector> FindInterPointsFromInterNode(const TArray<FLineSegment>& LineSegments, const FIntersectionNode& IntersectionNode);
    TArray<FVector> FindNonInterPointsFromInterNode(const TArray<FLineSegment>& LineSegments, const FIntersectionNode& IntersectionNode);
    TArray<FVector> FindPointsFromNonInterNode(const TArray<FNonIntersectionNode>& NonIntersectionNodes, float Width) const;
    TArray<FVector> FindLineSegmentPoints(const TArray<FLineSegment>& LineSegments, USplineComponent* SplineComponent, const TArray<FVector>& InRoadPoints);
    void DestroyProceduralMeshes();

    void GenerateMeshFromPoints(const TArray<FVector>& Points, float Thickness);
    void GenerateRoadMesh();
};
