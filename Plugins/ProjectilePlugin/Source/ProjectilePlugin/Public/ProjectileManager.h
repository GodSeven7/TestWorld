#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectileTypes.h"
#include "ProjectileData.h"
#include "SpatialQueryService.h"
#include "DamageService.h"
#include "ProjectileService.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GridObjectInfo.h"
#include "ProjectileManager.generated.h"


UCLASS()
class PROJECTILEPLUGIN_API UProjectileManager : public UWorldSubsystem, public IProjectileService
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    void Tick(float DeltaTime);

    void SetSpatialQueryService(ISpatialQueryService* InService) { SpatialQuery = InService; }
    ISpatialQueryService* GetSpatialQueryService() const { return SpatialQuery; }

    void SetDamageService(IDamageService* InService) { DamageService = InService; }
    IDamageService* GetDamageService() const { return DamageService; }

    void SetDamageCallback(TFunction<void(AActor*, float, int32)> InCallback) { DamageCallback = MoveTemp(InCallback); }

    int32 SpawnProjectile(const FProjectileSpawnParams& Params);
    void DestroyProjectile(int32 ProjectileID);

    virtual int32 SpawnProjectile(const FProjectileLaunchParams& Params) override;

    UFUNCTION(BlueprintCallable, Category = "Projectile")
    int32 GetActiveProjectileCount() const { return IndexCache.ActiveIndices.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Projectile")
    int32 GetTotalProjectileCount() const { return Projectiles.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void SetDebugEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void ClearAllProjectiles();

    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void SetGroundHeight(float Height);

    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void SetDrawTrajectoryDebug(bool bEnabled) { bDrawTrajectoryDebug = bEnabled; }

    UFUNCTION(BlueprintCallable, Category = "Projectile")
    bool IsDrawTrajectoryDebugEnabled() const { return bDrawTrajectoryDebug; }

    UPROPERTY(BlueprintAssignable, Category = "Projectile")
    FOnProjectileHitDelegate OnProjectileHit;

    UPROPERTY(BlueprintAssignable, Category = "Projectile")
    FOnProjectileSpawnedDelegate OnProjectileSpawned;

    UPROPERTY(BlueprintAssignable, Category = "Projectile")
    FOnProjectileDestroyedDelegate OnProjectileDestroyed;

protected:
    UPROPERTY()
    TSubclassOf<AActor> ProjectileActorClass;

    UPROPERTY()
    TObjectPtr<AActor> ProjectileActor;

    UPROPERTY()
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISMComponent;

    ISpatialQueryService* SpatialQuery = nullptr;
    IDamageService* DamageService = nullptr;

    TFunction<void(AActor*, float, int32)> DamageCallback;

    TArray<FProjectileData> Projectiles;
    TMap<int32, int32> ProjectileIDToIndex;
    TArray<int32> FreeIndices;

    FProjectileStateBits StateBits;
    FProjectileIndexCache IndexCache;

    TArray<FProjectileHitResult> PendingHitResults;

    int32 NextProjectileID = 1;

    static constexpr int32 INVALID_INSTANCE_INDEX = -1;
    static constexpr int32 INITIAL_CAPACITY = 500;
    static constexpr float INVALID_POSITION_OFFSET = -100000.0f;

    FVector InvalidPosition = FVector(0, 0, INVALID_POSITION_OFFSET);
    FTransform InvalidTransform;

    float DefaultGroundHeight = 0.0f;

    FCriticalSection SlotLock;

    bool bDebugEnabled = false;
    bool bDrawTrajectoryDebug = false;

    void LoadActorFromConfig();
    bool SpawnActorFromBlueprint();
    void EnsureDependencies();
    void PreAllocateInstances(int32 Count);

    void CollectActiveIndices();
    void CollectPhaseIndices();
    void CollectDeactivateIndices();

    void ParallelComputeMovement(float DeltaTime);
    void ParallelSweepCollisionDetection();
    void ParallelCollisionDetection();
    void ProcessHitResults();
    void FlushTransformUpdates();
    void ProcessPendingDeactivations();
    void DrawActiveTrajectories();

    void TransitionToCollisionPhase(int32 Index);
    void TransitionToDestroyedPhase(int32 Index);

    void ApplyDamageToTarget(int32 Index, AActor* Target);

    TArray<IGridObjectInfo*> QueryTargetsInSphere(const FVector& Center, float Radius, int32 ExcludeFaction);
    TArray<IGridObjectInfo*> QueryTargetsInCapsule(const FVector& Start, const FVector& End, float Radius, int32 ExcludeFaction);

    static constexpr int32 MAX_SWEEP_SUBSTEPS = 8;
    static constexpr float SWEEP_STEP_MULTIPLIER = 1.5f;

    int32 AllocateProjectileSlot();
    void ReleaseProjectileSlot(int32 Index);

    FTransform CalculateTransform(const FProjectileData& Data) const;
    void UpdateInstanceTransform(int32 InstanceIndex, const FTransform& Transform);
    void SetInstanceInvalid(int32 InstanceIndex);

private:
    void ProcessFlyingPhase(FProjectileData& Data, int32 Index, float DeltaTime);
    void ProcessCollisionPhase(FProjectileData& Data, int32 Index, float DeltaTime);
};
