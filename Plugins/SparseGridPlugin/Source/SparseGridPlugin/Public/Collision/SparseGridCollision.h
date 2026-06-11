// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SparseGridCollision.generated.h"

class USparseGridManager;
class USparseGrid;
class AActor;
class ISparseGridObject;

USTRUCT(BlueprintType)
struct SPARSEGRIDPLUGIN_API FSparseGridCollisionMatrix
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    TMap<int32, int32> CollisionMasks;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    int32 DefaultCollisionMask = 0xFFFFFFFF;

    bool ShouldCollide(int32 LayerA, int32 LayerB) const
    {
        int32 MaskA = DefaultCollisionMask;
        if (const int32* FoundMask = CollisionMasks.Find(LayerA))
        {
            MaskA = *FoundMask;
        }
        return (MaskA & (1 << LayerB)) != 0;
    }

    void SetCollision(int32 LayerA, int32 LayerB, bool bShouldCollide)
    {
        int32& MaskA = CollisionMasks.FindOrAdd(LayerA);
        if (bShouldCollide)
            MaskA |= (1 << LayerB);
        else
            MaskA &= ~(1 << LayerB);

        int32& MaskB = CollisionMasks.FindOrAdd(LayerB);
        if (bShouldCollide)
            MaskB |= (1 << LayerA);
        else
            MaskB &= ~(1 << LayerA);
    }
};

USTRUCT(BlueprintType)
struct SPARSEGRIDPLUGIN_API FSparseGridCollisionPair
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> ActorA;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> ActorB;

    FSparseGridCollisionPair() = default;

    FSparseGridCollisionPair(AActor* InActorA, AActor* InActorB)
    {
        if (InActorA < InActorB)
        {
            ActorA = InActorA;
            ActorB = InActorB;
        }
        else
        {
            ActorA = InActorB;
            ActorB = InActorA;
        }
    }

    bool IsValid() const { return ActorA.IsValid() && ActorB.IsValid(); }

    bool operator==(const FSparseGridCollisionPair& Other) const
    {
        return ActorA == Other.ActorA && ActorB == Other.ActorB;
    }

    bool operator!=(const FSparseGridCollisionPair& Other) const
    {
        return !(*this == Other);
    }

    friend uint32 GetTypeHash(const FSparseGridCollisionPair& Pair)
    {
        return HashCombine(GetTypeHash(Pair.ActorA), GetTypeHash(Pair.ActorB));
    }
};

USTRUCT(BlueprintType)
struct SPARSEGRIDPLUGIN_API FSparseGridCollisionStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TotalChecks = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ActiveCollisions = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 BeginEvents = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 EndEvents = 0;

    UPROPERTY(BlueprintReadOnly)
    float DetectionTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float EventDispatchTimeMs = 0.0f;

    void Reset()
    {
        TotalChecks = 0;
        ActiveCollisions = 0;
        BeginEvents = 0;
        EndEvents = 0;
        DetectionTimeMs = 0.0f;
        EventDispatchTimeMs = 0.0f;
    }
};

USTRUCT(BlueprintType)
struct SPARSEGRIDPLUGIN_API FSparseGridCollisionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AActor* ActorA = nullptr;

    UPROPERTY(BlueprintReadOnly)
    AActor* ActorB = nullptr;

    ISparseGridObject* ObjectA = nullptr;
    ISparseGridObject* ObjectB = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FVector ContactPoint = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float Distance = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float CombinedRadius = 0.0f;

    bool bDispatchEvents = true;

    bool IsValid() const { return ObjectA != nullptr && ObjectB != nullptr; }
};

USTRUCT(BlueprintType)
struct SPARSEGRIDPLUGIN_API FCollisionTickResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FSparseGridCollisionResult> NewCollisions;

    UPROPERTY(BlueprintReadOnly)
    TArray<FSparseGridCollisionPair> EndedCollisions;

    TArray<ISparseGridObject*> ObjectsToDestroy;
};

class SPARSEGRIDPLUGIN_API FSparseGridCollisionDetector
{
public:
    FSparseGridCollisionDetector();

    void SetCollisionMatrix(const FSparseGridCollisionMatrix& InMatrix);
    const FSparseGridCollisionMatrix& GetCollisionMatrix() const { return CollisionMatrix; }

    void SetEnabled(bool bEnabled) { bCollisionEnabled = bEnabled; }
    bool IsEnabled() const { return bCollisionEnabled; }

    FCollisionTickResult Tick(USparseGridManager* Manager);

    void DetectCollisionsFromObjects(
        const TArray<ISparseGridObject*>& Objects,
        USparseGrid* Grid,
        USparseGridManager* Manager,
        TArray<FSparseGridCollisionResult>& OutNewCollisions,
        TArray<FSparseGridCollisionPair>& OutEndedCollisions,
        TArray<FSparseGridCollisionResult>& OutActiveCollisions);

    void ProcessCollisionEvents(TArray<FSparseGridCollisionResult>& Collisions);

    const FSparseGridCollisionStats& GetStats() const { return Stats; }
    void ResetStats() { Stats.Reset(); }

    const TSet<FSparseGridCollisionPair>& GetActiveCollisions() const { return ActiveCollisions; }
    void ClearActiveCollisions() { ActiveCollisions.Empty(); }
    void RemoveCollisionsForObject(ISparseGridObject* Object);

private:
    FSparseGridCollisionMatrix CollisionMatrix;
    TSet<FSparseGridCollisionPair> ActiveCollisions;
    FSparseGridCollisionStats Stats;
    bool bCollisionEnabled = true;
};
