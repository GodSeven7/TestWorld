#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SparseGridHandle.h"
#include "SparseGrid.h"
#include "SparseGridRaycast.h"
#include "SparseGridCollision.h"
#include "SpatialQueryService.h"
#include "GridEventProvider.h"
#include "SparseGridConfig.h"
#include "SparseGridManager.generated.h"

class ISparseGridObject;

// Per-object flow field computation result, produced by ComputeFlowFieldDirections
// and consumed by ApplySteeringCorrections. Named FSteeringFlowResult to avoid
// collision with FFlowFieldResult in PathfindingService.h.
struct FSteeringFlowResult
{
    FVector FlowDirection = FVector::ZeroVector;
    FVector BaseVelocity = FVector::ZeroVector;
    float EffectiveMoveSpeed = 0.0f;
    float DistanceToDesired = 0.0f;
    FVector DirectionToDesired = FVector::ZeroVector;
    bool bContactApproach = false;
    bool bHasDesiredPosition = false;
    bool bUsedDirectMove = false;
    // True when the object is locked, has zero speed, or has arrived —
    // ApplySteeringCorrections must skip these entirely.
    bool bSkipCorrections = false;
};

USTRUCT()
struct FObjectEntry
{
	GENERATED_BODY()

	UPROPERTY()
	UObject* Object = nullptr;

	ISparseGridObject* ObjectInterface = nullptr;

	FVector Position;
	uint32 Generation = 0;
	bool bActive = false;
	bool bEnableSteering = true;
	FVector SteeringForce;
	FVector ComputedVelocity;
	FVector LastComputedVelocity; // 上一帧速度，用于平滑
};

UCLASS()
class SPARSEGRIDPLUGIN_API USparseGridManager : public UWorldSubsystem, public ISpatialQueryService, public IGridEventProvider
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    
    FSparseGridHandle CreateObject(ISparseGridObject* Object);
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    void DestroyObject(const FSparseGridHandle& Handle);
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    void UpdateObjectPosition(const FSparseGridHandle& Handle, const FVector& NewPosition);
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    FVector GetObjectPosition(const FSparseGridHandle& Handle) const;
    
    ISparseGridObject* GetObjectInterface(const FSparseGridHandle& Handle) const;

    IGridObjectInfo* GetObjectByID(int32 ObjectID) const override;
    FSparseGridHandle GetHandleByObjectID(int32 ObjectID) const;
    
    TArray<ISparseGridObject*> GetAllObjects() const;
    
    void BatchUpdatePositions(
        const TArray<FSparseGridHandle>& Handles,
        const TArray<FVector>& NewPositions);
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    TArray<FSparseGridHandle> QuerySphere(const FVector& Center, float Radius) const;

	TArray<FSparseGridHandle> QuerySphereExcludingFactions(const FVector& Center,
		float Radius, int32 ExcludeFactionID) const;

	TArray<FSparseGridHandle> QueryBoxExcludingFactions(const FVector& Center,
		const FVector& Extent, int32 ExcludeFactionID) const;
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    TArray<FSparseGridRaycastHit> Raycast(
        const FVector& Origin,
        const FVector& Direction,
        const FSparseGridRaycastConfig& Config);
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    bool RaycastSingle(
        const FVector& Origin,
        const FVector& Direction,
        float MaxDistance,
        FSparseGridRaycastHit& OutHit);
    
    TArray<FSparseGridRaycastHit> RaycastThreadSafe(
        const FVector& Origin,
        const FVector& Direction,
        const FSparseGridRaycastConfig& Config) const;
    
    USparseGrid* GetSparseGrid() const { return SparseGrid; }
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    bool IsHandleValid(const FSparseGridHandle& Handle) const;
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Debug")
    void GetDebugInfo(FString& OutInfo) const;
    
    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Debug")
    void DrawDebugVisualization(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    void UpdateObjectFaction(const FSparseGridHandle& Handle, int32 OldFaction, int32 NewFaction);

    void NotifyActorRemoved(AActor* Actor);

    FOnActorRemovedFromGrid& OnActorRemovedFromGrid() override { return OnActorRemovedFromGridDelegate; }

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Collision")
    FCollisionTickResult ProcessCollisions();

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Collision")
    void SetCollisionEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Collision")
    bool IsCollisionEnabled() const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Collision")
    void SetCollisionMatrix(const FSparseGridCollisionMatrix& InMatrix);

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Collision")
    FSparseGridCollisionMatrix GetCollisionMatrix() const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Collision")
    FSparseGridCollisionStats GetCollisionStats() const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Collision")
    int32 GetActiveCollisionCount() const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Collision")
    TArray<FSparseGridCollisionPair> GetActiveCollisions() const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid")
    void SetSparseGridConfig(USparseGridConfig* InConfig);

    USparseGridConfig* GetSparseGridConfig() const { return SparseGridConfig; }

    // Phase 1: Compute flow field directions and arrival behavior for each object.
    void ComputeFlowFieldDirections();
    // Phase 2: Apply dynamic avoidance, separation/alignment, and velocity smoothing.
    void ApplySteeringCorrections();

    TArray<USparseGrid::FCoarseCellInfo> QuerySphereAtLevel(const FVector& Center, float Radius, int32 Level) const;
    TArray<USparseGrid::FCoarseCellInfo> QueryBoxAtLevel(const FBox& Box, int32 Level) const;
    TArray<FSparseGridHandle> GetHandlesInCoarseCell(const FIntVector& CoarseCoord, int32 Level) const;
    int32 GetCoarseCellObjectCount(const FIntVector& CoarseCoord, int32 Level) const;

    UFUNCTION(BlueprintCallable, Category = "SparseGrid|Steering")
    FVector GetComputedVelocity(const FSparseGridHandle& Handle) const;


private:
    UPROPERTY()
    USparseGrid* SparseGrid = nullptr;

    UPROPERTY()
    TArray<FObjectEntry> Objects;
    TArray<int32> FreeIndices;

    mutable FRWLock ObjectsLock;

    bool bDebugEnabled = false;

    FSparseGridCollisionDetector* CollisionDetector = nullptr;

    FOnActorRemovedFromGrid OnActorRemovedFromGridDelegate;

    UPROPERTY()
    USparseGridConfig* SparseGridConfig = nullptr;

    // Per-object flow field results, indexed by object index.
    // Populated by ComputeFlowFieldDirections, consumed by ApplySteeringCorrections.
    TArray<FSteeringFlowResult> SteeringFlowResults;

    // ObjectID → Handle mapping for O(1) lookup
    TMap<int32, FSparseGridHandle> ObjectIDToHandleMap;

    bool IsIndexValid(int32 Index) const;

    bool ValidateHandle(const FSparseGridHandle& Handle, const FString& Operation) const;
    
    TArray<FSparseGridRaycastHit> RaycastInternal(
        const FVector& Origin,
        const FVector& Direction,
        const FSparseGridRaycastConfig& Config) const;

    static FSpatialQueryHandle ToSpatialQueryHandle(const FSparseGridHandle& Handle);
    static FSparseGridHandle ToGridHandle(const FSpatialQueryHandle& Handle);

public:
    virtual TArray<AActor*> QueryActorsInSphere(
        const FVector& Center,
        float Radius,
        const FSpatialQueryFilter& Filter = FSpatialQueryFilter()) const override;

    virtual TArray<AActor*> QueryActorsInBox(
        const FVector& Center,
        const FVector& Extent,
        const FSpatialQueryFilter& Filter = FSpatialQueryFilter()) const override;

    virtual AActor* QueryNearestActor(
        const FVector& Origin,
        const FSpatialQueryFilter& Filter = FSpatialQueryFilter()) const override;

    virtual TArray<FSpatialQueryHandle> QueryHandlesInSphere(
        const FVector& Center,
        float Radius) const override;

    virtual TArray<FSpatialQueryHandle> QueryHandlesInSphereExcludingFactions(
        const FVector& Center,
        float Radius,
        int32 ExcludeFactionID) const override;

    virtual TArray<FSpatialQueryHandle> QueryHandlesInBoxExcludingFactions(
        const FVector& Center,
        const FVector& Extent,
        int32 ExcludeFactionID) const override;

    virtual IGridObjectInfo* GetObjectFromHandle(const FSpatialQueryHandle& Handle) const override;
};
