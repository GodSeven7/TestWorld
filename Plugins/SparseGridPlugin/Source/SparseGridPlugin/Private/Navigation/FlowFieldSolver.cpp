#include "FlowFieldSolver.h"
#include "Algo/Reverse.h"

// ─── 二叉堆优先队列，用于Dijkstra最小代价节点选取 ───

struct FFlowFieldHeapEntry
{
    float Cost;
    FIntVector Cell;

    bool operator<(const FFlowFieldHeapEntry& Other) const
    {
        return Cost < Other.Cost; // 最小堆：Cost小的在堆顶
    }
};

class FFlowFieldPriorityQueue
{
public:
    void Push(float Cost, const FIntVector& Cell)
    {
        Heap.Add({Cost, Cell});
        SiftUp(Heap.Num() - 1);
    }

    FIntVector Pop()
    {
        check(Heap.Num() > 0);
        FIntVector Result = Heap[0].Cell;
        Heap[0] = Heap.Last();
        Heap.Pop(false);
        if (Heap.Num() > 0)
        {
            SiftDown(0);
        }
        return Result;
    }

    bool IsEmpty() const { return Heap.Num() == 0; }

    int32 Num() const { return Heap.Num(); }

    void Reserve(int32 Count) { Heap.Reserve(Count); }

private:
    TArray<FFlowFieldHeapEntry> Heap;

    void SiftUp(int32 Index)
    {
        while (Index > 0)
        {
            int32 Parent = (Index - 1) / 2;
            if (Heap[Index].Cost < Heap[Parent].Cost)
            {
                Swap(Heap[Index], Heap[Parent]);
                Index = Parent;
            }
            else
            {
                break;
            }
        }
    }

    void SiftDown(int32 Index)
    {
        int32 Count = Heap.Num();
        while (true)
        {
            int32 Left = 2 * Index + 1;
            int32 Right = 2 * Index + 2;
            int32 Smallest = Index;

            if (Left < Count && Heap[Left].Cost < Heap[Smallest].Cost)
                Smallest = Left;
            if (Right < Count && Heap[Right].Cost < Heap[Smallest].Cost)
                Smallest = Right;

            if (Smallest != Index)
            {
                Swap(Heap[Index], Heap[Smallest]);
                Index = Smallest;
            }
            else
            {
                break;
            }
        }
    }
};

// ─── FFlowFieldSolver ───

bool FFlowFieldSolver::Generate(
    const UNavigationGrid* NavGrid,
    const FIntVector& TargetCell,
    int32 Range,
    int32 CoarsePathLevel,
    FFlowFieldData& OutFlowField)
{
    if (!NavGrid)
        return false;

    OutFlowField.TargetCell = TargetCell;
    OutFlowField.IntegrationField.Empty();
    OutFlowField.DirectionField.Empty();

    // 层级化粗筛：从 CoarsePathLevel 逐级降到 Level 1
    TSet<FIntVector> ReachableL1;
    if (CoarsePathLevel > 0)
    {
        BuildCoarseReachable(NavGrid, TargetCell, CoarsePathLevel, Range, ReachableL1);
        if (ReachableL1.Num() == 0)
            return false;
    }

    // Level 0 IntegrationField + DirectionField
    BuildIntegrationField(NavGrid, TargetCell, Range, ReachableL1, OutFlowField.IntegrationField);

    if (OutFlowField.IntegrationField.Num() == 0)
        return false;

    BuildDirectionField(NavGrid, OutFlowField.IntegrationField, Range, OutFlowField.DirectionField);

    OutFlowField.bIsValid = true;
    OutFlowField.CacheGeneration++;
    return true;
}

void FFlowFieldSolver::BuildCoarseReachable(
    const UNavigationGrid* NavGrid,
    const FIntVector& TargetL0,
    int32 CoarsePathLevel,
    int32 L0Range,
    TSet<FIntVector>& OutReachableL1)
{
    TSet<FIntVector> CurrentReachable;

    // Phase 1: CoarsePathLevel BFS
    {
        FIntVector StartCell(TargetL0.X >> CoarsePathLevel, TargetL0.Y >> CoarsePathLevel, 0);
        int32 RangeAtLevel = (L0Range >> CoarsePathLevel) + 1;
        BFSAtLevel(NavGrid, StartCell, CoarsePathLevel, RangeAtLevel, CurrentReachable);
        if (CurrentReachable.Num() == 0)
            return;
    }

    // Phase 2: 逐级下降
    for (int32 Level = CoarsePathLevel - 1; Level >= 1; --Level)
    {
        int32 RangeAtLevel = (L0Range >> Level) + 1;

        TSet<FIntVector> NextReachable;

        FIntVector TargetAtLevel(TargetL0.X >> Level, TargetL0.Y >> Level, 0);

        for (const FIntVector& CoarseCell : CurrentReachable)
        {
            for (int32 DX = 0; DX < 2; ++DX)
            {
                for (int32 DY = 0; DY < 2; ++DY)
                {
                    FIntVector Child(CoarseCell.X * 2 + DX, CoarseCell.Y * 2 + DY, 0);

                    if (FMath::Abs(Child.X - TargetAtLevel.X) > RangeAtLevel ||
                        FMath::Abs(Child.Y - TargetAtLevel.Y) > RangeAtLevel)
                        continue;

                    if (NavGrid->IsCoarseWalkable(Child, Level))
                    {
                        NextReachable.Add(Child);
                    }
                }
            }
        }

        // BFS 从 TargetAtLevel 开始，只在 NextReachable 范围内展开
        TSet<FIntVector> BFSReachable;
        TArray<FIntVector> OpenSet;
        OpenSet.Add(TargetAtLevel);

        while (OpenSet.Num() > 0)
        {
            FIntVector Current = OpenSet.Pop();

            if (BFSReachable.Contains(Current))
                continue;

            if (!NextReachable.Contains(Current))
                continue;

            BFSReachable.Add(Current);

            static const FIntVector Offsets[] = {
                FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
                FIntVector(0, 1, 0), FIntVector(0, -1, 0)
            };
            for (const FIntVector& Offset : Offsets)
            {
                FIntVector Neighbor(Current.X + Offset.X, Current.Y + Offset.Y, 0);
                if (!BFSReachable.Contains(Neighbor) && NextReachable.Contains(Neighbor))
                {
                    OpenSet.Add(Neighbor);
                }
            }
        }

        CurrentReachable = MoveTemp(BFSReachable);

        if (CurrentReachable.Num() == 0)
            return;
    }

    OutReachableL1 = MoveTemp(CurrentReachable);
}

void FFlowFieldSolver::BFSAtLevel(
    const UNavigationGrid* NavGrid,
    const FIntVector& StartCell,
    int32 Level,
    int32 RangeAtLevel,
    TSet<FIntVector>& OutReachable)
{
    TSet<FIntVector> Visited;
    TArray<FIntVector> OpenSet;
    OpenSet.Add(StartCell);

    while (OpenSet.Num() > 0)
    {
        FIntVector Current = OpenSet.Pop();

        if (Visited.Contains(Current))
            continue;

        if (FMath::Abs(Current.X - StartCell.X) > RangeAtLevel ||
            FMath::Abs(Current.Y - StartCell.Y) > RangeAtLevel)
            continue;

        if (!NavGrid->IsCoarseWalkable(Current, Level))
            continue;

        Visited.Add(Current);

        static const FIntVector Offsets[] = {
            FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
            FIntVector(0, 1, 0), FIntVector(0, -1, 0)
        };
        for (const FIntVector& Offset : Offsets)
        {
            FIntVector Neighbor(Current.X + Offset.X, Current.Y + Offset.Y, 0);
            if (!Visited.Contains(Neighbor))
            {
                OpenSet.Add(Neighbor);
            }
        }
    }

    OutReachable = MoveTemp(Visited);
}

void FFlowFieldSolver::BuildIntegrationField(
    const UNavigationGrid* NavGrid,
    const FIntVector& TargetCell,
    int32 Range,
    const TSet<FIntVector>& ReachableL1,
    TMap<FIntVector, float>& OutIntegrationField)
{
    TArray<FIntVector> NeighborOffsets;
    GetNeighborOffsets(NeighborOffsets);

    int32 CellCount = (Range * 2 + 1) * (Range * 2 + 1);
    OutIntegrationField.Reserve(CellCount);

    // 初始化：Range 范围内的 L0 cell
    for (int32 DX = -Range; DX <= Range; ++DX)
    {
        for (int32 DY = -Range; DY <= Range; ++DY)
        {
            FIntVector Cell(TargetCell.X + DX, TargetCell.Y + DY, 0);

            // 粗筛：如果 CoarsePathLevel > 0，检查此 cell 的 L1 父节点是否可达
            if (ReachableL1.Num() > 0)
            {
                FIntVector L1Parent(Cell.X >> 1, Cell.Y >> 1, 0);
                if (!ReachableL1.Contains(L1Parent))
                    continue;
            }

            OutIntegrationField.Add(Cell, FLT_MAX);
        }
    }

    OutIntegrationField.Add(TargetCell, 0.0f);

    // 使用二叉堆优先队列的Dijkstra，O(N log N)
    FFlowFieldPriorityQueue OpenSet;
    OpenSet.Reserve(256);
    OpenSet.Push(0.0f, TargetCell);

    // ClosedSet：已确定最短路径的节点，避免重复处理
    TSet<FIntVector> ClosedSet;
    ClosedSet.Reserve(CellCount);

    int32 Iterations = 0;
    const int32 MaxIterations = CellCount * 2;

    while (!OpenSet.IsEmpty() && Iterations < MaxIterations)
    {
        Iterations++;

        FIntVector Current = OpenSet.Pop();

        // 跳过已处理的节点
        if (ClosedSet.Contains(Current))
            continue;
        ClosedSet.Add(Current);

        float CurrentCost = OutIntegrationField.FindRef(Current, FLT_MAX);
        if (CurrentCost >= FLT_MAX)
            continue;

        for (const FIntVector& Offset : NeighborOffsets)
        {
            FIntVector Neighbor(Current.X + Offset.X, Current.Y + Offset.Y, 0);

            if (FMath::Abs(Neighbor.X - TargetCell.X) > Range ||
                FMath::Abs(Neighbor.Y - TargetCell.Y) > Range)
                continue;

            float* NeighborCost = OutIntegrationField.Find(Neighbor);
            if (!NeighborCost)
                continue;

            // 已确定最短路径的节点不再更新
            if (ClosedSet.Contains(Neighbor))
                continue;

            if (!NavGrid->IsWalkable(Neighbor))
                continue;

            bool bIsDiagonal = (Offset.X != 0 && Offset.Y != 0);
            float MoveCost = NavGrid->GetCost(Neighbor) * (bIsDiagonal ? 1.4142f : 1.0f);
            float NewCost = CurrentCost + MoveCost;

            if (NewCost < *NeighborCost)
            {
                *NeighborCost = NewCost;
                OpenSet.Push(NewCost, Neighbor);
            }
        }
    }
}

void FFlowFieldSolver::BuildDirectionField(
    const UNavigationGrid* NavGrid,
    const TMap<FIntVector, float>& IntegrationField,
    int32 Range,
    TMap<FIntVector, FVector>& OutDirectionField)
{
    TArray<FIntVector> NeighborOffsets;
    GetNeighborOffsets(NeighborOffsets);

    OutDirectionField.Reserve(IntegrationField.Num());

    for (const auto& Pair : IntegrationField)
    {
        const FIntVector& Cell = Pair.Key;
        float CellCost = Pair.Value;

        if (CellCost >= FLT_MAX || CellCost <= 0.0f)
        {
            OutDirectionField.Add(Cell, FVector::ZeroVector);
            continue;
        }

        float BestCost = CellCost;
        FIntVector BestNeighbor = Cell;

        for (const FIntVector& Offset : NeighborOffsets)
        {
            FIntVector Neighbor(Cell.X + Offset.X, Cell.Y + Offset.Y, 0);
            const float* NeighborCost = IntegrationField.Find(Neighbor);
            if (!NeighborCost || *NeighborCost >= FLT_MAX)
                continue;

            if (*NeighborCost < BestCost)
            {
                BestCost = *NeighborCost;
                BestNeighbor = Neighbor;
            }
        }

        if (BestNeighbor != Cell)
        {
            FVector From(static_cast<float>(Cell.X), static_cast<float>(Cell.Y), 0.0f);
            FVector To(static_cast<float>(BestNeighbor.X), static_cast<float>(BestNeighbor.Y), 0.0f);
            FVector Dir = (To - From).GetSafeNormal();
            OutDirectionField.Add(Cell, Dir);
        }
        else
        {
            OutDirectionField.Add(Cell, FVector::ZeroVector);
        }
    }
}

void FFlowFieldSolver::GetNeighborOffsets(TArray<FIntVector>& OutOffsets)
{
    OutOffsets = {
        FIntVector(1, 0, 0),
        FIntVector(-1, 0, 0),
        FIntVector(0, 1, 0),
        FIntVector(0, -1, 0),
        FIntVector(1, 1, 0),
        FIntVector(1, -1, 0),
        FIntVector(-1, 1, 0),
        FIntVector(-1, -1, 0)
    };
}
