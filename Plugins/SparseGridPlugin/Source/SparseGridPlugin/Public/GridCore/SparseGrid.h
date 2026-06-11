#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SparseGridHandle.h"
#include "SparseGrid.generated.h"

class USparseGridManager;

UCLASS(BlueprintType)
class SPARSEGRIDPLUGIN_API USparseGrid : public UObject
{
    GENERATED_BODY()

public:
    USparseGrid();
    virtual ~USparseGrid() override;

    static constexpr int32 NUM_PARTITIONS = 31;
    static constexpr int32 MAX_FACTIONS = 8;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    void Initialize(float InBaseRadius, int32 InNumLevels = 4);

    void AddObject(const FSparseGridHandle& Handle, const FVector& Position);
    void AddObjectWithFaction(const FSparseGridHandle& Handle, const FVector& Position, int32 Faction);
    void UpdateObject(const FSparseGridHandle& Handle, const FVector& NewPosition);
    void RemoveObject(const FSparseGridHandle& Handle);

    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    TArray<FSparseGridHandle> QuerySphere(const FVector& Center, float Radius) const;

    TArray<FSparseGridHandle> QuerySphereExcludingFactions(const FVector& Center, float Radius, int32 ExcludeFactionID) const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    TArray<FSparseGridHandle> QueryBox(const FBox& Box) const;

    TArray<FSparseGridHandle> QueryBoxExcludingFactions(const FBox& Box, int32 ExcludeFactionID) const;

    TArray<FSparseGridHandle> QueryCell(const FIntVector& CellCoord) const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    void Clear();

    void UpdateObjectFaction(const FSparseGridHandle& Handle, int32 OldFaction, int32 NewFaction);

    void GetStatistics(int32& OutTotalCells, int32& OutNonEmptyCells, int32& OutTotalObjects) const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Debug")
    void DrawDebug(UWorld* World, FLinearColor Color = FLinearColor::Green, float Duration = 0.0f) const;

    void DrawDebugEx(UWorld* World, FLinearColor Color, float Duration) const;

    void BatchUpdate(const TArray<TPair<FSparseGridHandle, FVector>>& Updates);

    float GetCellSize() const { return CellSize; }
    float GetBaseRadius() const { return BaseRadius; }
    int32 GetNumLevels() const { return NumLevels; }

    float GetLevelCellSize(int32 Level) const
    {
        return CellSize * (1 << Level);
    }

    FIntVector ToLevelCoord(const FIntVector& L0Coord, int32 Level) const
    {
        return FIntVector(L0Coord.X >> Level, L0Coord.Y >> Level, 0);
    }

    FIntVector ToL0Coord(const FIntVector& LevelCoord, int32 Level) const
    {
        return FIntVector(LevelCoord.X << Level, LevelCoord.Y << Level, 0);
    }

    struct FCoarseCellInfo
    {
        FIntVector CellCoord;
        int32 ObjectCount = 0;
    };

    TArray<FCoarseCellInfo> QuerySphereAtLevel(const FVector& Center, float Radius, int32 Level) const;
    TArray<FCoarseCellInfo> QueryBoxAtLevel(const FBox& Box, int32 Level) const;
    TArray<FSparseGridHandle> GetHandlesInCoarseCell(const FIntVector& CoarseCoord, int32 Level) const;
    int32 GetCoarseCellObjectCount(const FIntVector& CoarseCoord, int32 Level) const;

    int32 GetOptimalLevelForRadius(float Radius) const;

private:
    struct FCellObjectList
    {
        TArray<FSparseGridHandle> Handles;
    };

    struct FCell
    {
        FCellObjectList FactionObjects[MAX_FACTIONS];
        uint32 ActiveFactionMask = 0;

        void Add(const FSparseGridHandle& Handle, int32 Faction)
        {
            if (Faction >= 0 && Faction < MAX_FACTIONS)
            {
                FactionObjects[Faction].Handles.Add(Handle);
                ActiveFactionMask |= (1u << Faction);
            }
        }

        bool Remove(const FSparseGridHandle& Handle, int32 Faction)
        {
            if (Faction >= 0 && Faction < MAX_FACTIONS)
            {
                FactionObjects[Faction].Handles.RemoveSwap(Handle);
                if (FactionObjects[Faction].Handles.IsEmpty())
                {
                    ActiveFactionMask &= ~(1u << Faction);
                }
                return true;
            }
            return false;
        }

        bool IsEmpty() const { return ActiveFactionMask == 0; }

        int32 Num() const
        {
            int32 Count = 0;
            for (int32 i = 0; i < MAX_FACTIONS; ++i)
            {
                Count += FactionObjects[i].Handles.Num();
            }
            return Count;
        }

        void GetAllHandles(TArray<FSparseGridHandle>& OutHandles) const
        {
            for (int32 i = 0; i < MAX_FACTIONS; ++i)
            {
                OutHandles.Append(FactionObjects[i].Handles);
            }
        }

        void GetHandlesExcludingFactions(int32 ExcludeFactionID, TArray<FSparseGridHandle>& OutHandles) const
        {
            uint32 AvailableMask = ~(1u << ExcludeFactionID) & ActiveFactionMask;
            for (int32 i = 0; i < MAX_FACTIONS; ++i)
            {
                if (AvailableMask & (1u << i))
                {
                    OutHandles.Append(FactionObjects[i].Handles);
                }
            }
        }
    };

    TMap<FIntVector, FCell*> GridCells;
    TMap<FSparseGridHandle, FIntVector> HandleToCell;
    TMap<FSparseGridHandle, int32> HandleToFaction;

    const TMap<FIntVector, FCell*>& GetGridCells() const { return GridCells; }

    mutable TArray<FRWLock> PartitionLocks;

    float BaseRadius = 50.0f;
    int32 NumLevels = 4;
    float CellSize = 100.0f;
    float InvCellSize = 1.0f / 100.0f;

    int32 GetPartitionIndex(const FIntVector& Coord) const;
    void AcquirePartitionLocks(int32 PartitionA, int32 PartitionB, bool bWriteLock);
    void ReleasePartitionLocks(int32 PartitionA, int32 PartitionB, bool bWriteLock);

    FIntVector WorldToGrid(const FVector& WorldPos) const;

    FCell* GetOrCreateCell(const FIntVector& GridCoord);
    FCell* GetCell(const FIntVector& GridCoord);
    const FCell* GetCell(const FIntVector& GridCoord) const;

    void MoveObject_Internal(const FSparseGridHandle& Handle, const FIntVector& From, const FIntVector& To);
    void AddObject_Internal(const FSparseGridHandle& Handle, const FIntVector& GridCoord, int32 Faction);
    void RemoveObject_Internal(const FSparseGridHandle& Handle, const FIntVector& GridCoord);

    int32 GetFactionFromHandle(const FSparseGridHandle& Handle) const;

    void ForCellsInSphere(const FVector& Center, float Radius,
                         TFunctionRef<void(const FIntVector&)> Func) const;
    void ForCellsInBox(const FBox& Box,
                      TFunctionRef<void(const FIntVector&)> Func) const;

    TArray<int32> GetPartitionsForSphere(const FVector& Center, float Radius) const;
    TArray<int32> GetPartitionsForBox(const FBox& Box) const;
};
