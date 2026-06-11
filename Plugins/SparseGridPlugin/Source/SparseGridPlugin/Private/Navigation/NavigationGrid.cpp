#include "NavigationGrid.h"

void UNavigationGrid::Initialize(float InCellSize, int32 InMaxCoarseLevel)
{
    CellSize = InCellSize;
    InvCellSize = 1.0f / CellSize;
    MaxCoarseLevel = FMath::Clamp(InMaxCoarseLevel, 1, 3);
}

FIntVector UNavigationGrid::WorldToGrid(const FVector& WorldPos) const
{
    return FIntVector(
        FMath::FloorToInt(WorldPos.X * InvCellSize),
        FMath::FloorToInt(WorldPos.Y * InvCellSize),
        0
    );
}

FVector UNavigationGrid::GridToWorld(const FIntVector& CellCoord) const
{
    return FVector(
        static_cast<float>(CellCoord.X) * CellSize,
        static_cast<float>(CellCoord.Y) * CellSize,
        0.0f
    );
}

FVector UNavigationGrid::GetCellCenter(const FIntVector& CellCoord) const
{
    return GridToWorld(CellCoord) + FVector(CellSize * 0.5f, CellSize * 0.5f, 0.0f);
}

bool UNavigationGrid::IsWalkable(const FIntVector& Cell) const
{
    if (const FNavCell* NavCell = GetNavCell(Cell))
    {
        return NavCell->IsWalkable();
    }
    return true;
}

float UNavigationGrid::GetCost(const FIntVector& Cell) const
{
    if (const FNavCell* NavCell = GetNavCell(Cell))
    {
        return NavCell->Cost;
    }
    return 1.0f;
}

FNavCell* UNavigationGrid::GetNavCell(const FIntVector& Cell)
{
    return NavCells.Find(Cell);
}

const FNavCell* UNavigationGrid::GetNavCell(const FIntVector& Cell) const
{
    return NavCells.Find(Cell);
}

FNavCell* UNavigationGrid::GetOrCreateNavCell(const FIntVector& Cell)
{
    return &NavCells.FindOrAdd(Cell);
}

bool UNavigationGrid::IsCoarseWalkable(const FIntVector& Cell, int32 Level) const
{
    if (Level <= 0)
        return IsWalkable(Cell);

    int32 Index = Level - 1;
    if (Index < 0 || Index >= 3)
        return false;

    const bool* Found = CoarseWalkable[Index].Find(Cell);
    // 未记录的cell默认可行走
    return Found ? *Found : true;
}

void UNavigationGrid::MarkStaticBlocked(const FIntVector& Cell)
{
    FNavCell& NavCell = NavCells.FindOrAdd(Cell);
    NavCell.SetStaticObstacle(true);
    NavCell.SetWalkable(false);
    PropagateWalkability(Cell);
}

void UNavigationGrid::ClearStaticBlocked(const FIntVector& Cell)
{
    if (FNavCell* NavCell = NavCells.Find(Cell))
    {
        NavCell->SetStaticObstacle(false);
        NavCell->SetWalkable(true);
        PropagateWalkability(Cell);
    }
}

void UNavigationGrid::PropagateWalkability(const FIntVector& L0Cell)
{
    // 从 Level 1 向上逐级传播
    for (int32 Level = 1; Level <= MaxCoarseLevel; ++Level)
    {
        FIntVector CoarseCell(L0Cell.X >> Level, L0Cell.Y >> Level, 0);
        UpdateCoarseCell(CoarseCell, Level);
    }
}

void UNavigationGrid::UpdateCoarseCell(const FIntVector& CoarseCell, int32 Level)
{
    int32 Index = Level - 1;
    if (Index < 0 || Index >= 3)
        return;

    // 检查此粗cell下的4个子cell（来自下一层）
    bool bAnyWalkable = false;

    if (Level == 1)
    {
        // Level 1 的子是 Level 0 NavCells
        for (int32 DX = 0; DX < 2; ++DX)
        {
            for (int32 DY = 0; DY < 2; ++DY)
            {
                FIntVector Child(CoarseCell.X * 2 + DX, CoarseCell.Y * 2 + DY, 0);
                if (IsWalkable(Child))
                {
                    bAnyWalkable = true;
                    break;
                }
            }
            if (bAnyWalkable) break;
        }
    }
    else
    {
        // Level N 的子是 Level N-1 CoarseWalkable
        int32 ChildIndex = Level - 2;
        if (ChildIndex < 0 || ChildIndex >= 3)
            return;

        for (int32 DX = 0; DX < 2; ++DX)
        {
            for (int32 DY = 0; DY < 2; ++DY)
            {
                FIntVector Child(CoarseCell.X * 2 + DX, CoarseCell.Y * 2 + DY, 0);
                const bool* ChildWalkable = CoarseWalkable[ChildIndex].Find(Child);
                if (!ChildWalkable || *ChildWalkable)
                {
                    bAnyWalkable = true;
                    break;
                }
            }
            if (bAnyWalkable) break;
        }
    }

    CoarseWalkable[Index].Add(CoarseCell, bAnyWalkable);
}

FFlowFieldData* UNavigationGrid::GetFlowFieldCache(const FIntVector& TargetCell)
{
    FFlowFieldData* Cache = FlowFieldCaches.Find(TargetCell);
    if (Cache)
    {
        Cache->LastAccessTime = FPlatformTime::Seconds();
    }
    return Cache;
}

FFlowFieldData& UNavigationGrid::GetOrCreateFlowFieldCache(const FIntVector& TargetCell)
{
    FFlowFieldData& Cache = FlowFieldCaches.FindOrAdd(TargetCell);
    Cache.LastAccessTime = FPlatformTime::Seconds();
    return Cache;
}

void UNavigationGrid::InvalidateFlowFieldCache(const FIntVector& TargetCell)
{
    if (FFlowFieldData* Cache = FlowFieldCaches.Find(TargetCell))
    {
        Cache->bIsValid = false;
    }
}

void UNavigationGrid::InvalidateAllFlowFieldCaches()
{
    for (auto& Pair : FlowFieldCaches)
    {
        Pair.Value.bIsValid = false;
    }
}

void UNavigationGrid::TrimFlowFieldCache(int32 MaxCount)
{
    if (FlowFieldCaches.Num() <= MaxCount)
        return;

    // LRU: 按 LastAccessTime 排序，删除最久未使用的
    TArray<FIntVector> Keys;
    FlowFieldCaches.GetKeys(Keys);

    Keys.Sort([this](const FIntVector& A, const FIntVector& B)
    {
        const FFlowFieldData* CacheA = FlowFieldCaches.Find(A);
        const FFlowFieldData* CacheB = FlowFieldCaches.Find(B);
        if (!CacheA || !CacheB) return false;
        return CacheA->LastAccessTime > CacheB->LastAccessTime; // 降序：最近访问在前
    });

    int32 RemoveCount = FlowFieldCaches.Num() - MaxCount;
    for (int32 i = Keys.Num() - 1; i >= 0 && RemoveCount > 0; --i, --RemoveCount)
    {
        FlowFieldCaches.Remove(Keys[i]);
    }
}
