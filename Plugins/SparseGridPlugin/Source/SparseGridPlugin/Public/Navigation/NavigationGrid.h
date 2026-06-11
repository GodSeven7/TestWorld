#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PathfindingTypes.h"
#include "NavigationGrid.generated.h"

UCLASS()
class SPARSEGRIDPLUGIN_API UNavigationGrid : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(float InCellSize, int32 InMaxCoarseLevel = 3);

    FIntVector WorldToGrid(const FVector& WorldPos) const;
    FVector GridToWorld(const FIntVector& CellCoord) const;
    FVector GetCellCenter(const FIntVector& CellCoord) const;

    bool IsWalkable(const FIntVector& Cell) const;
    float GetCost(const FIntVector& Cell) const;
    FNavCell* GetNavCell(const FIntVector& Cell);
    const FNavCell* GetNavCell(const FIntVector& Cell) const;
    FNavCell* GetOrCreateNavCell(const FIntVector& Cell);

    // 粗层级可行走查询
    bool IsCoarseWalkable(const FIntVector& Cell, int32 Level) const;

    void MarkStaticBlocked(const FIntVector& Cell);
    void ClearStaticBlocked(const FIntVector& Cell);

    FFlowFieldData* GetFlowFieldCache(const FIntVector& TargetCell);
    FFlowFieldData& GetOrCreateFlowFieldCache(const FIntVector& TargetCell);
    void InvalidateFlowFieldCache(const FIntVector& TargetCell);
    void InvalidateAllFlowFieldCaches();
    void TrimFlowFieldCache(int32 MaxCount);

    float GetCellSize() const { return CellSize; }
    int32 GetNavCellCount() const { return NavCells.Num(); }
    int32 GetMaxCoarseLevel() const { return MaxCoarseLevel; }

    const TMap<FIntVector, FFlowFieldData>& GetAllFlowFieldCaches() const { return FlowFieldCaches; }
    const TMap<FIntVector, FNavCell>& GetAllNavCells() const { return NavCells; }

private:
    // L0 cell walkability 变化时，向上传播更新粗层级缓存
    void PropagateWalkability(const FIntVector& L0Cell);
    void UpdateCoarseCell(const FIntVector& CoarseCell, int32 Level);

    UPROPERTY()
    TMap<FIntVector, FNavCell> NavCells;

    UPROPERTY()
    TMap<FIntVector, FFlowFieldData> FlowFieldCaches;

    // 粗层级可行走缓存: CoarseWalkable[Level-1] = TMap<FIntVector, bool>
    // Level 1 → index 0, Level 2 → index 1, Level 3 → index 2
    TMap<FIntVector, bool> CoarseWalkable[3];

    float CellSize = 50.0f;
    float InvCellSize = 0.02f;
    int32 MaxCoarseLevel = 3;
};
