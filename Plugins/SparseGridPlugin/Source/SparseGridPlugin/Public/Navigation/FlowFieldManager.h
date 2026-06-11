#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PathfindingService.h"
#include "PathfindingTypes.h"
#include "PathfindingConfig.h"
#include "NavigationGrid.h"
#include "FlowFieldManager.generated.h"

UCLASS()
class SPARSEGRIDPLUGIN_API UFlowFieldManager : public UWorldSubsystem, public IPathfindingService
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    // ─── IPathfindingService ───

    virtual bool GenerateFlowFieldSync(
        const FVector& Target,
        FFlowFieldResult& OutResult) override;

    virtual FFlowFieldRequestHandle RequestFlowFieldAsync(
        const FVector& Target,
        const FOnFlowFieldComplete::FDelegate& Callback) override;

    virtual void CancelFlowFieldRequest(const FFlowFieldRequestHandle& Handle) override;

    virtual bool GetFlowDirection(
        const FVector& Position,
        const FVector& Target,
        FVector& OutDirection) override;

    virtual bool GetPathFromFlowField(
        const FVector& Start,
        const FVector& Target,
        TArray<FVector>& OutPathPoints) override;

    virtual bool IsPointWalkable(const FVector& Point) const override;

    // ─── 障碍物管理 ───

    UFUNCTION(BlueprintCallable, Category = "FlowField")
    void MarkStaticObstacle(const FIntVector& CellCoord);

    UFUNCTION(BlueprintCallable, Category = "FlowField")
    void ClearStaticObstacle(const FIntVector& CellCoord);

    void MarkStaticObstacleCells(const TArray<FIntVector>& CellCoords);
    void ClearStaticObstacleCells(const TArray<FIntVector>& CellCoords);
    bool IsSegmentWalkable(const FVector& Start, const FVector& End, float ClearanceRadius = 0.0f) const;
    bool FindNearestWalkablePoint(const FVector& Point, float SearchRadius, FVector& OutPoint) const;

    void UpdateDynamicObstacles();
    void ProcessCompletedFlowFields();

    void ConfigureNavGrid(float InCellSize, int32 InCoarsePathLevel);

    UFUNCTION(BlueprintCallable, Category = "FlowField")
    void SetPathfindingConfig(UPathfindingConfig* InConfig);

    UFUNCTION(BlueprintCallable, Category = "FlowField")
    UNavigationGrid* GetNavigationGrid() const;

    // 将精确的 Level 0 坐标量化到缓存层级，用于 FlowField 缓存索引
    FIntVector QuantizeToCacheLevel(const FIntVector& L0Cell) const;

    // 将世界空间Range换算为Cell数量
    int32 WorldRangeToCellRange(float WorldRange) const;

private:
    struct FFlowFieldTask
    {
        FFlowFieldRequestHandle Handle;
        FVector Target;
        FOnFlowFieldComplete::FDelegate Callback;
        EPathfindingStatus Status = EPathfindingStatus::Pending;
    };

    UPROPERTY()
    UNavigationGrid* NavGrid = nullptr;

    UPROPERTY()
    UPathfindingConfig* PathConfig = nullptr;

    FCriticalSection FlowFieldLock;
    TArray<FFlowFieldTask> PendingFlowFieldTasks;
    TMap<FIntVector, int32> StaticObstacleRefCounts;
    int32 NextRequestId = 0;
    int32 CoarsePathLevel = 1;
    FIntVector GetRepresentativeTargetCell(const FIntVector& CacheKey) const;
    FIntVector ResolveWalkableTargetCell(const FIntVector& RequestedCell, int32 SearchCellRadius = 8) const;
    int32 CacheQuantizeShift = 1; // 缓存量化位移：1 = Level 1 (200x200), 2 = Level 2 (400x400)
};
