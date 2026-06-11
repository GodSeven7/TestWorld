#include "FlowFieldManager.h"
#include "SparseGridManager.h"
#include "SparseGridObject.h"
#include "FlowFieldSolver.h"
#include "PathSmoother.h"
#include "Engine/World.h"
#include "Async/Async.h"

void UFlowFieldManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    NavGrid = NewObject<UNavigationGrid>(this);
    // 使用默认CellSize初始化，SetSparseGridConfig时会通过ConfigureNavGrid覆盖为BaseRadius
    NavGrid->Initialize(50.0f);

    UE_LOG(LogTemp, Log, TEXT("FlowFieldManager initialized"));
}

void UFlowFieldManager::Deinitialize()
{
    Super::Deinitialize();

    NavGrid = nullptr;
    PendingFlowFieldTasks.Empty();
    StaticObstacleRefCounts.Empty();

    UE_LOG(LogTemp, Log, TEXT("FlowFieldManager deinitialized"));
}

bool UFlowFieldManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UFlowFieldManager::SetPathfindingConfig(UPathfindingConfig* InConfig)
{
    PathConfig = InConfig;
}

void UFlowFieldManager::ConfigureNavGrid(float InCellSize, int32 InCoarsePathLevel)
{
    CoarsePathLevel = InCoarsePathLevel;
    // 缓存量化层级 = CoarsePathLevel，目标在同一粗层级Cell内时复用缓存
    CacheQuantizeShift = CoarsePathLevel;
    if (NavGrid)
    {
        NavGrid->Initialize(InCellSize, CoarsePathLevel);
        UE_LOG(LogTemp, Log, TEXT("FlowFieldManager NavGrid configured: CellSize=%.2f, CoarsePathLevel=%d, CacheQuantizeShift=%d"),
            InCellSize, CoarsePathLevel, CacheQuantizeShift);
    }
}

UNavigationGrid* UFlowFieldManager::GetNavigationGrid() const
{
    return NavGrid;
}

FIntVector UFlowFieldManager::QuantizeToCacheLevel(const FIntVector& L0Cell) const
{
    // 将 Level 0 坐标量化到缓存层级
    int32 Shift = FMath::Max(0, CacheQuantizeShift);
    if (Shift <= 0)
    {
        return FIntVector(L0Cell.X, L0Cell.Y, 0);
    }

    const int32 CellSpan = 1 << Shift;
    return FIntVector(
        FMath::FloorToInt(static_cast<float>(L0Cell.X) / static_cast<float>(CellSpan)),
        FMath::FloorToInt(static_cast<float>(L0Cell.Y) / static_cast<float>(CellSpan)),
        0);
}

FIntVector UFlowFieldManager::GetRepresentativeTargetCell(const FIntVector& CacheKey) const
{
    const int32 Shift = FMath::Max(0, CacheQuantizeShift);
    if (Shift <= 0)
    {
        return CacheKey;
    }

    const int32 CellSpan = 1 << Shift;
    const int32 HalfExtent = 1 << (Shift - 1);
    return FIntVector(CacheKey.X * CellSpan + HalfExtent, CacheKey.Y * CellSpan + HalfExtent, 0);
}

int32 UFlowFieldManager::WorldRangeToCellRange(float WorldRange) const
{
    if (!NavGrid || NavGrid->GetCellSize() <= 0.0f)
        return 50;
    return FMath::CeilToInt(WorldRange / NavGrid->GetCellSize());
}

FIntVector UFlowFieldManager::ResolveWalkableTargetCell(const FIntVector& RequestedCell, int32 SearchCellRadius) const
{
    if (!NavGrid || NavGrid->IsWalkable(RequestedCell))
    {
        return RequestedCell;
    }

    SearchCellRadius = FMath::Max(0, SearchCellRadius);

    FIntVector BestCell = RequestedCell;
    int32 BestDistanceSq = TNumericLimits<int32>::Max();

    for (int32 Radius = 1; Radius <= SearchCellRadius; ++Radius)
    {
        for (int32 DX = -Radius; DX <= Radius; ++DX)
        {
            for (int32 DY = -Radius; DY <= Radius; ++DY)
            {
                if (FMath::Abs(DX) != Radius && FMath::Abs(DY) != Radius)
                {
                    continue;
                }

                const FIntVector Candidate(RequestedCell.X + DX, RequestedCell.Y + DY, 0);
                if (!NavGrid->IsWalkable(Candidate))
                {
                    continue;
                }

                const int32 DistanceSq = DX * DX + DY * DY;
                if (DistanceSq < BestDistanceSq)
                {
                    BestDistanceSq = DistanceSq;
                    BestCell = Candidate;
                }
            }
        }

        if (BestDistanceSq < TNumericLimits<int32>::Max())
        {
            return BestCell;
        }
    }

    return RequestedCell;
}

void UFlowFieldManager::MarkStaticObstacle(const FIntVector& CellCoord)
{
    TArray<FIntVector> Cells;
    Cells.Add(CellCoord);
    MarkStaticObstacleCells(Cells);
}

void UFlowFieldManager::ClearStaticObstacle(const FIntVector& CellCoord)
{
    TArray<FIntVector> Cells;
    Cells.Add(CellCoord);
    ClearStaticObstacleCells(Cells);
}

void UFlowFieldManager::MarkStaticObstacleCells(const TArray<FIntVector>& CellCoords)
{
    if (!NavGrid || CellCoords.Num() == 0)
    {
        return;
    }

    bool bChanged = false;
    for (const FIntVector& CellCoord : CellCoords)
    {
        int32& RefCount = StaticObstacleRefCounts.FindOrAdd(CellCoord);
        ++RefCount;
        if (RefCount == 1)
        {
            NavGrid->MarkStaticBlocked(CellCoord);
            bChanged = true;
        }
    }

    if (bChanged)
    {
        NavGrid->InvalidateAllFlowFieldCaches();
    }
}

void UFlowFieldManager::ClearStaticObstacleCells(const TArray<FIntVector>& CellCoords)
{
    if (!NavGrid || CellCoords.Num() == 0)
    {
        return;
    }

    bool bChanged = false;
    for (const FIntVector& CellCoord : CellCoords)
    {
        int32* RefCountPtr = StaticObstacleRefCounts.Find(CellCoord);
        if (!RefCountPtr)
        {
            continue;
        }

        --(*RefCountPtr);
        if (*RefCountPtr <= 0)
        {
            StaticObstacleRefCounts.Remove(CellCoord);
            NavGrid->ClearStaticBlocked(CellCoord);
            bChanged = true;
        }
    }

    if (bChanged)
    {
        NavGrid->InvalidateAllFlowFieldCaches();
    }
}

bool UFlowFieldManager::GenerateFlowFieldSync(
    const FVector& Target,
    FFlowFieldResult& OutResult)
{
    if (!NavGrid)
        return false;

    USparseGridManager* GridManager = GetWorld()->GetSubsystem<USparseGridManager>();
    if (!GridManager || !GridManager->GetSparseGrid())
        return false;

    float WorldRange = PathConfig ? PathConfig->FlowFieldWorldRange : 5000.0f;
    int32 Range = WorldRangeToCellRange(WorldRange);
    const FIntVector RequestedTargetCell = ResolveWalkableTargetCell(NavGrid->WorldToGrid(Target));

    // 用量化后的坐标作为缓存key，目标在小范围内移动时复用缓存
    const FIntVector CacheKey = QuantizeToCacheLevel(RequestedTargetCell);
    const FIntVector TargetCell = ResolveWalkableTargetCell(GetRepresentativeTargetCell(CacheKey));

    FFlowFieldData* CachedField = NavGrid->GetFlowFieldCache(CacheKey);
    if (CachedField && CachedField->bIsValid && CachedField->TargetCell == TargetCell)
    {
        OutResult.bSuccess = true;
        OutResult.TotalCost = 0.0f;
        return true;
    }

    // The cached field is generated toward this cache key's representative target cell.
    FFlowFieldData& FlowField = NavGrid->GetOrCreateFlowFieldCache(CacheKey);
    bool bSuccess = FFlowFieldSolver::Generate(NavGrid, TargetCell, Range, CoarsePathLevel, FlowField);

    if (bSuccess)
    {
        int32 MaxCache = PathConfig ? PathConfig->MaxFlowFieldCacheCount : 8;
        NavGrid->TrimFlowFieldCache(MaxCache);
        OutResult.bSuccess = true;
    }

    return bSuccess;
}

FFlowFieldRequestHandle UFlowFieldManager::RequestFlowFieldAsync(
    const FVector& Target,
    const FOnFlowFieldComplete::FDelegate& Callback)
{
    FFlowFieldRequestHandle Handle;
    Handle.RequestId = -1;

    if (!NavGrid)
        return Handle;

    USparseGridManager* GridManager = GetWorld()->GetSubsystem<USparseGridManager>();
    if (!GridManager || !GridManager->GetSparseGrid())
        return Handle;

    const FIntVector RequestedTargetCell = ResolveWalkableTargetCell(NavGrid->WorldToGrid(Target));

    // 用量化后的坐标作为缓存key
    const FIntVector CacheKey = QuantizeToCacheLevel(RequestedTargetCell);
    const FIntVector TargetCell = ResolveWalkableTargetCell(GetRepresentativeTargetCell(CacheKey));

    FFlowFieldData* CachedField = NavGrid->GetFlowFieldCache(CacheKey);
    if (CachedField && CachedField->bIsValid && CachedField->TargetCell == TargetCell)
    {
        FFlowFieldResult Result;
        Result.bSuccess = true;
        Callback.ExecuteIfBound(FFlowFieldRequestHandle(), Result);
        Handle.RequestId = 0;
        return Handle;
    }

    {
        FScopeLock Lock(&FlowFieldLock);
        Handle.RequestId = NextRequestId++;

        FFlowFieldTask Task;
        Task.Handle = Handle;
        Task.Target = Target;
        Task.Callback = Callback;
        Task.Status = EPathfindingStatus::Pending;
        PendingFlowFieldTasks.Add(Task);
    }

    float WorldRange = PathConfig ? PathConfig->FlowFieldWorldRange : 5000.0f;
    int32 Range = WorldRangeToCellRange(WorldRange);
    UNavigationGrid* NavGridRef = NavGrid;
    int32 CPL = CoarsePathLevel;

    Async(EAsyncExecution::Thread, [this, Target, TargetCell, CacheKey, Range, CPL, NavGridRef, Handle]()
    {
        FFlowFieldData TempFlowField;
        bool bSuccess = FFlowFieldSolver::Generate(NavGridRef, TargetCell, Range, CPL, TempFlowField);

        FFlowFieldResult Result;
        Result.bSuccess = bSuccess;

        if (bSuccess)
        {
            FScopeLock Lock(&FlowFieldLock);
            FFlowFieldData& CachedField = NavGridRef->GetOrCreateFlowFieldCache(CacheKey);
            CachedField = MoveTemp(TempFlowField);

            int32 MaxCache = PathConfig ? PathConfig->MaxFlowFieldCacheCount : 8;
            NavGridRef->TrimFlowFieldCache(MaxCache);
        }

        FScopeLock Lock(&FlowFieldLock);
        for (FFlowFieldTask& Task : PendingFlowFieldTasks)
        {
            if (Task.Handle.RequestId == Handle.RequestId)
            {
                Task.Status = bSuccess ? EPathfindingStatus::Completed : EPathfindingStatus::Failed;
                break;
            }
        }
    });

    return Handle;
}

void UFlowFieldManager::CancelFlowFieldRequest(const FFlowFieldRequestHandle& Handle)
{
    if (!Handle.IsValid())
        return;

    FScopeLock Lock(&FlowFieldLock);
    for (int32 i = PendingFlowFieldTasks.Num() - 1; i >= 0; --i)
    {
        if (PendingFlowFieldTasks[i].Handle.RequestId == Handle.RequestId)
        {
            PendingFlowFieldTasks[i].Status = EPathfindingStatus::Cancelled;
            PendingFlowFieldTasks.RemoveAtSwap(i);
            break;
        }
    }
}

bool UFlowFieldManager::GetFlowDirection(
    const FVector& Position,
    const FVector& Target,
    FVector& OutDirection)
{
    if (!NavGrid)
        return false;

    FIntVector PosCell = NavGrid->WorldToGrid(Position);
    FIntVector RequestedTargetCell = ResolveWalkableTargetCell(NavGrid->WorldToGrid(Target));

    // 用量化后的坐标查找缓存
    FIntVector CacheKey = QuantizeToCacheLevel(RequestedTargetCell);

    FFlowFieldData* FlowField = NavGrid->GetFlowFieldCache(CacheKey);
    if (!FlowField || !FlowField->bIsValid)
        return false;

    if (FlowField->TargetCell != ResolveWalkableTargetCell(GetRepresentativeTargetCell(CacheKey)))
        return false;

    // 双线性插值：采样4个Cell的方向，根据AI在Cell内的位置加权平均
    // 这样AI在Cell边界附近不会发生方向突变，转弯时方向平滑过渡
    float CellSize = NavGrid->GetCellSize();
    FVector CellCenter = NavGrid->GetCellCenter(PosCell);

    // AI在Cell内的局部偏移 [0, 1]
    float Fx = FMath::Clamp((Position.X - CellCenter.X) / CellSize + 0.5f, 0.0f, 1.0f);
    float Fy = FMath::Clamp((Position.Y - CellCenter.Y) / CellSize + 0.5f, 0.0f, 1.0f);

    // 4个采样Cell：左下、右下、左上、右上
    FIntVector Cell00 = PosCell;
    FIntVector Cell10(PosCell.X + 1, PosCell.Y, 0);
    FIntVector Cell01(PosCell.X, PosCell.Y + 1, 0);
    FIntVector Cell11(PosCell.X + 1, PosCell.Y + 1, 0);

    // 采样方向，无效Cell用零向量（不参与插值）
    FVector Dir00 = FVector::ZeroVector;
    FVector Dir10 = FVector::ZeroVector;
    FVector Dir01 = FVector::ZeroVector;
    FVector Dir11 = FVector::ZeroVector;
    float W00 = 0.f, W10 = 0.f, W01 = 0.f, W11 = 0.f;

    auto SampleDir = [&](const FIntVector& Cell, FVector& OutDir) -> bool {
        const FVector* Dir = FlowField->DirectionField.Find(Cell);
        if (Dir && Dir->SizeSquared() > KINDA_SMALL_NUMBER)
        {
            OutDir = *Dir;
            return true;
        }
        // Cell为障碍物或方向为零，尝试从8邻域找最低代价方向
        static const FIntVector Offsets[] = {
            FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
            FIntVector(0, 1, 0), FIntVector(0, -1, 0),
            FIntVector(1, 1, 0), FIntVector(1, -1, 0),
            FIntVector(-1, 1, 0), FIntVector(-1, -1, 0)
        };
        float BestCost = FLT_MAX;
        FVector BestDir = FVector::ZeroVector;
        for (const FIntVector& Offset : Offsets)
        {
            FIntVector Neighbor(Cell.X + Offset.X, Cell.Y + Offset.Y, 0);
            const FVector* NeighborDir = FlowField->DirectionField.Find(Neighbor);
            if (!NeighborDir || NeighborDir->SizeSquared() < KINDA_SMALL_NUMBER)
                continue;
            const float* NeighborCost = FlowField->IntegrationField.Find(Neighbor);
            if (!NeighborCost || *NeighborCost >= FLT_MAX)
                continue;
            if (*NeighborCost < BestCost)
            {
                BestCost = *NeighborCost;
                BestDir = *NeighborDir;
            }
        }
        if (BestDir.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            OutDir = BestDir;
            return true;
        }
        return false;
    };

    if (SampleDir(Cell00, Dir00)) W00 = (1.0f - Fx) * (1.0f - Fy);
    if (SampleDir(Cell10, Dir10)) W10 = Fx * (1.0f - Fy);
    if (SampleDir(Cell01, Dir01)) W01 = (1.0f - Fx) * Fy;
    if (SampleDir(Cell11, Dir11)) W11 = Fx * Fy;

    float TotalWeight = W00 + W10 + W01 + W11;
    if (TotalWeight < KINDA_SMALL_NUMBER)
        return false;

    FVector InterpolatedDir = (Dir00 * W00 + Dir10 * W10 + Dir01 * W01 + Dir11 * W11) / TotalWeight;

    if (InterpolatedDir.SizeSquared() > KINDA_SMALL_NUMBER)
    {
        OutDirection = InterpolatedDir.GetSafeNormal2D();
        return true;
    }

    return false;
}

bool UFlowFieldManager::GetPathFromFlowField(
    const FVector& Start,
    const FVector& Target,
    TArray<FVector>& OutPathPoints)
{
    if (!NavGrid)
        return false;

    FIntVector StartCell = NavGrid->WorldToGrid(Start);
    FIntVector RequestedTargetCell = ResolveWalkableTargetCell(NavGrid->WorldToGrid(Target));

    // 用量化后的坐标查找缓存
    FIntVector CacheKey = QuantizeToCacheLevel(RequestedTargetCell);

    FFlowFieldData* FlowField = NavGrid->GetFlowFieldCache(CacheKey);
    if (!FlowField || !FlowField->bIsValid)
        return false;

    if (FlowField->TargetCell != ResolveWalkableTargetCell(GetRepresentativeTargetCell(CacheKey)))
        return false;

    const FIntVector TargetCell = FlowField->TargetCell;

    TArray<FIntVector> CellPath;
    CellPath.Add(StartCell);

    FIntVector Current = StartCell;
    // MaxSteps基于世界范围换算，确保足够追踪完整路径
    float WorldRange = PathConfig ? PathConfig->FlowFieldWorldRange : 5000.0f;
    int32 MaxSteps = WorldRangeToCellRange(WorldRange) * 4;
    int32 Steps = 0;

    static const FIntVector NeighborOffsets[] = {
        FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
        FIntVector(0, 1, 0), FIntVector(0, -1, 0),
        FIntVector(1, 1, 0), FIntVector(1, -1, 0),
        FIntVector(-1, 1, 0), FIntVector(-1, -1, 0)
    };

    while (Current != TargetCell && Steps < MaxSteps)
    {
        Steps++;

        // 用 IntegrationField 找代价最低的邻居，比从 DirectionVector 反推更精确
        float CurrentCost = FLT_MAX;
        const float* CostPtr = FlowField->IntegrationField.Find(Current);
        if (CostPtr)
            CurrentCost = *CostPtr;

        if (CurrentCost >= FLT_MAX || CurrentCost <= 0.0f)
            break;

        float BestNeighborCost = FLT_MAX;
        FIntVector BestNeighbor = Current;

        for (const FIntVector& Offset : NeighborOffsets)
        {
            FIntVector Neighbor(Current.X + Offset.X, Current.Y + Offset.Y, 0);
            const float* NeighborCost = FlowField->IntegrationField.Find(Neighbor);
            if (!NeighborCost || *NeighborCost >= FLT_MAX)
                continue;

            if (*NeighborCost < BestNeighborCost)
            {
                BestNeighborCost = *NeighborCost;
                BestNeighbor = Neighbor;
            }
        }

        // 没有更优邻居（已到目标或死路）
        if (BestNeighbor == Current || BestNeighborCost >= CurrentCost)
            break;

        CellPath.Add(BestNeighbor);
        Current = BestNeighbor;
    }

    if (Current != TargetCell)
    {
        CellPath.Add(TargetCell);
    }

    bool bSmooth = PathConfig ? PathConfig->bEnablePathSmoothing : true;
    if (bSmooth && CellPath.Num() > 2)
    {
        FPathSmoother::SmoothPath(NavGrid, CellPath, OutPathPoints);
    }
    else
    {
        OutPathPoints.Reserve(CellPath.Num());
        for (const FIntVector& Cell : CellPath)
        {
            OutPathPoints.Add(NavGrid->GetCellCenter(Cell));
        }
    }

    return OutPathPoints.Num() >= 2;
}

bool UFlowFieldManager::IsPointWalkable(const FVector& Point) const
{
    if (!NavGrid)
        return false;

    return NavGrid->IsWalkable(NavGrid->WorldToGrid(Point));
}

bool UFlowFieldManager::IsSegmentWalkable(const FVector& Start, const FVector& End, float ClearanceRadius) const
{
    if (!NavGrid)
    {
        return false;
    }

    FVector Delta = End - Start;
    Delta.Z = 0.0f;
    const float Distance = Delta.Size();
    if (Distance <= KINDA_SMALL_NUMBER)
    {
        return IsPointWalkable(Start);
    }

    const float CellSize = FMath::Max(1.0f, NavGrid->GetCellSize());
    const float StepDistance = FMath::Max(1.0f, CellSize * 0.5f);
    const int32 StepCount = FMath::Max(1, FMath::CeilToInt(Distance / StepDistance));
    const FVector ClearanceOffset = FVector(-Delta.Y, Delta.X, 0.0f).GetSafeNormal2D() * FMath::Max(0.0f, ClearanceRadius);

    for (int32 StepIndex = 0; StepIndex <= StepCount; ++StepIndex)
    {
        const float Alpha = static_cast<float>(StepIndex) / static_cast<float>(StepCount);
        const FVector Sample = FMath::Lerp(Start, End, Alpha);
        if (!IsPointWalkable(Sample))
        {
            return false;
        }

        if (ClearanceOffset.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            if (!IsPointWalkable(Sample + ClearanceOffset) || !IsPointWalkable(Sample - ClearanceOffset))
            {
                return false;
            }
        }
    }

    return true;
}

bool UFlowFieldManager::FindNearestWalkablePoint(const FVector& Point, float SearchRadius, FVector& OutPoint) const
{
    if (!NavGrid)
    {
        return false;
    }

    const FIntVector RequestedCell = NavGrid->WorldToGrid(Point);
    const int32 SearchCellRadius = WorldRangeToCellRange(FMath::Max(0.0f, SearchRadius));
    const FIntVector ResolvedCell = ResolveWalkableTargetCell(RequestedCell, SearchCellRadius);
    if (!NavGrid->IsWalkable(ResolvedCell))
    {
        return false;
    }

    OutPoint = NavGrid->GetCellCenter(ResolvedCell);
    OutPoint.Z = Point.Z;
    return true;
}

void UFlowFieldManager::UpdateDynamicObstacles()
{
    // FlowField只包含静态障碍物（墙壁、建筑），动态障碍物（obstacle AI）由Steering Separation处理
    // 这样FlowField缓存极其稳定：地形不变则场不变，不会因为AI进入/退出攻击槽而全场重建
    // obstacle AI的绕行效果由Steering双层Separation实现：obstacle邻居用强权重，移动邻居用弱权重
}

void UFlowFieldManager::ProcessCompletedFlowFields()
{
    TArray<FFlowFieldTask> CompletedTasks;

    {
        FScopeLock Lock(&FlowFieldLock);
        for (int32 i = PendingFlowFieldTasks.Num() - 1; i >= 0; --i)
        {
            if (PendingFlowFieldTasks[i].Status == EPathfindingStatus::Completed ||
                PendingFlowFieldTasks[i].Status == EPathfindingStatus::Failed)
            {
                CompletedTasks.Add(PendingFlowFieldTasks[i]);
                PendingFlowFieldTasks.RemoveAtSwap(i);
            }
        }
    }

    for (const FFlowFieldTask& Task : CompletedTasks)
    {
        FFlowFieldResult Result;
        Result.bSuccess = (Task.Status == EPathfindingStatus::Completed);

        if (Result.bSuccess)
        {
            // 修复：使用量化后的CacheKey查找缓存，而非精确TargetCell
            FIntVector TargetCell = ResolveWalkableTargetCell(NavGrid->WorldToGrid(Task.Target));
            FIntVector CacheKey = QuantizeToCacheLevel(TargetCell);
            FFlowFieldData* FlowField = NavGrid->GetFlowFieldCache(CacheKey);
            if (FlowField)
            {
                GetPathFromFlowField(Task.Target, Task.Target, Result.PathPoints);
            }
        }

        Task.Callback.ExecuteIfBound(Task.Handle, Result);
    }
}
