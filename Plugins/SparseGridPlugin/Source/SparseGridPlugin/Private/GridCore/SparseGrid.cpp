#include "SparseGrid.h"
#include "SparseGridManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Async/ParallelFor.h"
#include "SparseGridObject.h"

USparseGrid::USparseGrid()
{
    BaseRadius = 50.0f;
    NumLevels = 4;
    CellSize = 2.0f * BaseRadius;
    InvCellSize = 1.0f / CellSize;

    PartitionLocks.SetNum(NUM_PARTITIONS);
}

USparseGrid::~USparseGrid()
{
    Clear();
}

void USparseGrid::Initialize(float InBaseRadius, int32 InNumLevels)
{
    Clear();
    BaseRadius = FMath::Max(InBaseRadius, 1.0f);
    NumLevels = FMath::Max(InNumLevels, 1);
    CellSize = 2.0f * BaseRadius;
    InvCellSize = 1.0f / CellSize;

    UE_LOG(LogTemp, Log, TEXT("SparseGrid initialized: BaseRadius=%.2f, CellSize=%.2f, NumLevels=%d"),
           BaseRadius, CellSize, NumLevels);
}

int32 USparseGrid::GetPartitionIndex(const FIntVector& Coord) const
{
    uint32 Hash = static_cast<uint32>(Coord.X) * 73856093u;
    Hash ^= static_cast<uint32>(Coord.Y) * 19349663u;
    return static_cast<int32>(Hash % NUM_PARTITIONS);
}

void USparseGrid::AcquirePartitionLocks(int32 PartitionA, int32 PartitionB, bool bWriteLock)
{
    int32 Low = FMath::Min(PartitionA, PartitionB);
    int32 High = FMath::Max(PartitionA, PartitionB);

    if (bWriteLock)
    {
        PartitionLocks[Low].WriteLock();
        if (Low != High)
        {
            PartitionLocks[High].WriteLock();
        }
    }
    else
    {
        PartitionLocks[Low].ReadLock();
        if (Low != High)
        {
            PartitionLocks[High].ReadLock();
        }
    }
}

void USparseGrid::ReleasePartitionLocks(int32 PartitionA, int32 PartitionB, bool bWriteLock)
{
    int32 Low = FMath::Min(PartitionA, PartitionB);
    int32 High = FMath::Max(PartitionA, PartitionB);

    if (bWriteLock)
    {
        PartitionLocks[Low].WriteUnlock();
        if (Low != High)
        {
            PartitionLocks[High].WriteUnlock();
        }
    }
    else
    {
        PartitionLocks[Low].ReadUnlock();
        if (Low != High)
        {
            PartitionLocks[High].ReadUnlock();
        }
    }
}

void USparseGrid::AddObject(const FSparseGridHandle& Handle, const FVector& Position)
{
    if (!Handle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to add invalid handle to sparse grid"));
        return;
    }

    FIntVector GridCoord = WorldToGrid(Position);
    int32 Partition = GetPartitionIndex(GridCoord);

    PartitionLocks[Partition].WriteLock();

    if (HandleToCell.Contains(Handle))
    {
        PartitionLocks[Partition].WriteUnlock();
        UE_LOG(LogTemp, Warning, TEXT("Handle %s already exists, use UpdateObject"), *Handle.ToString());
        return;
    }

    int32 Faction = GetFactionFromHandle(Handle);
    AddObject_Internal(Handle, GridCoord, Faction);

    PartitionLocks[Partition].WriteUnlock();

    UE_LOG(LogTemp, VeryVerbose, TEXT("Added object %s to cell %s (partition %d) faction %d"),
           *Handle.ToString(), *GridCoord.ToString(), Partition, Faction);
}

void USparseGrid::AddObjectWithFaction(const FSparseGridHandle& Handle, const FVector& Position, int32 Faction)
{
    if (!Handle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to add invalid handle to sparse grid"));
        return;
    }

    FIntVector GridCoord = WorldToGrid(Position);
    int32 Partition = GetPartitionIndex(GridCoord);

    PartitionLocks[Partition].WriteLock();

    if (HandleToCell.Contains(Handle))
    {
        PartitionLocks[Partition].WriteUnlock();
        UE_LOG(LogTemp, Warning, TEXT("Handle %s already exists, use UpdateObject"), *Handle.ToString());
        return;
    }

    AddObject_Internal(Handle, GridCoord, Faction);

    PartitionLocks[Partition].WriteUnlock();

    UE_LOG(LogTemp, VeryVerbose, TEXT("Added object %s to cell %s (partition %d) faction %d"),
           *Handle.ToString(), *GridCoord.ToString(), Partition, Faction);
}

void USparseGrid::AddObject_Internal(const FSparseGridHandle& Handle, const FIntVector& GridCoord, int32 Faction)
{
    FCell* Cell = GetOrCreateCell(GridCoord);
    Cell->Add(Handle, Faction);
    HandleToCell.Add(Handle, GridCoord);
    HandleToFaction.Add(Handle, Faction);
}

void USparseGrid::UpdateObject(const FSparseGridHandle& Handle, const FVector& NewPosition)
{
    FIntVector NewCoord = WorldToGrid(NewPosition);

    FIntVector OldCoord;
    bool bFound = false;
    int32 OldPartition = -1;
    int32 NewPartition = GetPartitionIndex(NewCoord);

    {
        for (int32 i = 0; i < NUM_PARTITIONS; i++)
        {
            PartitionLocks[i].ReadLock();
        }

        FIntVector* OldCoordPtr = HandleToCell.Find(Handle);
        if (OldCoordPtr)
        {
            OldCoord = *OldCoordPtr;
            OldPartition = GetPartitionIndex(OldCoord);
            bFound = true;
        }

        for (int32 i = 0; i < NUM_PARTITIONS; i++)
        {
            PartitionLocks[i].ReadUnlock();
        }
    }

    if (!bFound)
    {
        AddObject(Handle, NewPosition);
        return;
    }

    if (NewCoord == OldCoord)
        return;

    AcquirePartitionLocks(OldPartition, NewPartition, true);

    FIntVector* CurrentCoord = HandleToCell.Find(Handle);
    if (CurrentCoord && *CurrentCoord == OldCoord)
    {
        MoveObject_Internal(Handle, OldCoord, NewCoord);
    }

    ReleasePartitionLocks(OldPartition, NewPartition, true);

    UE_LOG(LogTemp, VeryVerbose, TEXT("Moved object %s from %s to %s"),
           *Handle.ToString(), *OldCoord.ToString(), *NewCoord.ToString());
}

void USparseGrid::RemoveObject(const FSparseGridHandle& Handle)
{
    FIntVector Coord;
    int32 Partition = -1;
    bool bFound = false;

    {
        for (int32 i = 0; i < NUM_PARTITIONS; i++)
        {
            PartitionLocks[i].ReadLock();
        }

        FIntVector* CoordPtr = HandleToCell.Find(Handle);
        if (CoordPtr)
        {
            Coord = *CoordPtr;
            Partition = GetPartitionIndex(Coord);
            bFound = true;
        }

        for (int32 i = 0; i < NUM_PARTITIONS; i++)
        {
            PartitionLocks[i].ReadUnlock();
        }
    }

    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveObject: handle not found"));
        return;
    }

    PartitionLocks[Partition].WriteLock();

    FIntVector* CurrentCoord = HandleToCell.Find(Handle);
    if (CurrentCoord && *CurrentCoord == Coord)
    {
        RemoveObject_Internal(Handle, Coord);
    }

    PartitionLocks[Partition].WriteUnlock();

    UE_LOG(LogTemp, VeryVerbose, TEXT("Removed object %s"), *Handle.ToString());
}

void USparseGrid::RemoveObject_Internal(const FSparseGridHandle& Handle, const FIntVector& GridCoord)
{
    int32 Faction = 0;
    if (int32* FactionPtr = HandleToFaction.Find(Handle))
    {
        Faction = *FactionPtr;
    }

    FCell* Cell = GetCell(GridCoord);
    if (Cell)
    {
        Cell->Remove(Handle, Faction);

        if (Cell->IsEmpty())
        {
            GridCells.Remove(GridCoord);
            delete Cell;
        }
    }

    HandleToCell.Remove(Handle);
    HandleToFaction.Remove(Handle);
}

void USparseGrid::MoveObject_Internal(const FSparseGridHandle& Handle, const FIntVector& From, const FIntVector& To)
{
    int32 Faction = 0;
    if (int32* FactionPtr = HandleToFaction.Find(Handle))
    {
        Faction = *FactionPtr;
    }

    FCell* OldCell = GetCell(From);
    if (OldCell)
    {
        OldCell->Remove(Handle, Faction);

        if (OldCell->IsEmpty())
        {
            GridCells.Remove(From);
            delete OldCell;
        }
    }

    FCell* NewCell = GetOrCreateCell(To);
    NewCell->Add(Handle, Faction);

    HandleToCell[Handle] = To;
}

TArray<FSparseGridHandle> USparseGrid::QuerySphere(const FVector& Center, float Radius) const
{
    TArray<FSparseGridHandle> Results;

    if (Radius <= 0.0f)
        return Results;

    TArray<int32> TouchedPartitions = GetPartitionsForSphere(Center, Radius);

    for (int32 Partition : TouchedPartitions)
    {
        PartitionLocks[Partition].ReadLock();
    }

    ForCellsInSphere(Center, Radius, [&](const FIntVector& Coord)
    {
        const FCell* Cell = GetCell(Coord);
        if (Cell && !Cell->IsEmpty())
        {
            Cell->GetAllHandles(Results);
        }
    });

    for (int32 Partition : TouchedPartitions)
    {
        PartitionLocks[Partition].ReadUnlock();
    }

    UE_LOG(LogTemp, Verbose, TEXT("Sphere query at %s radius %.2f returned %d objects"),
           *Center.ToString(), Radius, Results.Num());

    return Results;
}

TArray<FSparseGridHandle> USparseGrid::QuerySphereExcludingFactions(const FVector& Center, float Radius, int32 ExcludeFactionID) const
{
    TArray<FSparseGridHandle> Results;

    if (Radius <= 0.0f)
        return Results;

    TArray<int32> TouchedPartitions = GetPartitionsForSphere(Center, Radius);

    for (int32 Partition : TouchedPartitions)
    {
        PartitionLocks[Partition].ReadLock();
    }

    ForCellsInSphere(Center, Radius, [&](const FIntVector& Coord)
    {
        const FCell* Cell = GetCell(Coord);
        if (Cell && !Cell->IsEmpty())
        {
            Cell->GetHandlesExcludingFactions(ExcludeFactionID, Results);
        }
    });

    for (int32 Partition : TouchedPartitions)
    {
        PartitionLocks[Partition].ReadUnlock();
    }

    return Results;
}

TArray<int32> USparseGrid::GetPartitionsForSphere(const FVector& Center, float Radius) const
{
    TSet<int32> Partitions;

    FVector Min = Center - FVector(Radius, Radius, 0.0f);
    FVector Max = Center + FVector(Radius, Radius, 0.0f);

    FIntVector MinCoord = WorldToGrid(Min);
    FIntVector MaxCoord = WorldToGrid(Max);

    for (int32 X = MinCoord.X; X <= MaxCoord.X; X++)
    {
        for (int32 Y = MinCoord.Y; Y <= MaxCoord.Y; Y++)
        {
            Partitions.Add(GetPartitionIndex(FIntVector(X, Y, 0)));
        }
    }

    return Partitions.Array();
}

TArray<int32> USparseGrid::GetPartitionsForBox(const FBox& Box) const
{
    TSet<int32> Partitions;

    FIntVector MinCoord = WorldToGrid(Box.Min);
    FIntVector MaxCoord = WorldToGrid(Box.Max);

    for (int32 X = MinCoord.X; X <= MaxCoord.X; X++)
    {
        for (int32 Y = MinCoord.Y; Y <= MaxCoord.Y; Y++)
        {
            Partitions.Add(GetPartitionIndex(FIntVector(X, Y, 0)));
        }
    }

    return Partitions.Array();
}

TArray<FSparseGridHandle> USparseGrid::QueryBox(const FBox& Box) const
{
    TArray<FSparseGridHandle> Results;

    if (!Box.IsValid)
        return Results;

    TArray<int32> TouchedPartitions = GetPartitionsForBox(Box);

    for (int32 Partition : TouchedPartitions)
    {
        PartitionLocks[Partition].ReadLock();
    }

    ForCellsInBox(Box, [&](const FIntVector& Coord)
    {
        const FCell* Cell = GetCell(Coord);
        if (Cell && !Cell->IsEmpty())
        {
            Cell->GetAllHandles(Results);
        }
    });

    for (int32 Partition : TouchedPartitions)
    {
        PartitionLocks[Partition].ReadUnlock();
    }

    return Results;
}

TArray<FSparseGridHandle> USparseGrid::QueryBoxExcludingFactions(const FBox& Box, int32 ExcludeFactionID) const
{
    TArray<FSparseGridHandle> Results;

    if (!Box.IsValid)
        return Results;

    TArray<int32> TouchedPartitions = GetPartitionsForBox(Box);

    for (int32 Partition : TouchedPartitions)
    {
        PartitionLocks[Partition].ReadLock();
    }

    ForCellsInBox(Box, [&](const FIntVector& Coord)
    {
        const FCell* Cell = GetCell(Coord);
        if (Cell && !Cell->IsEmpty())
        {
            Cell->GetHandlesExcludingFactions(ExcludeFactionID, Results);
        }
    });

    for (int32 Partition : TouchedPartitions)
    {
        PartitionLocks[Partition].ReadUnlock();
    }

    return Results;
}

TArray<FSparseGridHandle> USparseGrid::QueryCell(const FIntVector& CellCoord) const
{
    TArray<FSparseGridHandle> Results;

    int32 Partition = GetPartitionIndex(CellCoord);

    PartitionLocks[Partition].ReadLock();

    const FCell* Cell = GetCell(CellCoord);
    if (Cell && !Cell->IsEmpty())
    {
        Cell->GetAllHandles(Results);
    }

    PartitionLocks[Partition].ReadUnlock();

    return Results;
}

void USparseGrid::Clear()
{
    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].WriteLock();
    }

    for (auto& Pair : GridCells)
    {
        delete Pair.Value;
    }

    GridCells.Empty();
    HandleToCell.Empty();
    HandleToFaction.Empty();

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].WriteUnlock();
    }

    UE_LOG(LogTemp, Log, TEXT("SparseGrid cleared"));
}

void USparseGrid::UpdateObjectFaction(const FSparseGridHandle& Handle, int32 OldFaction, int32 NewFaction)
{
    if (OldFaction == NewFaction)
        return;

    int32 PartitionToLock = -1;
    FIntVector CellCoord;

    {
        for (int32 i = 0; i < NUM_PARTITIONS; i++)
        {
            PartitionLocks[i].ReadLock();
        }

        FIntVector* CoordPtr = HandleToCell.Find(Handle);
        if (CoordPtr)
        {
            CellCoord = *CoordPtr;
            PartitionToLock = GetPartitionIndex(CellCoord);
        }

        for (int32 i = 0; i < NUM_PARTITIONS; i++)
        {
            PartitionLocks[i].ReadUnlock();
        }
    }

    if (PartitionToLock < 0)
        return;

    PartitionLocks[PartitionToLock].WriteLock();

    FCell* Cell = GetCell(CellCoord);
    if (Cell)
    {
        Cell->Remove(Handle, OldFaction);
        Cell->Add(Handle, NewFaction);
    }

    HandleToFaction[Handle] = NewFaction;

    PartitionLocks[PartitionToLock].WriteUnlock();
}

void USparseGrid::GetStatistics(int32& OutTotalCells, int32& OutNonEmptyCells, int32& OutTotalObjects) const
{
    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadLock();
    }

    OutTotalCells = GridCells.Num();
    OutNonEmptyCells = 0;
    OutTotalObjects = 0;

    for (const auto& Pair : GridCells)
    {
        const FCell* Cell = Pair.Value;
        if (Cell && !Cell->IsEmpty())
        {
            OutNonEmptyCells++;
            OutTotalObjects += Cell->Num();
        }
    }

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadUnlock();
    }
}

void USparseGrid::DrawDebug(UWorld* World, FLinearColor Color, float Duration) const
{
    if (!World)
        return;

    FColor DrawColor = Color.ToFColor(true);

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadLock();
    }

    for (const auto& Pair : GridCells)
    {
        const FIntVector& Coord = Pair.Key;
        const FCell* Cell = Pair.Value;

        FVector Min(Coord.X * CellSize, Coord.Y * CellSize, 0.0f);
        FVector Max = Min + FVector(CellSize, CellSize, 0.0f);
        FVector Center = (Min + Max) * 0.5f;
        FVector Extent = (Max - Min) * 0.5f;

        DrawDebugBox(World, Center, Extent, DrawColor, false, Duration, 0, 1.0f);

        if (Cell && Cell->Num() > 0)
        {
            FString Text = FString::Printf(TEXT("%d"), Cell->Num());
            DrawDebugString(World, Center, Text, nullptr, FColor::White, Duration);
        }
    }

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadUnlock();
    }
}

void USparseGrid::DrawDebugEx(UWorld* World, FLinearColor Color, float Duration) const
{
    if (!World)
        return;

    FColor DrawColor = Color.ToFColor(true);

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadLock();
    }

    int32 CellsDrawn = 0;

    for (const auto& Pair : GridCells)
    {
        const FIntVector& Coord = Pair.Key;
        const FCell* Cell = Pair.Value;

        FVector Min(Coord.X * CellSize, Coord.Y * CellSize, 0.0f);
        FVector Max = Min + FVector(CellSize, CellSize, 0.0f);
        FVector Center = Min + FVector(CellSize * 0.5f, CellSize * 0.5f, 0.0f);

        float Thickness = Cell && Cell->Num() > 0 ? 10.0f : 0.5f;

        FVector Corners[4];
        Corners[0] = Min;
        Corners[1] = FVector(Max.X, Min.Y, 0.0f);
        Corners[2] = FVector(Max.X, Max.Y, 0.0f);
        Corners[3] = FVector(Min.X, Max.Y, 0.0f);

        for (int32 i = 0; i < 4; i++)
        {
            DrawDebugLine(World, Corners[i], Corners[(i + 1) % 4], DrawColor, false, Duration, 0, Thickness);
        }

        if (Cell && Cell->Num() > 0)
        {
            FString Text = FString::Printf(TEXT("%d"), Cell->Num());
            DrawDebugString(World, Center, Text, nullptr, FColor::White, Duration);
        }

        CellsDrawn++;
    }

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadUnlock();
    }

    if (Duration > 0.0f && CellsDrawn > 0)
    {
        int32 TotalCells, NonEmptyCells, TotalObjects;
        GetStatistics(TotalCells, NonEmptyCells, TotalObjects);

        UE_LOG(LogTemp, Log, TEXT("SparseGrid Debug: %d cells drawn, %d non-empty, %d objects"),
               CellsDrawn, NonEmptyCells, TotalObjects);
    }
}

void USparseGrid::BatchUpdate(const TArray<TPair<FSparseGridHandle, FVector>>& Updates)
{
    if (Updates.Num() == 0)
        return;

    for (const auto& Update : Updates)
    {
        UpdateObject(Update.Key, Update.Value);
    }
}

FIntVector USparseGrid::WorldToGrid(const FVector& WorldPos) const
{
    return FIntVector(
        FMath::FloorToInt(WorldPos.X * InvCellSize),
        FMath::FloorToInt(WorldPos.Y * InvCellSize),
        0
    );
}

int32 USparseGrid::GetFactionFromHandle(const FSparseGridHandle& Handle) const
{
    USparseGridManager* Manager = Cast<USparseGridManager>(GetOuter());
    if (Manager)
    {
        if (ISparseGridObject* Object = Manager->GetObjectInterface(Handle))
        {
            return Object->GetFaction();
        }
    }
    return -1;
}

USparseGrid::FCell* USparseGrid::GetOrCreateCell(const FIntVector& GridCoord)
{
    FCell** CellPtr = GridCells.Find(GridCoord);
    if (CellPtr)
        return *CellPtr;

    FCell* NewCell = new FCell();
    GridCells.Add(GridCoord, NewCell);
    return NewCell;
}

USparseGrid::FCell* USparseGrid::GetCell(const FIntVector& GridCoord)
{
    FCell** CellPtr = GridCells.Find(GridCoord);
    return CellPtr ? *CellPtr : nullptr;
}

const USparseGrid::FCell* USparseGrid::GetCell(const FIntVector& GridCoord) const
{
    const FCell* const* CellPtr = GridCells.Find(GridCoord);
    return CellPtr ? *CellPtr : nullptr;
}

void USparseGrid::ForCellsInSphere(const FVector& Center, float Radius,
                                  TFunctionRef<void(const FIntVector&)> Func) const
{
    FVector Min = Center - FVector(Radius, Radius, 0.0f);
    FVector Max = Center + FVector(Radius, Radius, 0.0f);

    FIntVector MinCoord = WorldToGrid(Min);
    FIntVector MaxCoord = WorldToGrid(Max);

    for (int32 X = MinCoord.X; X <= MaxCoord.X; X++)
    {
        for (int32 Y = MinCoord.Y; Y <= MaxCoord.Y; Y++)
        {
            Func(FIntVector(X, Y, 0));
        }
    }
}

void USparseGrid::ForCellsInBox(const FBox& Box,
                               TFunctionRef<void(const FIntVector&)> Func) const
{
    FIntVector MinCoord = WorldToGrid(Box.Min);
    FIntVector MaxCoord = WorldToGrid(Box.Max);

    for (int32 X = MinCoord.X; X <= MaxCoord.X; X++)
    {
        for (int32 Y = MinCoord.Y; Y <= MaxCoord.Y; Y++)
        {
            Func(FIntVector(X, Y, 0));
        }
    }
}

int32 USparseGrid::GetOptimalLevelForRadius(float Radius) const
{
    for (int32 Level = NumLevels - 1; Level >= 0; --Level)
    {
        float LevelCellSize = GetLevelCellSize(Level);
        if (LevelCellSize <= Radius * 2.0f)
        {
            return Level;
        }
    }
    return 0;
}

TArray<USparseGrid::FCoarseCellInfo> USparseGrid::QuerySphereAtLevel(const FVector& Center, float Radius, int32 Level) const
{
    TArray<FCoarseCellInfo> Results;

    if (Radius <= 0.0f || Level < 0 || Level >= NumLevels)
        return Results;

    if (Level == 0)
    {
        TArray<FSparseGridHandle> Handles = QuerySphere(Center, Radius);
        if (Handles.Num() > 0)
        {
            FCoarseCellInfo Info;
            Info.CellCoord = WorldToGrid(Center);
            Info.ObjectCount = Handles.Num();
            Results.Add(Info);
        }
        return Results;
    }

    float LevelCellSize = GetLevelCellSize(Level);
    float InvLevelCellSize = 1.0f / LevelCellSize;

    FIntVector MinLevelCoord(
        FMath::FloorToInt((Center.X - Radius) * InvLevelCellSize),
        FMath::FloorToInt((Center.Y - Radius) * InvLevelCellSize),
        0
    );
    FIntVector MaxLevelCoord(
        FMath::FloorToInt((Center.X + Radius) * InvLevelCellSize),
        FMath::FloorToInt((Center.Y + Radius) * InvLevelCellSize),
        0
    );

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadLock();
    }

    for (int32 LX = MinLevelCoord.X; LX <= MaxLevelCoord.X; LX++)
    {
        for (int32 LY = MinLevelCoord.Y; LY <= MaxLevelCoord.Y; LY++)
        {
            FIntVector CoarseCoord(LX, LY, 0);
            int32 Count = 0;

            FIntVector L0Start = ToL0Coord(CoarseCoord, Level);
            int32 Span = 1 << Level;

            for (int32 X = L0Start.X; X < L0Start.X + Span; X++)
            {
                for (int32 Y = L0Start.Y; Y < L0Start.Y + Span; Y++)
                {
                    const FCell* Cell = GetCell(FIntVector(X, Y, 0));
                    if (Cell && !Cell->IsEmpty())
                    {
                        Count += Cell->Num();
                    }
                }
            }

            if (Count > 0)
            {
                FCoarseCellInfo Info;
                Info.CellCoord = CoarseCoord;
                Info.ObjectCount = Count;
                Results.Add(Info);
            }
        }
    }

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadUnlock();
    }

    return Results;
}

TArray<USparseGrid::FCoarseCellInfo> USparseGrid::QueryBoxAtLevel(const FBox& Box, int32 Level) const
{
    TArray<FCoarseCellInfo> Results;

    if (!Box.IsValid || Level < 0 || Level >= NumLevels)
        return Results;

    if (Level == 0)
    {
        TArray<FSparseGridHandle> Handles = QueryBox(Box);
        if (Handles.Num() > 0)
        {
            FCoarseCellInfo Info;
            Info.CellCoord = WorldToGrid(Box.GetCenter());
            Info.ObjectCount = Handles.Num();
            Results.Add(Info);
        }
        return Results;
    }

    float LevelCellSize = GetLevelCellSize(Level);
    float InvLevelCellSize = 1.0f / LevelCellSize;

    FIntVector MinLevelCoord(
        FMath::FloorToInt(Box.Min.X * InvLevelCellSize),
        FMath::FloorToInt(Box.Min.Y * InvLevelCellSize),
        0
    );
    FIntVector MaxLevelCoord(
        FMath::FloorToInt(Box.Max.X * InvLevelCellSize),
        FMath::FloorToInt(Box.Max.Y * InvLevelCellSize),
        0
    );

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadLock();
    }

    for (int32 LX = MinLevelCoord.X; LX <= MaxLevelCoord.X; LX++)
    {
        for (int32 LY = MinLevelCoord.Y; LY <= MaxLevelCoord.Y; LY++)
        {
            FIntVector CoarseCoord(LX, LY, 0);
            int32 Count = 0;

            FIntVector L0Start = ToL0Coord(CoarseCoord, Level);
            int32 Span = 1 << Level;

            for (int32 X = L0Start.X; X < L0Start.X + Span; X++)
            {
                for (int32 Y = L0Start.Y; Y < L0Start.Y + Span; Y++)
                {
                    const FCell* Cell = GetCell(FIntVector(X, Y, 0));
                    if (Cell && !Cell->IsEmpty())
                    {
                        Count += Cell->Num();
                    }
                }
            }

            if (Count > 0)
            {
                FCoarseCellInfo Info;
                Info.CellCoord = CoarseCoord;
                Info.ObjectCount = Count;
                Results.Add(Info);
            }
        }
    }

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadUnlock();
    }

    return Results;
}

TArray<FSparseGridHandle> USparseGrid::GetHandlesInCoarseCell(const FIntVector& CoarseCoord, int32 Level) const
{
    TArray<FSparseGridHandle> Results;

    if (Level < 0 || Level >= NumLevels)
        return Results;

    if (Level == 0)
    {
        return QueryCell(CoarseCoord);
    }

    FIntVector L0Start = ToL0Coord(CoarseCoord, Level);
    int32 Span = 1 << Level;

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadLock();
    }

    for (int32 X = L0Start.X; X < L0Start.X + Span; X++)
    {
        for (int32 Y = L0Start.Y; Y < L0Start.Y + Span; Y++)
        {
            const FCell* Cell = GetCell(FIntVector(X, Y, 0));
            if (Cell && !Cell->IsEmpty())
            {
                Cell->GetAllHandles(Results);
            }
        }
    }

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadUnlock();
    }

    return Results;
}

int32 USparseGrid::GetCoarseCellObjectCount(const FIntVector& CoarseCoord, int32 Level) const
{
    if (Level < 0 || Level >= NumLevels)
        return 0;

    if (Level == 0)
    {
        const FCell* Cell = GetCell(CoarseCoord);
        return Cell ? Cell->Num() : 0;
    }

    FIntVector L0Start = ToL0Coord(CoarseCoord, Level);
    int32 Span = 1 << Level;
    int32 Count = 0;

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadLock();
    }

    for (int32 X = L0Start.X; X < L0Start.X + Span; X++)
    {
        for (int32 Y = L0Start.Y; Y < L0Start.Y + Span; Y++)
        {
            const FCell* Cell = GetCell(FIntVector(X, Y, 0));
            if (Cell && !Cell->IsEmpty())
            {
                Count += Cell->Num();
            }
        }
    }

    for (int32 i = 0; i < NUM_PARTITIONS; i++)
    {
        PartitionLocks[i].ReadUnlock();
    }

    return Count;
}
