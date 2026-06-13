#include "SparseGridManager.h"
#include "SparseGridObject.h"
#include "SparseGridRaycast.h"
#include "SparseGridCollisionUtils.h"
#include "SparseGridCollision.h"
#include "FlowFieldManager.h"
#include "CrowdSurroundManager.h"
#include "SparseGridDebug.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Async/ParallelFor.h"

void USparseGridManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CollisionDetector = new FSparseGridCollisionDetector();
    
    SparseGrid = NewObject<USparseGrid>(this);

    float BaseRadius = 50.0f;
    int32 NumLevels = 4;
    if (SparseGridConfig)
    {
        BaseRadius = SparseGridConfig->BaseRadius;
        NumLevels = SparseGridConfig->NumLevels;
    }
    SparseGrid->Initialize(BaseRadius, NumLevels);

    Objects.Reserve(1024);
    FreeIndices.Reserve(512);
    
    UE_LOG(LogTemp, Log, TEXT("SparseGridManager initialized (BaseRadius=%.2f, CellSize=%.2f, NumLevels=%d)"),
           BaseRadius, SparseGrid->GetCellSize(), NumLevels);
}

void USparseGridManager::Deinitialize()
{
    Super::Deinitialize();

    if (CollisionDetector)
    {
        delete CollisionDetector;
        CollisionDetector = nullptr;
    }

    Objects.Empty();
    FreeIndices.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("SparseGridManager deinitialized"));
}

bool USparseGridManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FSparseGridHandle USparseGridManager::CreateObject(ISparseGridObject* Object)
{
    if (!Object)
    {
        return FSparseGridHandle::Invalid;
    }

    FVector Position = Object->GetSparseGridPosition();
    UObject* UObjectPtr = Object->QueryUObject();
    
    int32 Index;
    uint32 Generation;
    
    {
        FWriteScopeLock Lock(ObjectsLock);
        
        if (FreeIndices.Num() > 0)
        {
            Index = FreeIndices.Pop();
            FObjectEntry& Entry = Objects[Index];
            Generation = Entry.Generation + 1;
            Entry.Generation = Generation;
            Entry.bActive = true;
            Entry.Position = Position;
            Entry.ObjectInterface = Object;
            Entry.Object = UObjectPtr;
            Entry.bEnableSteering = Object->EnableSteering();
        }
        else
        {
            Index = Objects.Num();
            Objects.Add(FObjectEntry());
            FObjectEntry& Entry = Objects[Index];
            Generation = 1;
            Entry.Generation = Generation;
            Entry.bActive = true;
            Entry.Position = Position;
            Entry.ObjectInterface = Object;
            Entry.Object = UObjectPtr;
            Entry.bEnableSteering = Object->EnableSteering();
        }
    }
    
    FSparseGridHandle Handle(Index, Generation);
    SparseGrid->AddObject(Handle, Position);

    // Register ObjectID → Handle mapping for O(1) lookup
    int32 ObjectID = Object->GetObjectID();
    if (ObjectID != INDEX_NONE)
    {
        ObjectIDToHandleMap.Add(ObjectID, Handle);
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("Created object %s at %s with interface"),
           *Handle.ToString(), *Position.ToString());
    
    return Handle;
}

void USparseGridManager::DestroyObject(const FSparseGridHandle& Handle)
{
    if (!ValidateHandle(Handle, "DestroyObject"))
        return;

    // Remove ObjectID mapping before clearing the interface
    {
        FReadScopeLock Lock(ObjectsLock);
        int32 Index = Handle.GetIndex();
        if (IsIndexValid(Index) && Objects[Index].ObjectInterface)
        {
            int32 ObjectID = Objects[Index].ObjectInterface->GetObjectID();
            if (ObjectID != INDEX_NONE)
            {
                ObjectIDToHandleMap.Remove(ObjectID);
            }
        }
    }

    // Remove from grid first
    SparseGrid->RemoveObject(Handle);
    
    // Mark as inactive and add to free list
    int32 Index = Handle.GetIndex();
    
    {
        FWriteScopeLock Lock(ObjectsLock);
        Objects[Index].bActive = false;
        Objects[Index].ObjectInterface = nullptr;
        Objects[Index].Object = nullptr;
        FreeIndices.Add(Index);
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Destroyed object %s"), *Handle.ToString());
}

void USparseGridManager::UpdateObjectPosition(const FSparseGridHandle& Handle, 
                                              const FVector& NewPosition)
{
    if (!ValidateHandle(Handle, "UpdateObjectPosition"))
        return;
    
    int32 Index = Handle.GetIndex();
    
    {
        FReadScopeLock Lock(ObjectsLock);
        Objects[Index].Position = NewPosition;
    }
    
    SparseGrid->UpdateObject(Handle, NewPosition);
}

FVector USparseGridManager::GetObjectPosition(const FSparseGridHandle& Handle) const
{
    if (!ValidateHandle(Handle, "GetObjectPosition"))
        return FVector::ZeroVector;
    
    FReadScopeLock Lock(ObjectsLock);
    return Objects[Handle.GetIndex()].Position;
}

ISparseGridObject* USparseGridManager::GetObjectInterface(const FSparseGridHandle& Handle) const
{
    if (!ValidateHandle(Handle, "GetObjectInterface"))
        return nullptr;
    
    FReadScopeLock Lock(ObjectsLock);
    return Objects[Handle.GetIndex()].ObjectInterface;
}

TArray<ISparseGridObject*> USparseGridManager::GetAllObjects() const
{
    TArray<ISparseGridObject*> Result;
    
    FReadScopeLock Lock(ObjectsLock);
    
    Result.Reserve(Objects.Num());
    
    for (const FObjectEntry& Entry : Objects)
    {
        if (Entry.bActive && Entry.ObjectInterface)
        {
            if (Entry.ObjectInterface->IsSparseGridObjectActive())
            {
                Result.Add(Entry.ObjectInterface);
            }
        }
    }
    
    return Result;
}

void USparseGridManager::BatchUpdatePositions(
    const TArray<FSparseGridHandle>& Handles,
    const TArray<FVector>& NewPositions)
{
    int32 Count = FMath::Min(Handles.Num(), NewPositions.Num());
    if (Count == 0)
        return;
    
    // Update positions in Objects array
    {
        FWriteScopeLock Lock(ObjectsLock);
        for (int32 i = 0; i < Count; i++)
        {
            int32 Index = Handles[i].GetIndex();
            if (IsIndexValid(Index))
            {
                Objects[Index].Position = NewPositions[i];
            }
        }
    }
    
    // Prepare batch update for grid
    TArray<TPair<FSparseGridHandle, FVector>> Updates;
    Updates.SetNum(Count);
    for (int32 i = 0; i < Count; i++)
    {
        Updates[i] = {Handles[i], NewPositions[i]};
    }
    
    SparseGrid->BatchUpdate(Updates);
}

TArray<FSparseGridHandle> USparseGridManager::QuerySphere(const FVector& Center, 
                                                          float Radius) const
{
    return SparseGrid->QuerySphere(Center, Radius);
}

TArray<FSparseGridHandle> USparseGridManager::QuerySphereExcludingFactions(const FVector& Center,
	float Radius, int32 ExcludeFactionID) const
{
	return SparseGrid->QuerySphereExcludingFactions(Center, Radius, ExcludeFactionID);
}

TArray<FSparseGridHandle> USparseGridManager::QueryBoxExcludingFactions(const FVector& Center,
	const FVector& Extent, int32 ExcludeFactionID) const
{
	FBox QueryBox = FBox::BuildAABB(Center, Extent);
	return SparseGrid->QueryBoxExcludingFactions(QueryBox, ExcludeFactionID);
}


bool USparseGridManager::IsHandleValid(const FSparseGridHandle& Handle) const
{
    return ValidateHandle(Handle, "");
}

void USparseGridManager::UpdateObjectFaction(const FSparseGridHandle& Handle, int32 OldFaction, int32 NewFaction)
{
    if (!Handle.IsValid())
        return;

    FWriteScopeLock Lock(ObjectsLock);

    int32 Index = Handle.GetIndex();
    if (!IsIndexValid(Index))
        return;

    FObjectEntry& Entry = Objects[Index];
    if (!Entry.bActive || Entry.Generation != Handle.GetGeneration())
        return;

    SparseGrid->UpdateObjectFaction(Handle, OldFaction, NewFaction);
}

void USparseGridManager::GetDebugInfo(FString& OutInfo) const
{
    int32 TotalCells, NonEmptyCells, TotalObjects;
    SparseGrid->GetStatistics(TotalCells, NonEmptyCells, TotalObjects);
    
    FReadScopeLock Lock(ObjectsLock);
    
    OutInfo = FString::Printf(
        TEXT("SparseGrid Stats (Thread-Safe):\n")
        TEXT("  Total Cells: %d\n")
        TEXT("  Non-Empty Cells: %d\n")
        TEXT("  Total Objects: %d\n")
        TEXT("  Object Pool Size: %d\n")
        TEXT("  Free Indices: %d\n")
        TEXT("  Partitions: %d"),
        TotalCells, NonEmptyCells, TotalObjects,
        Objects.Num(), FreeIndices.Num(),
        USparseGrid::NUM_PARTITIONS
    );
}

void USparseGridManager::DrawDebugVisualization(bool bEnabled)
{
    bDebugEnabled = bEnabled;
}

bool USparseGridManager::IsIndexValid(int32 Index) const
{
    return Index >= 0 && Index < Objects.Num();
}

bool USparseGridManager::ValidateHandle(const FSparseGridHandle& Handle, 
                                       const FString& Operation) const
{
    if (!Handle.IsValid())
    {
        if (!Operation.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Invalid handle"), *Operation);
        }
        return false;
    }
    
    int32 Index = Handle.GetIndex();
    if (!IsIndexValid(Index))
    {
        if (!Operation.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Index out of bounds"), *Operation);
        }
        return false;
    }
    
    const FObjectEntry& Entry = Objects[Index];
    if (!Entry.bActive || Entry.Generation != Handle.GetGeneration())
    {
        if (!Operation.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: Handle generation mismatch"), *Operation);
        }
        return false;
    }
    
    return true;
}

// === Raycast Implementation ===

TArray<FSparseGridRaycastHit> USparseGridManager::Raycast(
    const FVector& Origin,
    const FVector& Direction,
    const FSparseGridRaycastConfig& Config)
{
    return RaycastInternal(Origin, Direction.GetSafeNormal(), Config);
}

bool USparseGridManager::RaycastSingle(
    const FVector& Origin,
    const FVector& Direction,
    float MaxDistance,
    FSparseGridRaycastHit& OutHit)
{
    FSparseGridRaycastConfig Config;
    Config.MaxDistance = MaxDistance;
    Config.bStopAtFirstHit = true;
    Config.bSortByDistance = false;
    
    TArray<FSparseGridRaycastHit> Hits = RaycastInternal(Origin, Direction.GetSafeNormal(), Config);
    
    if (Hits.Num() > 0)
    {
        OutHit = Hits[0];
        return true;
    }
    
    OutHit.Reset();
    return false;
}

TArray<FSparseGridRaycastHit> USparseGridManager::RaycastThreadSafe(
    const FVector& Origin,
    const FVector& Direction,
    const FSparseGridRaycastConfig& Config) const
{
    return RaycastInternal(Origin, Direction.GetSafeNormal(), Config);
}

TArray<FSparseGridRaycastHit> USparseGridManager::RaycastInternal(
    const FVector& Origin,
    const FVector& Direction,
    const FSparseGridRaycastConfig& Config) const
{
    TArray<FSparseGridRaycastHit> Hits;
    
    if (!SparseGrid)
    {
        return Hits;
    }
    
    const float CellSize = SparseGrid->GetCellSize();
    const float MaxT = Config.MaxDistance;
    
    // Initialize DDA state
    FSparseGridDDAState DDA;
    DDA.Initialize(Origin, Direction, CellSize);
    
    // Track visited cells to avoid duplicates
    TSet<FIntVector> VisitedCells;
    
    // Traverse cells along the ray
    while (DDA.CurrentT < MaxT)
    {
        // Check if we've already visited this cell
        if (!VisitedCells.Contains(DDA.CurrentCell))
        {
            VisitedCells.Add(DDA.CurrentCell);
            
            // Query objects in this cell
            TArray<FSparseGridHandle> CellObjects = SparseGrid->QueryCell(DDA.CurrentCell);
            
            // Check each object for ray intersection
            for (const FSparseGridHandle& Handle : CellObjects)
            {
                // Get object interface
                ISparseGridObject* GridObject = nullptr;
                {
                    FReadScopeLock Lock(ObjectsLock);
                    int32 Index = Handle.GetIndex();
                    if (IsIndexValid(Index) && Objects[Index].bActive)
                    {
                        GridObject = Objects[Index].ObjectInterface;
                    }
                }
                
                if (!GridObject)
                    continue;
                
                // Check if raycast is enabled
                if (!GridObject->EnableSparseGridRaycast())
                    continue;
                
                // Check layer filtering
                int32 ObjectLayer = GridObject->GetSparseGridRaycastLayer();
                if (Config.ShouldIgnoreLayer(ObjectLayer))
                    continue;
                
                // Perform ray-object intersection test using utility
                FVector HitPoint;
                float HitDistance;
                FVector Center = GridObject->GetSparseGridPosition();
                float Radius = GridObject->GetSparseGridCollisionRadius();
                
                if (FSparseGridCollisionUtils::RaycastSphere(Center, Radius, Origin, Direction, HitPoint, HitDistance))
                {
                    // Check if hit is within max distance
                    if (HitDistance <= MaxT)
                    {
                        FSparseGridRaycastHit& Hit = Hits.AddDefaulted_GetRef();
                        Hit.HitActor = GridObject->GetSparseGridOwnerActor();
                        Hit.HitPoint = HitPoint;
                        Hit.HitDistance = HitDistance;
                        Hit.HitLayer = ObjectLayer;
                        Hit.HitNormal = (HitPoint - Center).GetSafeNormal();
                        
                        // Early exit if we only need first hit
                        if (Config.bStopAtFirstHit)
                        {
                            return Hits;
                        }
                    }
                }
            }
        }
        
        // Step to next cell
        DDA.StepToNextCell();
    }
    
    // Sort by distance if requested
    if (Config.bSortByDistance && Hits.Num() > 1)
    {
        Hits.Sort([](const FSparseGridRaycastHit& A, const FSparseGridRaycastHit& B)
        {
            return A.HitDistance < B.HitDistance;
        });
    }
    
    return Hits;
}

void USparseGridManager::NotifyActorRemoved(AActor* Actor)
{
	if (Actor)
	{
		OnActorRemovedFromGridDelegate.Broadcast(Actor);
	}
}

FCollisionTickResult USparseGridManager::ProcessCollisions()
{
    if (!CollisionDetector)
    {
        return FCollisionTickResult();
    }

    return CollisionDetector->Tick(this);
}

void USparseGridManager::SetCollisionEnabled(bool bEnabled)
{
    if (CollisionDetector)
    {
        CollisionDetector->SetEnabled(bEnabled);
    }
}

bool USparseGridManager::IsCollisionEnabled() const
{
    if (CollisionDetector)
    {
        return CollisionDetector->IsEnabled();
    }
    return false;
}

void USparseGridManager::SetCollisionMatrix(const FSparseGridCollisionMatrix& InMatrix)
{
    if (CollisionDetector)
    {
        CollisionDetector->SetCollisionMatrix(InMatrix);
    }
}

FSparseGridCollisionMatrix USparseGridManager::GetCollisionMatrix() const
{
    if (CollisionDetector)
    {
        return CollisionDetector->GetCollisionMatrix();
    }
    return FSparseGridCollisionMatrix();
}

FSparseGridCollisionStats USparseGridManager::GetCollisionStats() const
{
    if (CollisionDetector)
    {
        return CollisionDetector->GetStats();
    }
    return FSparseGridCollisionStats();
}

int32 USparseGridManager::GetActiveCollisionCount() const
{
    if (CollisionDetector)
    {
        return CollisionDetector->GetActiveCollisions().Num();
    }
    return 0;
}

TArray<FSparseGridCollisionPair> USparseGridManager::GetActiveCollisions() const
{
    TArray<FSparseGridCollisionPair> Result;
    if (CollisionDetector)
    {
        Result.Reserve(CollisionDetector->GetActiveCollisions().Num());
        for (const FSparseGridCollisionPair& Pair : CollisionDetector->GetActiveCollisions())
        {
            Result.Add(Pair);
        }
    }
    return Result;
}

TArray<AActor*> USparseGridManager::QueryActorsInSphere(
    const FVector& Center,
    float Radius,
    const FSpatialQueryFilter& Filter) const
{
    TArray<AActor*> Result;

    int32 ExcludeFactionID = -1;
    if (Filter.FactionFilter >= 0)
    {
        ExcludeFactionID = Filter.FactionFilter;
    }

    TArray<FSparseGridHandle> Handles;
    if (ExcludeFactionID >= 0)
    {
        Handles = QuerySphereExcludingFactions(Center, Radius, ExcludeFactionID);
    }
    else
    {
        Handles = QuerySphere(Center, Radius);
    }

    for (const FSparseGridHandle& Handle : Handles)
    {
        ISparseGridObject* Obj = GetObjectInterface(Handle);
        if (!Obj)
            continue;

        AActor* Actor = Obj->GetSparseGridOwnerActor();
        if (!Actor)
            continue;

        if (Filter.ActorClasses.Num() > 0)
        {
            bool bMatchesClass = false;
            for (const TSubclassOf<AActor>& ClassFilter : Filter.ActorClasses)
            {
                if (Actor->IsA(ClassFilter))
                {
                    bMatchesClass = true;
                    break;
                }
            }
            if (!bMatchesClass)
                continue;
        }

        if (Filter.MaxDistance < MAX_FLT)
        {
            if (FVector::DistSquared2D(Center, Obj->GetSparseGridPosition()) > Filter.MaxDistance * Filter.MaxDistance)
                continue;
        }

        Result.Add(Actor);
    }

    return Result;
}

TArray<AActor*> USparseGridManager::QueryActorsInBox(
    const FVector& Center,
    const FVector& Extent,
    const FSpatialQueryFilter& Filter) const
{
    TArray<AActor*> Result;

    int32 ExcludeFactionID = -1;
    if (Filter.FactionFilter >= 0)
    {
        ExcludeFactionID = Filter.FactionFilter;
    }

    TArray<FSparseGridHandle> Handles;
    if (ExcludeFactionID >= 0)
    {
        Handles = QueryBoxExcludingFactions(Center, Extent, ExcludeFactionID);
    }
    else
    {
        FBox QueryBox = FBox::BuildAABB(Center, Extent);
        Handles = SparseGrid->QueryBox(QueryBox);
    }

    for (const FSparseGridHandle& Handle : Handles)
    {
        ISparseGridObject* Obj = GetObjectInterface(Handle);
        if (!Obj)
            continue;

        AActor* Actor = Obj->GetSparseGridOwnerActor();
        if (!Actor)
            continue;

        if (Filter.ActorClasses.Num() > 0)
        {
            bool bMatchesClass = false;
            for (const TSubclassOf<AActor>& ClassFilter : Filter.ActorClasses)
            {
                if (Actor->IsA(ClassFilter))
                {
                    bMatchesClass = true;
                    break;
                }
            }
            if (!bMatchesClass)
                continue;
        }

        Result.Add(Actor);
    }

    return Result;
}

AActor* USparseGridManager::QueryNearestActor(
    const FVector& Origin,
    const FSpatialQueryFilter& Filter) const
{
    float SearchRadius = Filter.MaxDistance < MAX_FLT ? Filter.MaxDistance : 10000.0f;

    TArray<AActor*> Actors = QueryActorsInSphere(Origin, SearchRadius, Filter);

    AActor* Nearest = nullptr;
    float MinDistSq = MAX_FLT;
    for (AActor* Actor : Actors)
    {
        float DistSq = FVector::DistSquared2D(Origin, Actor->GetActorLocation());
        if (DistSq < MinDistSq)
        {
            MinDistSq = DistSq;
            Nearest = Actor;
        }
    }

    return Nearest;
}

FSpatialQueryHandle USparseGridManager::ToSpatialQueryHandle(const FSparseGridHandle& Handle)
{
    FSpatialQueryHandle Result;
    Result.Index = static_cast<int32>(Handle.GetIndex());
    Result.Version = Handle.GetGeneration();
    return Result;
}

FSparseGridHandle USparseGridManager::ToGridHandle(const FSpatialQueryHandle& Handle)
{
    return FSparseGridHandle(static_cast<uint32>(Handle.Index), Handle.Version);
}

TArray<FSpatialQueryHandle> USparseGridManager::QueryHandlesInSphere(
    const FVector& Center,
    float Radius) const
{
    TArray<FSparseGridHandle> GridHandles = QuerySphere(Center, Radius);
    TArray<FSpatialQueryHandle> Result;
    Result.Reserve(GridHandles.Num());
    for (const FSparseGridHandle& H : GridHandles)
    {
        Result.Add(ToSpatialQueryHandle(H));
    }
    return Result;
}

TArray<FSpatialQueryHandle> USparseGridManager::QueryHandlesInSphereExcludingFactions(
    const FVector& Center,
    float Radius,
    int32 ExcludeFactionID) const
{
    TArray<FSparseGridHandle> GridHandles = QuerySphereExcludingFactions(Center, Radius, ExcludeFactionID);
    TArray<FSpatialQueryHandle> Result;
    Result.Reserve(GridHandles.Num());
    for (const FSparseGridHandle& H : GridHandles)
    {
        Result.Add(ToSpatialQueryHandle(H));
    }
    return Result;
}

TArray<FSpatialQueryHandle> USparseGridManager::QueryHandlesInBoxExcludingFactions(
    const FVector& Center,
    const FVector& Extent,
    int32 ExcludeFactionID) const
{
    TArray<FSparseGridHandle> GridHandles = QueryBoxExcludingFactions(Center, Extent, ExcludeFactionID);
    TArray<FSpatialQueryHandle> Result;
    Result.Reserve(GridHandles.Num());
    for (const FSparseGridHandle& H : GridHandles)
    {
        Result.Add(ToSpatialQueryHandle(H));
    }
    return Result;
}

IGridObjectInfo* USparseGridManager::GetObjectFromHandle(const FSpatialQueryHandle& Handle) const
{
    return GetObjectInterface(ToGridHandle(Handle));
}

IGridObjectInfo* USparseGridManager::GetObjectByID(int32 ObjectID) const
{
    FSparseGridHandle Handle = GetHandleByObjectID(ObjectID);
    if (!Handle.IsValid())
    {
        return nullptr;
    }
    return GetObjectInterface(Handle);
}

FSparseGridHandle USparseGridManager::GetHandleByObjectID(int32 ObjectID) const
{
    const FSparseGridHandle* Found = ObjectIDToHandleMap.Find(ObjectID);
    return Found ? *Found : FSparseGridHandle::Invalid;
}

void USparseGridManager::SetSparseGridConfig(USparseGridConfig* InConfig)
{
    SparseGridConfig = InConfig;

    if (!SparseGridConfig || !SparseGrid)
        return;

    SparseGrid->Initialize(SparseGridConfig->BaseRadius, SparseGridConfig->NumLevels);

    if (UFlowFieldManager* FlowFieldMgr = GetWorld()->GetSubsystem<UFlowFieldManager>())
    {
        // NavGrid使用独立的CellSize（BaseRadius * NavCellSizeMultiplier），而非SparseGrid CellSize
        float NavCellSize = SparseGridConfig->GetNavCellSize();
        FlowFieldMgr->ConfigureNavGrid(NavCellSize, SparseGridConfig->CoarsePathLevel);
    }

    if (UCrowdSurroundManager* CrowdSurroundManager = GetWorld()->GetSubsystem<UCrowdSurroundManager>())
    {
        CrowdSurroundManager->SetConfig(SparseGridConfig);
    }

    UE_LOG(LogTemp, Log, TEXT("SparseGridConfig applied: BaseRadius=%.2f, CellSize=%.2f, NumLevels=%d, CoarsePathLevel=%d"),
           SparseGridConfig->BaseRadius, SparseGrid->GetCellSize(), SparseGridConfig->NumLevels,
           SparseGridConfig->CoarsePathLevel);
}

TArray<USparseGrid::FCoarseCellInfo> USparseGridManager::QuerySphereAtLevel(const FVector& Center, float Radius, int32 Level) const
{
    if (!SparseGrid)
        return TArray<USparseGrid::FCoarseCellInfo>();

    return SparseGrid->QuerySphereAtLevel(Center, Radius, Level);
}

TArray<USparseGrid::FCoarseCellInfo> USparseGridManager::QueryBoxAtLevel(const FBox& Box, int32 Level) const
{
    if (!SparseGrid)
        return TArray<USparseGrid::FCoarseCellInfo>();

    return SparseGrid->QueryBoxAtLevel(Box, Level);
}

TArray<FSparseGridHandle> USparseGridManager::GetHandlesInCoarseCell(const FIntVector& CoarseCoord, int32 Level) const
{
    if (!SparseGrid)
        return TArray<FSparseGridHandle>();

    return SparseGrid->GetHandlesInCoarseCell(CoarseCoord, Level);
}

int32 USparseGridManager::GetCoarseCellObjectCount(const FIntVector& CoarseCoord, int32 Level) const
{
    if (!SparseGrid)
        return 0;

    return SparseGrid->GetCoarseCellObjectCount(CoarseCoord, Level);
}

void USparseGridManager::ComputeFlowFieldDirections()
{
    if (!SparseGrid)
        return;

    // CVar disable switch: bypass FlowField and Separation, use raw MovementInput
    if (SparseGridDebug::IsSteeringDisabled())
    {
        UCrowdSurroundManager* CrowdSurroundManager = GetWorld() ? GetWorld()->GetSubsystem<UCrowdSurroundManager>() : nullptr;
        FWriteScopeLock Lock(ObjectsLock);
        SteeringFlowResults.SetNum(Objects.Num());
        for (int32 i = 0; i < Objects.Num(); ++i)
        {
            FObjectEntry& Entry = Objects[i];
            FSteeringFlowResult& Result = SteeringFlowResults[i];
            Result = FSteeringFlowResult();

            Entry.ComputedVelocity = FVector::ZeroVector;
            if (!Entry.bActive || !Entry.ObjectInterface || !Entry.bEnableSteering)
                continue;

            FCrowdSurroundAssignment CurrentSurroundAssignment;
            if (CrowdSurroundManager
                && Entry.ObjectInterface->GetObjectID() != INDEX_NONE
                && CrowdSurroundManager->GetSurroundAssignment(Entry.ObjectInterface->GetObjectID(), CurrentSurroundAssignment)
                && CurrentSurroundAssignment.bLocksCrowdPosition)
            {
                Entry.SteeringForce = FVector::ZeroVector;
                Result.bSkipCorrections = true;
                continue;
            }

            float MoveSpeed = Entry.ObjectInterface->QueryMoveSpeed();
            FVector DesiredPos = Entry.ObjectInterface->QueryDesiredPosition();
            if (MoveSpeed > KINDA_SMALL_NUMBER && !DesiredPos.IsNearlyZero())
            {
                FVector Direction = (DesiredPos - Entry.Position).GetSafeNormal2D();
                Entry.ComputedVelocity = Direction * MoveSpeed;
            }
            Entry.SteeringForce = FVector::ZeroVector;
            Result.bSkipCorrections = true; // No corrections when steering is disabled
        }
        return;
    }

    UFlowFieldManager* FlowFieldMgr = GetWorld()->GetSubsystem<UFlowFieldManager>();

    float BaseRadius = SparseGridConfig ? SparseGridConfig->BaseRadius : 50.0f;
    float DirectMoveRadius = SparseGridConfig ? SparseGridConfig->GetLocalDirectMoveRadius() : BaseRadius * 3.0f;
    float ArrivalSlowRadius = SparseGridConfig ? SparseGridConfig->GetArrivalSlowRadius() : BaseRadius * 1.5f;
    float ArrivalStopRadius = SparseGridConfig ? SparseGridConfig->GetArrivalStopRadius() : BaseRadius * 0.8f;
    int32 FlowFieldGenerationBudget = SparseGridConfig ? FMath::Max(1, SparseGridConfig->MaxFlowFieldGenerationsPerFrame) : 4;

    ArrivalStopRadius = FMath::Max(0.0f, ArrivalStopRadius);
    ArrivalSlowRadius = FMath::Max(ArrivalStopRadius + 1.0f, ArrivalSlowRadius);

    FWriteScopeLock Lock(ObjectsLock);

    int32 FlowFieldsGeneratedThisFrame = 0;
    UCrowdSurroundManager* CrowdSurroundManager = GetWorld() ? GetWorld()->GetSubsystem<UCrowdSurroundManager>() : nullptr;

    // Lazy cache for surround assignments — avoids redundant queries per frame
    TMap<int32, FCrowdSurroundAssignment> SurroundAssignmentCache;
    TSet<int32> MissingSurroundAssignmentCache;

    auto GetCachedSurroundAssignment = [this, CrowdSurroundManager, &SurroundAssignmentCache, &MissingSurroundAssignmentCache](int32 ObjectIndex, FCrowdSurroundAssignment& OutAssignment) -> bool
    {
        if (!CrowdSurroundManager || !Objects.IsValidIndex(ObjectIndex))
        {
            OutAssignment = FCrowdSurroundAssignment();
            return false;
        }

        if (const FCrowdSurroundAssignment* CachedAssignment = SurroundAssignmentCache.Find(ObjectIndex))
        {
            OutAssignment = *CachedAssignment;
            return true;
        }

        if (MissingSurroundAssignmentCache.Contains(ObjectIndex))
        {
            OutAssignment = FCrowdSurroundAssignment();
            return false;
        }

        const FObjectEntry& CachedEntry = Objects[ObjectIndex];
        if (!CachedEntry.ObjectInterface)
        {
            MissingSurroundAssignmentCache.Add(ObjectIndex);
            OutAssignment = FCrowdSurroundAssignment();
            return false;
        }

        const int32 CachedObjectID = CachedEntry.ObjectInterface->GetObjectID();
        if (CachedObjectID == INDEX_NONE || !CrowdSurroundManager->GetSurroundAssignment(CachedObjectID, OutAssignment))
        {
            MissingSurroundAssignmentCache.Add(ObjectIndex);
            OutAssignment = FCrowdSurroundAssignment();
            return false;
        }

        SurroundAssignmentCache.Add(ObjectIndex, OutAssignment);
        return true;
    };

    SteeringFlowResults.SetNum(Objects.Num());

    for (int32 i = 0; i < Objects.Num(); ++i)
    {
        FObjectEntry& Entry = Objects[i];
        FSteeringFlowResult& Result = SteeringFlowResults[i];
        Result = FSteeringFlowResult();

        Entry.LastComputedVelocity = Entry.ComputedVelocity;
        Entry.ComputedVelocity = FVector::ZeroVector;

        if (!Entry.bActive || !Entry.ObjectInterface)
            continue;

        if (!Entry.bEnableSteering)
            continue;

        float MoveSpeed = Entry.ObjectInterface->QueryMoveSpeed();
        FVector DesiredPos = Entry.ObjectInterface->QueryDesiredPosition();
        bool bHasDesiredPosition = !DesiredPos.IsNearlyZero();

        // Check CrowdSurround assignment
        FCrowdSurroundAssignment CurrentSurroundAssignment;
        const bool bHasCurrentSurroundAssignment = GetCachedSurroundAssignment(i, CurrentSurroundAssignment);

        // Locked position → zero velocity, skip corrections
        if (bHasCurrentSurroundAssignment && CurrentSurroundAssignment.bLocksCrowdPosition)
        {
            Entry.ComputedVelocity = FVector::ZeroVector;
            Entry.SteeringForce = FVector::ZeroVector;
            Result.bSkipCorrections = true;
            continue;
        }

        // CrowdSurround correction is applied via CorrectedDesiredPosition
        // in SpatialGlue::TickCrowdSurroundCorrection, which flows through
        // QueryDesiredPosition(). No direct override needed here.

        const bool bContactApproach = bHasCurrentSurroundAssignment
            && CurrentSurroundAssignment.bShouldMove;

        // MoveSpeed=0: AI fully stopped, not affected by Separation
        if (MoveSpeed <= KINDA_SMALL_NUMBER)
        {
            Entry.ComputedVelocity = FVector::ZeroVector;
            if (SparseGridDebug::IsSteeringTargetDebugEnabled() && bHasDesiredPosition)
            {
                FVector From = Entry.Position;
                FVector To = DesiredPos;
                From.Z += 30.0f;
                To.Z += 30.0f;
                const FColor StoppedColor(160, 160, 160);
                DrawDebugLine(GetWorld(), From, To, StoppedColor, false, 0.02f, SDPG_World, 0.75f);
                DrawDebugSphere(GetWorld(), To, 6.0f, 8, StoppedColor, false, 0.02f, SDPG_World, 0.75f);
            }
            Result.bSkipCorrections = true;
            continue;
        }

        float DistanceToDesired = TNumericLimits<float>::Max();
        float ArrivalSpeedScale = 1.0f;
        float EffectiveArrivalStopRadius = ArrivalStopRadius;
        float EffectiveArrivalSlowRadius = ArrivalSlowRadius;
        if (bContactApproach)
        {
            const float TightStopRadius = 0.0f;
            EffectiveArrivalStopRadius = FMath::Min(EffectiveArrivalStopRadius, TightStopRadius);
            EffectiveArrivalSlowRadius = FMath::Max(
                EffectiveArrivalStopRadius + 1.0f,
                FMath::Min(EffectiveArrivalSlowRadius, BaseRadius * 0.3f));
        }

        if (bHasDesiredPosition)
        {
            FVector ToDesiredForArrival = DesiredPos - Entry.Position;
            ToDesiredForArrival.Z = 0.0f;
            DistanceToDesired = ToDesiredForArrival.Size();

            if (DistanceToDesired <= EffectiveArrivalStopRadius)
            {
                Entry.ComputedVelocity = FVector::ZeroVector;
                Entry.SteeringForce = FVector::ZeroVector;

                if (SparseGridDebug::IsSteeringTargetDebugEnabled())
                {
                    FVector From = Entry.Position;
                    FVector To = DesiredPos;
                    From.Z += 34.0f;
                    To.Z += 34.0f;
                    const FColor ArrivedColor(80, 160, 255);
                    DrawDebugLine(GetWorld(), From, To, ArrivedColor, false, 0.02f, SDPG_World, 0.75f);
                    DrawDebugSphere(GetWorld(), To, 7.0f, 8, ArrivedColor, false, 0.02f, SDPG_World, 0.75f);
                }
                Result.bSkipCorrections = true;
                continue;
            }

            if (DistanceToDesired < EffectiveArrivalSlowRadius)
            {
                const float SlowRange = FMath::Max(1.0f, EffectiveArrivalSlowRadius - EffectiveArrivalStopRadius);
                ArrivalSpeedScale = FMath::Clamp((DistanceToDesired - EffectiveArrivalStopRadius) / SlowRange, 0.15f, 1.0f);
            }
        }

        const float EffectiveMoveSpeed = MoveSpeed * ArrivalSpeedScale;

        FVector DirectionToDesired = FVector::ZeroVector;
        if (bHasDesiredPosition)
        {
            DirectionToDesired = DesiredPos - Entry.Position;
            DirectionToDesired.Z = 0.0f;
            DirectionToDesired = DirectionToDesired.GetSafeNormal2D();
        }

        // FlowField direction: pathfinding direction from current position to DesiredPosition
        FVector FlowDirection = FVector::ZeroVector;
        bool bUsedDirectMove = false;
        bool bHadFlowDirection = false;
        if (bHasDesiredPosition)
        {
            FVector ToDesired = DesiredPos - Entry.Position;
            ToDesired.Z = 0.0f;
            const float EffectiveDirectMoveRadius = (bHasCurrentSurroundAssignment && CurrentSurroundAssignment.DirectMoveRadius > KINDA_SMALL_NUMBER)
                ? CurrentSurroundAssignment.DirectMoveRadius
                : DirectMoveRadius;
            const float EffectiveDirectMoveRadiusSq = EffectiveDirectMoveRadius * EffectiveDirectMoveRadius;
            const bool bDirectPathWalkable = !FlowFieldMgr
                || FlowFieldMgr->IsSegmentWalkable(Entry.Position, DesiredPos, Entry.ObjectInterface->GetSparseGridCollisionRadius());
            if (ToDesired.SizeSquared() <= EffectiveDirectMoveRadiusSq && bDirectPathWalkable)
            {
                FlowDirection = ToDesired.GetSafeNormal2D();
                bUsedDirectMove = FlowDirection.SizeSquared() > KINDA_SMALL_NUMBER;
                bHadFlowDirection = bUsedDirectMove;
            }
            else if (FlowFieldMgr)
            {
                if (!FlowFieldMgr->GetFlowDirection(Entry.Position, DesiredPos, FlowDirection))
                {
                    if (FlowFieldsGeneratedThisFrame < FlowFieldGenerationBudget)
                    {
                        FFlowFieldResult UnusedResult;
                        FlowFieldMgr->GenerateFlowFieldSync(DesiredPos, UnusedResult);
                        FlowFieldMgr->GetFlowDirection(Entry.Position, DesiredPos, FlowDirection);
                        ++FlowFieldsGeneratedThisFrame;
                    }
                }
                bHadFlowDirection = FlowDirection.SizeSquared() > KINDA_SMALL_NUMBER;
            }
        }

        // Base velocity = FlowField direction × MoveSpeed
        FVector BaseVelocity = FVector::ZeroVector;
        if (FlowDirection.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            BaseVelocity = FlowDirection.GetSafeNormal2D() * EffectiveMoveSpeed;
        }

        // Store results for ApplySteeringCorrections
        Result.FlowDirection = FlowDirection;
        Result.BaseVelocity = BaseVelocity;
        Result.EffectiveMoveSpeed = EffectiveMoveSpeed;
        Result.DistanceToDesired = DistanceToDesired;
        Result.DirectionToDesired = DirectionToDesired;
        Result.bContactApproach = bContactApproach;
        Result.bHasDesiredPosition = bHasDesiredPosition;
        Result.bUsedDirectMove = bUsedDirectMove;
        Result.bSkipCorrections = false;
    }
}

void USparseGridManager::ApplySteeringCorrections()
{
    if (!SparseGrid || !SparseGridConfig)
        return;

    float BaseRadius = SparseGridConfig->BaseRadius;
    float SepWeight = SparseGridConfig->SeparationWeight;
    float SepRadius = SparseGridConfig->GetSeparationRadius();
    int32 MaxSepNeighbors = SparseGridConfig->MaxSeparationNeighbors;
    float MaxSpeed = SparseGridConfig->MaxSpeed;
    float MaxForce = SparseGridConfig->MaxForce;
    float ArrivalSlowRadius = SparseGridConfig->GetArrivalSlowRadius();
    float ArrivalStopRadius = SparseGridConfig->GetArrivalStopRadius();

    ArrivalStopRadius = FMath::Max(0.0f, ArrivalStopRadius);
    ArrivalSlowRadius = FMath::Max(ArrivalStopRadius + 1.0f, ArrivalSlowRadius);

    float SepRadiusSq = SepRadius * SepRadius;

    FWriteScopeLock Lock(ObjectsLock);

    if (SteeringFlowResults.Num() != Objects.Num())
        return;

    UCrowdSurroundManager* CrowdSurroundManager = GetWorld() ? GetWorld()->GetSubsystem<UCrowdSurroundManager>() : nullptr;

    // Rebuild surround assignment cache for neighbor yield checks
    TMap<int32, FCrowdSurroundAssignment> SurroundAssignmentCache;
    TSet<int32> MissingSurroundAssignmentCache;

    auto GetCachedSurroundAssignment = [this, CrowdSurroundManager, &SurroundAssignmentCache, &MissingSurroundAssignmentCache](int32 ObjectIndex, FCrowdSurroundAssignment& OutAssignment) -> bool
    {
        if (!CrowdSurroundManager || !Objects.IsValidIndex(ObjectIndex))
        {
            OutAssignment = FCrowdSurroundAssignment();
            return false;
        }

        if (const FCrowdSurroundAssignment* CachedAssignment = SurroundAssignmentCache.Find(ObjectIndex))
        {
            OutAssignment = *CachedAssignment;
            return true;
        }

        if (MissingSurroundAssignmentCache.Contains(ObjectIndex))
        {
            OutAssignment = FCrowdSurroundAssignment();
            return false;
        }

        const FObjectEntry& CachedEntry = Objects[ObjectIndex];
        if (!CachedEntry.ObjectInterface)
        {
            MissingSurroundAssignmentCache.Add(ObjectIndex);
            OutAssignment = FCrowdSurroundAssignment();
            return false;
        }

        const int32 CachedObjectID = CachedEntry.ObjectInterface->GetObjectID();
        if (CachedObjectID == INDEX_NONE || !CrowdSurroundManager->GetSurroundAssignment(CachedObjectID, OutAssignment))
        {
            MissingSurroundAssignmentCache.Add(ObjectIndex);
            OutAssignment = FCrowdSurroundAssignment();
            return false;
        }

        SurroundAssignmentCache.Add(ObjectIndex, OutAssignment);
        return true;
    };

    auto GetSurroundIngressPriority = [](const FCrowdSurroundAssignment& Assignment) -> int32
    {
        // Locked agents have highest priority (0), agents in position next (1), moving agents last (2)
        if (Assignment.bLocksCrowdPosition)  return 0;
        if (Assignment.bIsInPosition)        return 1;
        return 2;
    };

    for (int32 i = 0; i < Objects.Num(); ++i)
    {
        FObjectEntry& Entry = Objects[i];

        if (!Entry.bActive || !Entry.ObjectInterface || !Entry.bEnableSteering)
            continue;

        const FSteeringFlowResult& FlowResult = SteeringFlowResults[i];

        // Skip objects that were already zeroed (locked, zero speed, or arrived)
        if (FlowResult.bSkipCorrections)
            continue;

        // Re-derive current object's surround assignment for yield logic
        FCrowdSurroundAssignment CurrentSurroundAssignment;
        const bool bHasCurrentSurroundAssignment = GetCachedSurroundAssignment(i, CurrentSurroundAssignment);

        auto ShouldNeighborYieldToCurrent = [&GetCachedSurroundAssignment, &GetSurroundIngressPriority, &CurrentSurroundAssignment, bHasCurrentSurroundAssignment](int32 NeighborIndex) -> bool
        {
            if (!bHasCurrentSurroundAssignment || !CurrentSurroundAssignment.bShouldMove)
            {
                return false;
            }

            FCrowdSurroundAssignment NeighborAssignment;
            if (!GetCachedSurroundAssignment(NeighborIndex, NeighborAssignment) || !NeighborAssignment.bHasAssignment)
            {
                return false;
            }

            const int32 CurrentPriority = GetSurroundIngressPriority(CurrentSurroundAssignment);
            const int32 NeighborPriority = GetSurroundIngressPriority(NeighborAssignment);
            if (CurrentPriority >= NeighborPriority)
            {
                return false;
            }

            return !NeighborAssignment.bShouldMove || CurrentPriority + 1 <= NeighborPriority;
        };

        auto IsBeyondContactDestination = [FlowResult, BaseRadius](const FVector& ToNeighbor) -> bool
        {
            if (!FlowResult.bContactApproach || !FlowResult.bHasDesiredPosition || FlowResult.DirectionToDesired.SizeSquared() <= KINDA_SMALL_NUMBER)
            {
                return false;
            }

            const float ForwardDistance = FVector::DotProduct(ToNeighbor, FlowResult.DirectionToDesired);
            return ForwardDistance > FlowResult.DistanceToDesired + FMath::Max(1.0f, BaseRadius * 0.1f);
        };

        // Read flow field results
        FVector FlowDirection = FlowResult.FlowDirection;
        FVector BaseVelocity = FlowResult.BaseVelocity;
        float EffectiveMoveSpeed = FlowResult.EffectiveMoveSpeed;
        float DistanceToDesired = FlowResult.DistanceToDesired;

        // Recompute arrival radii for MaxAllowedSpeed
        float EffectiveArrivalStopRadius = ArrivalStopRadius;
        float EffectiveArrivalSlowRadius = ArrivalSlowRadius;
        if (FlowResult.bContactApproach)
        {
            const float TightStopRadius = 0.0f;
            EffectiveArrivalStopRadius = FMath::Min(EffectiveArrivalStopRadius, TightStopRadius);
            EffectiveArrivalSlowRadius = FMath::Max(
                EffectiveArrivalStopRadius + 1.0f,
                FMath::Min(EffectiveArrivalSlowRadius, BaseRadius * 0.3f));
        }

        // === Three-layer steering forces ===
        FVector ObstacleAvoidanceForce = FVector::ZeroVector;
        FVector SeparationForce = FVector::ZeroVector;
        FVector HardSeparationForce = FVector::ZeroVector;
        FVector AlignmentForce = FVector::ZeroVector;
        int32 ObstacleCount = 0;
        int32 SeparationCount = 0;
        int32 HardSepCount = 0;
        int32 AlignmentCount = 0;
        float HardSepRadius = BaseRadius * 2.0f;
        float HardSepRadiusSq = HardSepRadius * HardSepRadius;
        float ComfortDist = BaseRadius * 2.2f;

        // Dynamic avoidance: candidate angle penalty evaluation
        TArray<FSparseGridHandle> Neighbors = SparseGrid->QuerySphere(Entry.Position, SepRadius);
        Neighbors.Sort([this, &Entry](const FSparseGridHandle& A, const FSparseGridHandle& B)
        {
            const bool bAValid = IsIndexValid(A.GetIndex());
            const bool bBValid = IsIndexValid(B.GetIndex());
            if (bAValid != bBValid)
            {
                return bAValid;
            }
            if (!bAValid)
            {
                return false;
            }

            const float DistASq = FVector::DistSquared2D(Objects[A.GetIndex()].Position, Entry.Position);
            const float DistBSq = FVector::DistSquared2D(Objects[B.GetIndex()].Position, Entry.Position);
            return DistASq < DistBSq;
        });

        if (FlowDirection.SizeSquared() > KINDA_SMALL_NUMBER && Neighbors.Num() > 1)
        {
            const FVector PreferredDirection = FlowDirection.GetSafeNormal2D();
            const float SelfRadius = FMath::Max(Entry.ObjectInterface->GetSparseGridCollisionRadius(), BaseRadius);
            const float Padding = SparseGridConfig ? SparseGridConfig->GetCrowdContactPadding() : BaseRadius * 0.2f;
            const float LookAhead = SparseGridConfig
                ? FMath::Max(SparseGridConfig->GetDynamicAvoidanceLookAhead(), BaseRadius * 3.0f)
                : BaseRadius * 5.0f;
            const float PreferredSide = (i % 2 == 0) ? 1.0f : -1.0f;
            const float CandidateAngles[] = { 0.0f, 22.5f, -22.5f, 45.0f, -45.0f, 70.0f, -70.0f, 100.0f, -100.0f };

            FVector BestDirection = PreferredDirection;
            float BestPenalty = TNumericLimits<float>::Max();

            for (float CandidateAngle : CandidateAngles)
            {
                const float SignedAngle = CandidateAngle * PreferredSide;
                const FVector CandidateDirection = PreferredDirection.RotateAngleAxis(SignedAngle, FVector::UpVector).GetSafeNormal2D();
                const float DirectionDot = FMath::Clamp(FVector::DotProduct(CandidateDirection, PreferredDirection), -1.0f, 1.0f);
                float CandidatePenalty = (1.0f - DirectionDot) * 250.0f + FMath::Abs(SignedAngle) * 0.5f;

                for (const FSparseGridHandle& NeighborHandle : Neighbors)
                {
                    if (NeighborHandle.GetIndex() == i && NeighborHandle.GetGeneration() == Entry.Generation)
                    {
                        continue;
                    }

                    if (!IsIndexValid(NeighborHandle.GetIndex()))
                    {
                        continue;
                    }

                    const FObjectEntry& NeighborEntry = Objects[NeighborHandle.GetIndex()];
                    if (!NeighborEntry.bActive || !NeighborEntry.ObjectInterface)
                    {
                        continue;
                    }

                    const bool bNeighborYieldsToCurrent = ShouldNeighborYieldToCurrent(NeighborHandle.GetIndex());
                    FVector ToNeighbor = NeighborEntry.Position - Entry.Position;
                    ToNeighbor.Z = 0.0f;
                    const float ForwardDistance = FVector::DotProduct(ToNeighbor, CandidateDirection);
                    if (ForwardDistance <= 0.0f || ForwardDistance > LookAhead)
                    {
                        continue;
                    }

                    if (FlowResult.bContactApproach && ForwardDistance > DistanceToDesired + FMath::Max(1.0f, SelfRadius * 0.1f))
                    {
                        continue;
                    }

                    const FVector ClosestPoint = CandidateDirection * ForwardDistance;
                    const float LateralDistanceSq = (ToNeighbor - ClosestPoint).SizeSquared2D();
                    const float NeighborRadius = FMath::Max(NeighborEntry.ObjectInterface->GetSparseGridCollisionRadius(), BaseRadius);
                    const float RequiredClearance = SelfRadius + NeighborRadius + Padding;
                    const float EffectiveRequiredClearance = bNeighborYieldsToCurrent
                        ? FMath::Max(SelfRadius * 0.5f, RequiredClearance * 0.35f)
                        : RequiredClearance;
                    const float EffectiveRequiredClearanceSq = EffectiveRequiredClearance * EffectiveRequiredClearance;
                    if (LateralDistanceSq >= EffectiveRequiredClearanceSq)
                    {
                        continue;
                    }

                    const float LateralDistance = FMath::Sqrt(FMath::Max(0.0f, LateralDistanceSq));
                    const float BlockRatio = 1.0f - LateralDistance / EffectiveRequiredClearance;
                    const float ForwardBias = 1.0f - ForwardDistance / LookAhead;
                    const bool bNavigationObstacle = NeighborEntry.ObjectInterface->IsNavigationObstacle();
                    const FVector NeighborVelocity = NeighborEntry.ObjectInterface->QueryMovementVelocity();
                    const float SameDirection = NeighborVelocity.SizeSquared() > KINDA_SMALL_NUMBER
                        ? FVector::DotProduct(NeighborVelocity.GetSafeNormal2D(), CandidateDirection)
                        : 0.0f;
                    const float BlockWeight = bNeighborYieldsToCurrent
                        ? 80.0f
                        : (bNavigationObstacle ? 1400.0f : (SameDirection > 0.5f ? 350.0f : 700.0f));
                    const float ForwardBiasWeight = bNeighborYieldsToCurrent ? 30.0f : 200.0f;
                    CandidatePenalty += BlockWeight * BlockRatio + ForwardBias * ForwardBiasWeight;
                }

                if (CandidatePenalty < BestPenalty)
                {
                    BestPenalty = CandidatePenalty;
                    BestDirection = CandidateDirection;
                }
            }

            FlowDirection = BestDirection;
            BaseVelocity = FlowDirection * EffectiveMoveSpeed;
        }

        // Separation / Alignment / HardSeparation forces
        for (const FSparseGridHandle& NeighborHandle : Neighbors)
        {
            if (NeighborHandle.GetIndex() == i && NeighborHandle.GetGeneration() == Entry.Generation)
                continue;

            if (!IsIndexValid(NeighborHandle.GetIndex()))
                continue;

            const FObjectEntry& NeighborEntry = Objects[NeighborHandle.GetIndex()];
            if (!NeighborEntry.bActive || !NeighborEntry.ObjectInterface)
                continue;

            const bool bNeighborYieldsToCurrent = ShouldNeighborYieldToCurrent(NeighborHandle.GetIndex());
            const float YieldSeparationScale = bNeighborYieldsToCurrent ? 0.2f : 1.0f;

            FVector ToNeighbor = NeighborEntry.Position - Entry.Position;
            float CurrentDistSq = ToNeighbor.SizeSquared();
            if (CurrentDistSq < KINDA_SMALL_NUMBER || CurrentDistSq > SepRadiusSq)
                continue;

            float CurrentDist = FMath::Sqrt(CurrentDistSq);

            // Hard separation at close range
            if (CurrentDistSq < HardSepRadiusSq)
            {
                float Overlap = 1.0f - (CurrentDist / HardSepRadius);
                HardSeparationForce += (-ToNeighbor.GetSafeNormal()) * Overlap * YieldSeparationScale;
                HardSepCount++;
            }

            if (IsBeyondContactDestination(ToNeighbor))
            {
                continue;
            }

            if (NeighborEntry.ObjectInterface->IsNavigationObstacle())
            {
                FVector Diff = -ToNeighbor;
                ObstacleAvoidanceForce += Diff.GetSafeNormal() / CurrentDist;
                ObstacleCount++;
            }
            else
            {
                FVector NeighborVelocity = NeighborEntry.ObjectInterface->QueryMovementVelocity();
                FVector RelVel = BaseVelocity - NeighborVelocity;
                float ApproachSpeed = RelVel | ToNeighbor.GetSafeNormal();

                if (ApproachSpeed <= 0.0f && CurrentDist >= ComfortDist)
                {
                    if (NeighborVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
                    {
                        AlignmentForce += NeighborVelocity.GetSafeNormal2D();
                        AlignmentCount++;
                    }
                    continue;
                }

                FVector SepDir = (-ToNeighbor).GetSafeNormal() / CurrentDist;
                SeparationForce += SepDir * YieldSeparationScale;
                SeparationCount++;

                if (NeighborVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
                {
                    AlignmentForce += NeighborVelocity.GetSafeNormal2D();
                    AlignmentCount++;
                }
            }

            if (ObstacleCount + SeparationCount >= MaxSepNeighbors)
                break;
        }

        float AlignWeight = SparseGridConfig ? SparseGridConfig->AlignmentWeight : 1.0f;

        FVector SteeringAccumulator = FVector::ZeroVector;

        if (ObstacleAvoidanceForce.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            ObstacleAvoidanceForce = ObstacleAvoidanceForce.GetSafeNormal() * MaxSpeed;
            SteeringAccumulator += ObstacleAvoidanceForce * (SepWeight * 2.0f);
        }

        if (SeparationForce.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            SeparationForce = SeparationForce.GetSafeNormal() * MaxSpeed;
            SteeringAccumulator += SeparationForce * SepWeight;
        }

        if (AlignmentCount > 0 && AlignmentForce.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            AlignmentForce = AlignmentForce.GetSafeNormal() * MaxSpeed;
            SteeringAccumulator += AlignmentForce * AlignWeight;
        }

        if (HardSeparationForce.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            HardSeparationForce = HardSeparationForce.GetSafeNormal() * MaxSpeed;
            SteeringAccumulator += HardSeparationForce * (SepWeight * 3.0f);
        }

        if (SteeringAccumulator.SizeSquared() > MaxForce * MaxForce)
        {
            SteeringAccumulator = SteeringAccumulator.GetSafeNormal() * MaxForce;
        }

        const float MoveSpeed = Entry.ObjectInterface->QueryMoveSpeed();
        const float MaxAllowedSpeed = (FlowResult.bHasDesiredPosition && DistanceToDesired < EffectiveArrivalSlowRadius)
            ? FMath::Min(MaxSpeed, FMath::Max(EffectiveMoveSpeed, MoveSpeed * 0.2f))
            : MaxSpeed;

        FVector Velocity = BaseVelocity + SteeringAccumulator;
        Velocity.Z = 0.0f;
        float FinalSpeed = Velocity.Size();
        if (FinalSpeed > MaxAllowedSpeed)
        {
            Velocity = Velocity.GetSafeNormal() * MaxAllowedSpeed;
        }

        // Velocity smoothing: exponential moving average to prevent jitter
        constexpr float VelocitySmoothing = 0.3f;
        Velocity = FMath::Lerp(Velocity, Entry.LastComputedVelocity, VelocitySmoothing);
        FinalSpeed = Velocity.Size();
        if (FinalSpeed > MaxAllowedSpeed)
        {
            Velocity = Velocity.GetSafeNormal() * MaxAllowedSpeed;
        }

        Entry.ComputedVelocity = Velocity;
        Entry.SteeringForce = FVector::ZeroVector;

        if (SparseGridDebug::IsSteeringTargetDebugEnabled() && FlowResult.bHasDesiredPosition)
        {
            FVector DesiredPos = Entry.ObjectInterface->QueryDesiredPosition();
            FVector From = Entry.Position;
            FVector To = DesiredPos;
            From.Z += 34.0f;
            To.Z += 34.0f;

            const FColor TargetColor = FlowResult.bUsedDirectMove ? FColor::Green : (FlowDirection.SizeSquared() > KINDA_SMALL_NUMBER ? FColor::Cyan : FColor::Red);
            DrawDebugLine(GetWorld(), From, To, TargetColor, false, 0.02f, SDPG_World, 1.0f);
            DrawDebugSphere(GetWorld(), To, 8.0f, 8, TargetColor, false, 0.02f, SDPG_World, 1.0f);

            if (Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
            {
                DrawDebugDirectionalArrow(GetWorld(), From, From + Velocity.GetSafeNormal2D() * BaseRadius, BaseRadius * 0.25f, FColor::Yellow, false, 0.02f, SDPG_World, 1.0f);
            }
        }
    }
}

FVector USparseGridManager::GetComputedVelocity(const FSparseGridHandle& Handle) const
{
    FReadScopeLock Lock(ObjectsLock);
    if (!ValidateHandle(Handle, TEXT("GetComputedVelocity")))
        return FVector::ZeroVector;

    return Objects[Handle.GetIndex()].ComputedVelocity;
}
