#pragma once

#include "CoreMinimal.h"
#include "PathfindingTypes.h"
#include "NavigationGrid.h"

class SPARSEGRIDPLUGIN_API FFlowFieldSolver
{
public:
    static bool Generate(
        const UNavigationGrid* NavGrid,
        const FIntVector& TargetCell,
        int32 Range,
        int32 CoarsePathLevel,
        FFlowFieldData& OutFlowField);

private:
    // 层级化粗筛：从 CoarsePathLevel 逐级降到 Level 1，返回可达的 Level 1 区域
    static void BuildCoarseReachable(
        const UNavigationGrid* NavGrid,
        const FIntVector& TargetL0,
        int32 CoarsePathLevel,
        int32 L0Range,
        TSet<FIntVector>& OutReachableL1);

    // 在指定层级做 BFS，返回可达的粗 cell 集合（有范围限制）
    static void BFSAtLevel(
        const UNavigationGrid* NavGrid,
        const FIntVector& StartCell,
        int32 Level,
        int32 RangeAtLevel,
        TSet<FIntVector>& OutReachable);

    // Level 0 IntegrationField + DirectionField 构建
    static void BuildIntegrationField(
        const UNavigationGrid* NavGrid,
        const FIntVector& TargetCell,
        int32 Range,
        const TSet<FIntVector>& ReachableL1,
        TMap<FIntVector, float>& OutIntegrationField);

    static void BuildDirectionField(
        const UNavigationGrid* NavGrid,
        const TMap<FIntVector, float>& IntegrationField,
        int32 Range,
        TMap<FIntVector, FVector>& OutDirectionField);

    static void GetNeighborOffsets(TArray<FIntVector>& OutOffsets);
};
