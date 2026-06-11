#include "ProjectileManager.h"
#include "ProjectileDebug.h"
#include "SpatialQueryService.h"
#include "DamageService.h"
#include "GridObjectInfo.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Async/ParallelFor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "DrawDebugHelpers.h"

void UProjectileManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    InvalidTransform = FTransform(FVector(0, 0, INVALID_POSITION_OFFSET));

    EnsureDependencies();
    LoadActorFromConfig();

    UE_LOG(LogTemp, Log, TEXT("ProjectileManager initialized (capacity: %d)"), INITIAL_CAPACITY);
}

void UProjectileManager::Deinitialize()
{
    if (ProjectileActor)
    {
        ProjectileActor->Destroy();
        ProjectileActor = nullptr;
    }

    HISMComponent = nullptr;
    Projectiles.Empty();
    FreeIndices.Empty();
    ProjectileIDToIndex.Empty();
    StateBits.ClearAll();
    IndexCache.Clear();

    Super::Deinitialize();
}

bool UProjectileManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UProjectileManager::EnsureDependencies()
{
    if (!SpatialQuery)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProjectileManager: SpatialQueryService not injected"));
    }
    if (!DamageService)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProjectileManager: DamageService not injected"));
    }
}

void UProjectileManager::LoadActorFromConfig()
{
    FString ConfigPath = TEXT("/Script/ProjectilePlugin.ProjectileSettings");

    FString ActorPath;
    if (!GConfig || !GConfig->GetString(*ConfigPath, TEXT("ProjectileActorPath"), ActorPath, GGameIni))
    {
        UE_LOG(LogTemp, Log, TEXT("ProjectileManager: No INI config found for ProjectileActorPath"));
        return;
    }

    ProjectileActorClass = LoadClass<AActor>(nullptr, *ActorPath);
    if (!ProjectileActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProjectileManager: Failed to load Blueprint: %s"), *ActorPath);
        return;
    }

    if (SpawnActorFromBlueprint())
    {
        PreAllocateInstances(INITIAL_CAPACITY);
    }
}

bool UProjectileManager::SpawnActorFromBlueprint()
{
    UWorld* World = GetWorld();
    if (!World || !ProjectileActorClass)
    {
        return false;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(TEXT("ProjectileActor"));
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ProjectileActor = World->SpawnActor<AActor>(ProjectileActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    if (!ProjectileActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProjectileManager: Failed to spawn ProjectileActor"));
        return false;
    }

    HISMComponent = ProjectileActor->FindComponentByClass<UHierarchicalInstancedStaticMeshComponent>();
    if (!HISMComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProjectileManager: No HISMComponent found in Blueprint"));
        ProjectileActor->Destroy();
        ProjectileActor = nullptr;
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("ProjectileManager: Initialized from Blueprint"));
    return true;
}

void UProjectileManager::PreAllocateInstances(int32 Count)
{
    if (!HISMComponent) return;

    int32 OldCount = Projectiles.Num();

    TArray<FTransform> Transforms;
    Transforms.Reserve(Count - OldCount);

    for (int32 i = OldCount; i < Count; i++)
    {
        Transforms.Add(InvalidTransform);
    }

    HISMComponent->AddInstances(Transforms, false, false, false);

    Projectiles.SetNum(Count);
    StateBits.ResizeAll(OldCount, Count);

    for (int32 i = OldCount; i < Count; i++)
    {
        Projectiles[i].InstanceIndex = i;
        FreeIndices.Add(i);
    }
}

void UProjectileManager::Tick(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(UProjectileManager_Tick);

    if (Projectiles.Num() == 0) return;

    CollectActiveIndices();

    if (IndexCache.ActiveIndices.Num() == 0) return;

    ParallelComputeMovement(DeltaTime);
    ParallelSweepCollisionDetection();
    ProcessHitResults();
    FlushTransformUpdates();
    CollectDeactivateIndices();
    ProcessPendingDeactivations();

    DrawActiveTrajectories();
}

void UProjectileManager::CollectActiveIndices()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_CollectActive);

    IndexCache.ActiveIndices.Empty();

    for (TBitArray<>::FConstIterator It(StateBits.IsActive); It; ++It)
    {
        if (It.GetValue())
        {
            IndexCache.ActiveIndices.Add(It.GetIndex());
        }
    }
}

void UProjectileManager::CollectPhaseIndices()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_CollectPhase);

    IndexCache.FlyingIndices.Empty();
    IndexCache.CollisionPhaseIndices.Empty();

    for (int32 Index : IndexCache.ActiveIndices)
    {
        if (StateBits.IsFlying[Index])
        {
            IndexCache.FlyingIndices.Add(Index);
        }
        else if (StateBits.IsInCollisionPhase[Index])
        {
            IndexCache.CollisionPhaseIndices.Add(Index);
        }
    }
}

void UProjectileManager::CollectDeactivateIndices()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_CollectDeactivate);

    IndexCache.DeactivateIndices.Empty();

    for (TBitArray<>::FConstIterator It(StateBits.PendingDeactivate); It; ++It)
    {
        if (It.GetValue())
        {
            IndexCache.DeactivateIndices.Add(It.GetIndex());
        }
    }
}

void UProjectileManager::ParallelComputeMovement(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_ComputeMovement);

    const TArray<int32>& ActiveIndices = IndexCache.ActiveIndices;

    ParallelFor(ActiveIndices.Num(), [this, DeltaTime, &ActiveIndices](int32 ArrayIndex)
    {
        int32 Index = ActiveIndices[ArrayIndex];
        FProjectileData& Data = Projectiles[Index];

        if (StateBits.IsFlying[Index])
        {
            ProcessFlyingPhase(Data, Index, DeltaTime);
        }
        else if (StateBits.IsInCollisionPhase[Index])
        {
            ProcessCollisionPhase(Data, Index, DeltaTime);
        }

        if (Data.bPendingDestroy && StateBits.IsFlying[Index])
        {
            Data.DestroyTimer -= DeltaTime;
            if (Data.DestroyTimer <= 0.0f)
            {
                TransitionToDestroyedPhase(Index);
            }
        }

        Data.Lifetime -= DeltaTime;
        if (Data.Lifetime <= 0.0f)
        {
            TransitionToDestroyedPhase(Index);
        }

        StateBits.NeedsTransformUpdate[Index] = true;
    });
}

void UProjectileManager::ProcessFlyingPhase(FProjectileData& Data, int32 Index, float DeltaTime)
{
    Data.PrevPosition = Data.Position;
    Data.ElapsedTime += DeltaTime;

    while (Data.CurrentPointIndex < Data.TrajectoryTimes.Num() - 1)
    {
        if (Data.ElapsedTime < Data.TrajectoryTimes[Data.CurrentPointIndex + 1])
        {
            break;
        }
        Data.CurrentPointIndex++;
    }

    if (Data.CurrentPointIndex < Data.TrajectoryPoints.Num() - 1)
    {
        float T0 = Data.TrajectoryTimes[Data.CurrentPointIndex];
        float T1 = Data.TrajectoryTimes[Data.CurrentPointIndex + 1];
        float Alpha = (T1 > T0) ? (Data.ElapsedTime - T0) / (T1 - T0) : 0.0f;
        Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

        FVector P0 = Data.TrajectoryPoints[Data.CurrentPointIndex];
        FVector P1 = Data.TrajectoryPoints[Data.CurrentPointIndex + 1];
        Data.Position = FMath::Lerp(P0, P1, Alpha);
        Data.Direction = (P1 - P0).GetSafeNormal();
        Data.Velocity = Data.Direction * Data.Speed;
    }
    else
    {
        Data.Position = Data.TrajectoryPoints.Last();
        TransitionToCollisionPhase(Index);
    }

    if (Data.ShouldEnterCollisionPhase())
    {
        TransitionToCollisionPhase(Index);
    }
}

void UProjectileManager::ProcessCollisionPhase(FProjectileData& Data, int32 Index, float DeltaTime)
{
    if (Data.bPendingDestroy)
    {
        Data.DestroyTimer -= DeltaTime;
        if (Data.DestroyTimer <= 0.0f)
        {
            TransitionToDestroyedPhase(Index);
        }
        return;
    }

    FVector Acceleration(0, 0, -980.0f);

    Data.Velocity += Acceleration * DeltaTime;
    Data.Position += Data.Velocity * DeltaTime;

    if (Data.Position.Z <= Data.GroundHeight)
    {
        Data.Position.Z = Data.GroundHeight;
        Data.Velocity = FVector::ZeroVector;
        Data.StartDestroyTimer();
        StateBits.PendingDestroy[Index] = true;
    }
}

void UProjectileManager::TransitionToCollisionPhase(int32 Index)
{
    StateBits.IsFlying[Index] = false;
    StateBits.IsInCollisionPhase[Index] = true;
    StateBits.DamageApplied[Index] = false;
    Projectiles[Index].Phase = EProjectilePhase::Collision;
}

void UProjectileManager::TransitionToDestroyedPhase(int32 Index)
{
    StateBits.IsActive[Index] = false;
    StateBits.IsFlying[Index] = false;
    StateBits.IsInCollisionPhase[Index] = false;
    StateBits.PendingDeactivate[Index] = true;
    Projectiles[Index].Phase = EProjectilePhase::Destroyed;
}

void UProjectileManager::ParallelCollisionDetection()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_CollisionDetection);

    if (!SpatialQuery) return;

    const TArray<int32>& CollisionIndices = IndexCache.CollisionPhaseIndices;

    TArray<FProjectileHitResult> LocalHitResults;
    LocalHitResults.SetNum(CollisionIndices.Num());

    ParallelFor(CollisionIndices.Num(), [this, &CollisionIndices, &LocalHitResults](int32 ArrayIndex)
    {
        int32 Index = CollisionIndices[ArrayIndex];
        FProjectileData& Data = Projectiles[Index];
        FProjectileHitResult& HitResult = LocalHitResults[ArrayIndex];

        HitResult.InstanceIndex = Index;
        HitResult.ProjectileID = Data.ProjectileID;
        HitResult.bIsValid = false;

        if (StateBits.DamageApplied[Index] || StateBits.PendingDestroy[Index])
        {
            return;
        }

        TArray<IGridObjectInfo*> NearbyObjects = QueryTargetsInSphere(
            Data.Position, Data.CollisionRadius * 2.0f, Data.Faction);

        for (IGridObjectInfo* Object : NearbyObjects)
        {
            if (!Object) continue;

            AActor* Actor = Object->GetOwnerActor();
            if (!Actor) continue;
            if (Actor == Data.Instigator.Get()) continue;
            if (Actor == Data.LastHitActor.Get()) continue;

            FVector ObjectPos = Object->GetGridPosition();
            float ObjectRadius = Object->GetCollisionRadius();
            float CombinedRadius = Data.CollisionRadius + ObjectRadius;
            float DistSq = FVector::DistSquared(Data.Position, ObjectPos);

            if (DistSq < CombinedRadius * CombinedRadius)
            {
                HitResult.bIsValid = true;
                HitResult.HitActor = Actor;
                HitResult.HitLocation = Data.Position;
                HitResult.HitNormal = (Data.Position - ObjectPos).GetSafeNormal();
                HitResult.Damage = Data.Damage;
                break;
            }
        }
    });

    for (const auto& Result : LocalHitResults)
    {
        if (Result.bIsValid)
        {
            PendingHitResults.Add(Result);
        }
    }
}

void UProjectileManager::ParallelSweepCollisionDetection()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_SweepCollisionDetection);

    if (!SpatialQuery) return;

    const TArray<int32>& ActiveIndices = IndexCache.ActiveIndices;

    TArray<FProjectileHitResult> LocalHitResults;
    LocalHitResults.SetNum(ActiveIndices.Num());

    ParallelFor(ActiveIndices.Num(), [this, &ActiveIndices, &LocalHitResults](int32 ArrayIndex)
    {
        int32 Index = ActiveIndices[ArrayIndex];
        FProjectileData& Data = Projectiles[Index];
        FProjectileHitResult& HitResult = LocalHitResults[ArrayIndex];

        HitResult.InstanceIndex = Index;
        HitResult.ProjectileID = Data.ProjectileID;
        HitResult.bIsValid = false;

        if (!StateBits.IsFlying[Index] || StateBits.DamageApplied[Index] || StateBits.PendingDestroy[Index])
        {
            return;
        }

        FVector MoveDelta = Data.Position - Data.PrevPosition;
        float MoveDistance = MoveDelta.Size();

        if (MoveDistance < UE_KINDA_SMALL_NUMBER)
        {
            return;
        }

        float StepLength = Data.CollisionRadius * SWEEP_STEP_MULTIPLIER;
        int32 NumSubSteps = FMath::CeilToInt(MoveDistance / StepLength);
        NumSubSteps = FMath::Clamp(NumSubSteps, 1, MAX_SWEEP_SUBSTEPS);

        FVector StepDir = MoveDelta.GetSafeNormal();
        float StepSize = MoveDistance / NumSubSteps;

        for (int32 Step = 0; Step < NumSubSteps; Step++)
        {
            FVector StepStart = Data.PrevPosition + StepDir * (Step * StepSize);
            FVector StepEnd = Data.PrevPosition + StepDir * ((Step + 1) * StepSize);

            TArray<IGridObjectInfo*> NearbyObjects = QueryTargetsInCapsule(
                StepStart, StepEnd, Data.CollisionRadius, Data.Faction);

            for (IGridObjectInfo* Object : NearbyObjects)
            {
                if (!Object) continue;

                AActor* Actor = Object->GetOwnerActor();
                if (!Actor) continue;
                if (Actor == Data.Instigator.Get()) continue;
                if (Actor == Data.LastHitActor.Get()) continue;

                FVector ObjectPos = Object->GetGridPosition();
                float ObjectRadius = Object->GetCollisionRadius();

                float DistSq = FMath::PointDistToSegmentSquared(ObjectPos, StepStart, StepEnd);
                float CombinedRadius = Data.CollisionRadius + ObjectRadius;

                if (DistSq < CombinedRadius * CombinedRadius)
                {
                    HitResult.bIsValid = true;
                    HitResult.HitActor = Actor;
                    HitResult.HitLocation = Data.Position;
                    HitResult.HitNormal = (Data.Position - ObjectPos).GetSafeNormal();
                    HitResult.Damage = Data.Damage;
                    return;
                }
            }
        }
    });

    for (const auto& Result : LocalHitResults)
    {
        if (Result.bIsValid)
        {
            PendingHitResults.Add(Result);
        }
    }
}

TArray<IGridObjectInfo*> UProjectileManager::QueryTargetsInCapsule(
    const FVector& Start, const FVector& End, float Radius, int32 ExcludeFaction)
{
    if (!SpatialQuery) return {};

    TArray<IGridObjectInfo*> Results;

    FVector Center = (Start + End) * 0.5f;
    FVector Direction = End - Start;
    float HalfHeight = Direction.Size() * 0.5f + Radius;
    FBox QueryBox = FBox::BuildAABB(Center, FVector(Radius, Radius, HalfHeight));
    FVector Extent = QueryBox.GetExtent();

    TArray<FSpatialQueryHandle> Handles = SpatialQuery->QueryHandlesInBoxExcludingFactions(Center, Extent, ExcludeFaction);
    for (const FSpatialQueryHandle& Handle : Handles)
    {
        if (IGridObjectInfo* Object = SpatialQuery->GetObjectFromHandle(Handle))
        {
            Results.Add(Object);
        }
    }

    return Results;
}

TArray<IGridObjectInfo*> UProjectileManager::QueryTargetsInSphere(
    const FVector& Center, float Radius, int32 ExcludeFaction)
{
    if (!SpatialQuery) return {};

    TArray<IGridObjectInfo*> Results;
    TArray<FSpatialQueryHandle> Handles = SpatialQuery->QueryHandlesInSphereExcludingFactions(Center, Radius, ExcludeFaction);

    for (const FSpatialQueryHandle& Handle : Handles)
    {
        if (IGridObjectInfo* Object = SpatialQuery->GetObjectFromHandle(Handle))
        {
            Results.Add(Object);
        }
    }

    return Results;
}

void UProjectileManager::ProcessHitResults()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_ProcessHitResults);

    for (const FProjectileHitResult& Result : PendingHitResults)
    {
        int32 Index = Result.InstanceIndex;
        if (!Projectiles.IsValidIndex(Index)) continue;

        FProjectileData& Data = Projectiles[Index];

        if (!StateBits.IsActive[Index] || StateBits.DamageApplied[Index]) continue;

        if (Result.HitActor)
        {
            ApplyDamageToTarget(Index, Result.HitActor);
            StateBits.DamageApplied[Index] = true;
            Data.bDamageApplied = true;
            Data.LastHitActor = Result.HitActor;

            Data.StartDestroyTimer();
            StateBits.PendingDestroy[Index] = true;
        }

        OnProjectileHit.Broadcast(Result);
    }

    PendingHitResults.Empty();
}

void UProjectileManager::ApplyDamageToTarget(int32 Index, AActor* Target)
{
    if (!DamageService || !Target) return;

    FProjectileData& Data = Projectiles[Index];

    DamageService->ApplyDamageHit(Target, Data.Damage, EDamageType::Physical, Data.Instigator.Get(), Data.Position, true);
}

void UProjectileManager::FlushTransformUpdates()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_FlushTransforms);

    if (!HISMComponent) return;

    const TArray<int32>& ActiveIndices = IndexCache.ActiveIndices;

    for (int32 Index : ActiveIndices)
    {
        if (StateBits.NeedsTransformUpdate[Index])
        {
            FProjectileData& Data = Projectiles[Index];

            if (StateBits.IsActive[Index])
            {
                FTransform Transform = CalculateTransform(Data);
                HISMComponent->UpdateInstanceTransform(Index, Transform, true, false, true);
            }
            else
            {
                HISMComponent->UpdateInstanceTransform(Index, InvalidTransform, true, false, true);
            }
        }
    }

    StateBits.NeedsTransformUpdate.Init(false, StateBits.NeedsTransformUpdate.Num());
}

void UProjectileManager::ProcessPendingDeactivations()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(Projectile_ProcessDeactivations);

    const TArray<int32>& DeactivateIndices = IndexCache.DeactivateIndices;

    for (int32 Index : DeactivateIndices)
    {
        SetInstanceInvalid(Index);
        ReleaseProjectileSlot(Index);

        OnProjectileDestroyed.Broadcast(Projectiles[Index].ProjectileID);
    }

    StateBits.PendingDeactivate.Init(false, StateBits.PendingDeactivate.Num());
}

int32 UProjectileManager::SpawnProjectile(const FProjectileSpawnParams& Params)
{
    int32 Index = AllocateProjectileSlot();
    if (Index == INVALID_INSTANCE_INDEX)
    {
        PreAllocateInstances(500);
        Index = AllocateProjectileSlot();
        if (Index == INVALID_INSTANCE_INDEX) return -1;
    }

    FProjectileData& Data = Projectiles[Index];
    FProjectileSpawnParams ModifiedParams = Params;
    ModifiedParams.ProjectileID = NextProjectileID++;
    ModifiedParams.Config.GroundHeight = DefaultGroundHeight;

    FParabolicTrajectoryResult Trajectory = FProjectileTrajectoryUtils::CalculateParabolicTrajectory(
        ModifiedParams.Origin,
        ModifiedParams.TargetLocation,
        ModifiedParams.Config.Height,
        ModifiedParams.Config.Speed
    );

    if (!Trajectory.bSuccess)
    {
        TArray<FVector> FallbackPoints;
        TArray<float> FallbackTimes;
        FallbackPoints.Add(ModifiedParams.Origin);
        FallbackPoints.Add(ModifiedParams.TargetLocation);
        FallbackTimes.Add(0.0f);
        FallbackTimes.Add(1.0f);

        Data.InitializeWithTrajectory(ModifiedParams, FallbackPoints, FallbackTimes);
    }
    else
    {
        Data.InitializeWithTrajectory(ModifiedParams, Trajectory.PathPoints, Trajectory.PathTimes);
        Data.TotalFlightTime = Trajectory.TotalTime;
    }

    Data.InstanceIndex = Index;

    ProjectileIDToIndex.Add(Data.ProjectileID, Index);

    StateBits.IsActive[Index] = true;
    StateBits.IsFlying[Index] = true;
    StateBits.IsInCollisionPhase[Index] = false;
    StateBits.DamageApplied[Index] = false;
    StateBits.PendingDeactivate[Index] = false;
    StateBits.PendingDestroy[Index] = false;
    StateBits.NeedsTransformUpdate[Index] = true;

    FTransform Transform = CalculateTransform(Data);
    UpdateInstanceTransform(Index, Transform);

    OnProjectileSpawned.Broadcast(Data.ProjectileID);

    return Data.ProjectileID;
}

int32 UProjectileManager::SpawnProjectile(const FProjectileLaunchParams& Params)
{
    FProjectileSpawnParams SpawnParams;
    SpawnParams.Origin = Params.Origin;
    SpawnParams.TargetLocation = Params.TargetLocation;
    SpawnParams.Instigator = Params.Instigator;
    SpawnParams.Config.Speed = Params.Config.Speed;
    SpawnParams.Config.Height = Params.Config.Height;
    SpawnParams.Config.Damage = Params.Config.Damage;
    SpawnParams.Config.CollisionRadius = Params.Config.CollisionRadius;
    SpawnParams.Config.MaxLifetime = Params.Config.MaxLifetime;
    SpawnParams.Config.Faction = Params.Config.Faction;
    SpawnParams.Config.GroundHeight = Params.Config.GroundHeight;
    SpawnParams.Config.CollisionHeightThreshold = Params.Config.CollisionHeightThreshold;
    SpawnParams.Config.DestroyDelay = Params.Config.DestroyDelay;
    return SpawnProjectile(SpawnParams);
}

void UProjectileManager::DestroyProjectile(int32 ProjectileID)
{
    int32* IndexPtr = ProjectileIDToIndex.Find(ProjectileID);
    if (!IndexPtr) return;

    TransitionToDestroyedPhase(*IndexPtr);
}

int32 UProjectileManager::AllocateProjectileSlot()
{
    FScopeLock Lock(&SlotLock);

    if (FreeIndices.Num() > 0)
    {
        return FreeIndices.Pop();
    }

    return INVALID_INSTANCE_INDEX;
}

void UProjectileManager::ReleaseProjectileSlot(int32 Index)
{
    FScopeLock Lock(&SlotLock);

    if (Projectiles.IsValidIndex(Index))
    {
        ProjectileIDToIndex.Remove(Projectiles[Index].ProjectileID);
        Projectiles[Index].Deactivate();

        StateBits.IsActive[Index] = false;
        StateBits.IsFlying[Index] = false;
        StateBits.IsInCollisionPhase[Index] = false;
        StateBits.NeedsTransformUpdate[Index] = false;
        StateBits.DamageApplied[Index] = false;
        StateBits.PendingDeactivate[Index] = false;
        StateBits.PendingDestroy[Index] = false;

        if (!FreeIndices.Contains(Index))
        {
            FreeIndices.Add(Index);
        }
    }
}

void UProjectileManager::SetInstanceInvalid(int32 InstanceIndex)
{
    if (HISMComponent && Projectiles.IsValidIndex(InstanceIndex))
    {
        HISMComponent->UpdateInstanceTransform(InstanceIndex, InvalidTransform, true, false, true);
    }
}

FTransform UProjectileManager::CalculateTransform(const FProjectileData& Data) const
{
    FQuat Rotation = Data.Direction.ToOrientationQuat();
    return FTransform(Rotation, Data.Position, Data.Scale);
}

void UProjectileManager::UpdateInstanceTransform(int32 InstanceIndex, const FTransform& Transform)
{
    if (HISMComponent)
    {
        HISMComponent->UpdateInstanceTransform(InstanceIndex, Transform, true, false, true);
    }
}

void UProjectileManager::SetDebugEnabled(bool bEnabled)
{
    bDebugEnabled = bEnabled;
}

void UProjectileManager::SetGroundHeight(float Height)
{
    DefaultGroundHeight = Height;
}

void UProjectileManager::ClearAllProjectiles()
{
    StateBits.ClearAll();

    FreeIndices.Empty();
    for (int32 i = 0; i < Projectiles.Num(); i++)
    {
        Projectiles[i].Deactivate();
        SetInstanceInvalid(i);
        FreeIndices.Add(i);
    }

    ProjectileIDToIndex.Empty();
    IndexCache.Clear();
    PendingHitResults.Empty();
}

void UProjectileManager::DrawActiveTrajectories()
{
    if (!bDrawTrajectoryDebug) return;

    UWorld* World = GetWorld();
    if (!World) return;

    for (int32 Index : IndexCache.ActiveIndices)
    {
        const FProjectileData& Data = Projectiles[Index];

        for (int32 i = 0; i < Data.TrajectoryPoints.Num(); i++)
        {
            bool bPassed = i <= Data.CurrentPointIndex;
            FColor PointColor = bPassed ? FColor::Red : FColor::Green;
            DrawDebugPoint(World, Data.TrajectoryPoints[i], 3.0f, PointColor, false, 0.0f);
        }

        DrawDebugPoint(World, Data.Position, 8.0f, FColor::Yellow, false, 0.0f);

        if (StateBits.IsFlying[Index])
        {
            FVector MoveDelta = Data.Position - Data.PrevPosition;
            float MoveDistance = MoveDelta.Size();

            if (MoveDistance > UE_KINDA_SMALL_NUMBER)
            {
                FVector CapsuleCenter = (Data.PrevPosition + Data.Position) * 0.5f;
                float HalfHeight = MoveDistance * 0.5f + Data.CollisionRadius;
                FQuat CapsuleRotation = MoveDelta.GetSafeNormal().ToOrientationQuat();

                DrawDebugCapsule(World,
                    CapsuleCenter,
                    HalfHeight,
                    Data.CollisionRadius,
                    CapsuleRotation,
                    FColor::Blue,
                    false, 0.0f, 0, 1.0f);
            }
        }
    }
}
