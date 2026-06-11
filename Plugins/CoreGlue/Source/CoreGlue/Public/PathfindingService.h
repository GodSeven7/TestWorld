#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PathfindingService.generated.h"

USTRUCT(BlueprintType)
struct FFlowFieldResult
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVector> PathPoints;

    UPROPERTY()
    float TotalCost = 0.0f;

    UPROPERTY()
    bool bSuccess = false;
};

USTRUCT(BlueprintType)
struct FFlowFieldRequestHandle
{
    GENERATED_BODY()

    UPROPERTY()
    int32 RequestId = -1;

    bool IsValid() const { return RequestId >= 0; }
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFlowFieldComplete, const FFlowFieldRequestHandle&, const FFlowFieldResult&);

UINTERFACE(MinimalAPI, Blueprintable)
class UPathfindingService : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API IPathfindingService
{
    GENERATED_BODY()

public:
    virtual bool GenerateFlowFieldSync(
        const FVector& Target,
        FFlowFieldResult& OutResult) = 0;

    virtual FFlowFieldRequestHandle RequestFlowFieldAsync(
        const FVector& Target,
        const FOnFlowFieldComplete::FDelegate& Callback) = 0;

    virtual void CancelFlowFieldRequest(const FFlowFieldRequestHandle& Handle) = 0;

    virtual bool GetFlowDirection(
        const FVector& Position,
        const FVector& Target,
        FVector& OutDirection) = 0;

    virtual bool GetPathFromFlowField(
        const FVector& Start,
        const FVector& Target,
        TArray<FVector>& OutPathPoints) = 0;

    virtual bool IsPointWalkable(const FVector& Point) const = 0;
};
