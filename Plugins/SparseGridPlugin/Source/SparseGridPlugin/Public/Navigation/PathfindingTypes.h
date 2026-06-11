#pragma once

#include "CoreMinimal.h"
#include "PathfindingTypes.generated.h"

USTRUCT(BlueprintType)
struct FNavCell
{
    GENERATED_BODY()

    UPROPERTY()
    uint8 Flags = 1;

    UPROPERTY()
    float Cost = 1.0f;

    bool IsWalkable() const { return (Flags & 1) != 0 && (Flags & 2) == 0; }
    bool IsStaticObstacle() const { return (Flags & 2) != 0; }
    void SetWalkable(bool bValue) { bValue ? (Flags |= 1) : (Flags &= ~1); }
    void SetStaticObstacle(bool bValue) { bValue ? (Flags |= 2) : (Flags &= ~2); }
};

USTRUCT()
struct FFlowFieldData
{
    GENERATED_BODY()

    TMap<FIntVector, float> IntegrationField;
    TMap<FIntVector, FVector> DirectionField;
    FIntVector TargetCell;
    uint32 CacheGeneration = 0;
    bool bIsValid = false;
    double LastAccessTime = 0.0; // LRU 时间戳
};

UENUM(BlueprintType)
enum class EPathfindingStatus : uint8
{
    Pending,
    InProgress,
    Completed,
    Failed,
    Cancelled
};
