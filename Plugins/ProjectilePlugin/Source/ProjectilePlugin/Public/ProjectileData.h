#pragma once

#include "CoreMinimal.h"
#include "ProjectileTypes.h"
#include "ProjectileData.generated.h"

USTRUCT()
struct PROJECTILEPLUGIN_API FProjectileData
{
    GENERATED_BODY()

    int32 InstanceIndex = -1;
    int32 ProjectileID = -1;

    TArray<FVector> TrajectoryPoints;
    TArray<float> TrajectoryTimes;
    int32 CurrentPointIndex = 0;
    float ElapsedTime = 0.0f;
    float TotalFlightTime = 0.0f;

    FVector Position = FVector::ZeroVector;
    FVector PrevPosition = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    FVector Direction = FVector::ForwardVector;

    float Speed = 2000.0f;
    float Height = 200.0f;
    float Damage = 10.0f;
    float CollisionRadius = 10.0f;
    float Lifetime = 5.0f;
    float MaxLifetime = 5.0f;

    int32 Faction = 0;

    EProjectilePhase Phase = EProjectilePhase::Destroyed;

    float GroundHeight = 0.0f;
    float CollisionHeightThreshold = 50.0f;

    float DestroyDelay = 3.0f;
    float DestroyTimer = 0.0f;
    bool bPendingDestroy = false;

    TWeakObjectPtr<AActor> Instigator;
    TWeakObjectPtr<AActor> LastHitActor;

    FVector Scale = FVector(1.0f);

    bool bDamageApplied = false;

    void InitializeWithTrajectory(const FProjectileSpawnParams& Params, 
        const TArray<FVector>& InTrajectoryPoints, 
        const TArray<float>& InTrajectoryTimes)
    {
        TrajectoryPoints = InTrajectoryPoints;
        TrajectoryTimes = InTrajectoryTimes;
        CurrentPointIndex = 0;
        ElapsedTime = 0.0f;

        if (TrajectoryPoints.Num() > 0)
        {
            Position = TrajectoryPoints[0];
            PrevPosition = Position;
        }

        Speed = Params.Config.Speed;
        Height = Params.Config.Height;
        Damage = Params.Config.Damage;
        CollisionRadius = Params.Config.CollisionRadius;
        MaxLifetime = Params.Config.MaxLifetime;
        Lifetime = MaxLifetime;
        Faction = Params.Config.Faction;
        GroundHeight = Params.Config.GroundHeight;
        CollisionHeightThreshold = Params.Config.CollisionHeightThreshold;
        DestroyDelay = Params.Config.DestroyDelay;
        DestroyTimer = 0.0f;
        bPendingDestroy = false;
        Instigator = Params.Instigator;
        ProjectileID = Params.ProjectileID;
        Phase = EProjectilePhase::Flying;
        bDamageApplied = false;
    }

    void Deactivate()
    {
        InstanceIndex = -1;
        ProjectileID = -1;

        TrajectoryPoints.Empty();
        TrajectoryTimes.Empty();
        CurrentPointIndex = 0;
        ElapsedTime = 0.0f;
        TotalFlightTime = 0.0f;

        Position = FVector::ZeroVector;
        PrevPosition = FVector::ZeroVector;
        Velocity = FVector::ZeroVector;
        Direction = FVector::ForwardVector;

        Speed = 2000.0f;
        Height = 200.0f;
        Damage = 10.0f;
        CollisionRadius = 10.0f;
        Lifetime = 5.0f;
        MaxLifetime = 5.0f;

        Faction = 0;

        Phase = EProjectilePhase::Destroyed;

        GroundHeight = 0.0f;
        CollisionHeightThreshold = 50.0f;

        DestroyDelay = 3.0f;
        DestroyTimer = 0.0f;
        bPendingDestroy = false;

        Instigator = nullptr;
        LastHitActor = nullptr;

        Scale = FVector(1.0f);
        bDamageApplied = false;
    }

    void StartDestroyTimer()
    {
        if (!bPendingDestroy)
        {
            bPendingDestroy = true;
            DestroyTimer = DestroyDelay;
        }
    }

    float GetHeightAboveGround() const
    {
        return Position.Z - GroundHeight;
    }

    bool ShouldEnterCollisionPhase() const
    {
        return GetHeightAboveGround() <= CollisionHeightThreshold;
    }
};

USTRUCT()
struct PROJECTILEPLUGIN_API FProjectileStateBits
{
    GENERATED_BODY()

    TBitArray<> IsActive;
    TBitArray<> IsFlying;
    TBitArray<> IsInCollisionPhase;
    TBitArray<> NeedsTransformUpdate;
    TBitArray<> DamageApplied;
    TBitArray<> PendingDeactivate;
    TBitArray<> PendingDestroy;

    void InitAll(int32 Count)
    {
        IsActive.Init(false, Count);
        IsFlying.Init(false, Count);
        IsInCollisionPhase.Init(false, Count);
        NeedsTransformUpdate.Init(false, Count);
        DamageApplied.Init(false, Count);
        PendingDeactivate.Init(false, Count);
        PendingDestroy.Init(false, Count);
    }

    void ResizeAll(int32 OldCount, int32 NewCount)
    {
        IsActive.Reserve(NewCount);
        IsFlying.Reserve(NewCount);
        IsInCollisionPhase.Reserve(NewCount);
        NeedsTransformUpdate.Reserve(NewCount);
        DamageApplied.Reserve(NewCount);
        PendingDeactivate.Reserve(NewCount);
        PendingDestroy.Reserve(NewCount);

        for (int32 i = OldCount; i < NewCount; i++)
        {
            IsActive.Add(false);
            IsFlying.Add(false);
            IsInCollisionPhase.Add(false);
            NeedsTransformUpdate.Add(false);
            DamageApplied.Add(false);
            PendingDeactivate.Add(false);
            PendingDestroy.Add(false);
        }
    }

    void ClearAll()
    {
        IsActive.Init(false, IsActive.Num());
        IsFlying.Init(false, IsFlying.Num());
        IsInCollisionPhase.Init(false, IsInCollisionPhase.Num());
        NeedsTransformUpdate.Init(false, NeedsTransformUpdate.Num());
        DamageApplied.Init(false, DamageApplied.Num());
        PendingDeactivate.Init(false, PendingDeactivate.Num());
        PendingDestroy.Init(false, PendingDestroy.Num());
    }
};

USTRUCT()
struct PROJECTILEPLUGIN_API FProjectileIndexCache
{
    GENERATED_BODY()

    TArray<int32> ActiveIndices;
    TArray<int32> FlyingIndices;
    TArray<int32> CollisionPhaseIndices;
    TArray<int32> DeactivateIndices;

    void Clear()
    {
        ActiveIndices.Empty();
        FlyingIndices.Empty();
        CollisionPhaseIndices.Empty();
        DeactivateIndices.Empty();
    }

    void Reserve(int32 Count)
    {
        ActiveIndices.Reserve(Count);
        FlyingIndices.Reserve(Count);
        CollisionPhaseIndices.Reserve(Count);
        DeactivateIndices.Reserve(Count);
    }
};

USTRUCT()
struct PROJECTILEPLUGIN_API FParabolicTrajectoryResult
{
    GENERATED_BODY()

    TArray<FVector> PathPoints;
    TArray<float> PathTimes;
    float TotalTime = 0.0f;
    bool bSuccess = false;
};

class PROJECTILEPLUGIN_API FProjectileTrajectoryUtils
{
public:
    static FParabolicTrajectoryResult CalculateParabolicTrajectory(
        const FVector& StartPos,
        const FVector& TargetPos,
        float Height,
        float Speed,
        float SimFrequency = 30.0f
    );
};
