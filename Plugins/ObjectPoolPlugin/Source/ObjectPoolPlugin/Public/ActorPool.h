// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActorPool.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorReturnedToPool, AActor*);

/**
 * Interface for poolable actors.
 * Implement this on Actor classes to enable pooling.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UPoolableActor : public UInterface
{
	GENERATED_BODY()
};

class OBJECTPOOLPLUGIN_API IPoolableActor
{
	GENERATED_BODY()
	
public:
	/** Called when actor is taken from pool */
	virtual void OnActorPoolSpawn() {}
	
	/** Called when actor is returned to pool */
	virtual void OnActorPoolReturn() {}
	
	/** Reset actor state for reuse */
	virtual void ResetActorPoolState() {}
	
	/** Check if actor can be returned to pool */
	virtual bool CanReturnToActorPool() const { return true; }
	
	/** Set actor active state */
	virtual void SetActorPoolActive(bool bActive) {}
	
	/** Check if actor is active in pool */
	virtual bool IsActorPoolActive() const { return true; }

	virtual void SetActorPoolPendingReturn(bool bReturn) {}

	virtual bool IsActorPoolPendingReturn() const { return false; }

	/** Get delegate for return to pool event - override to provide delegate */
	virtual FOnActorReturnedToPool& GetOnReturnedToPoolDelegate() 
	{ 
		static FOnActorReturnedToPool DefaultDelegate;
		return DefaultDelegate;
	}

	/**
	 * Recycle self back to pool.
	 * Call this to return the actor to its pool.
	 * @return true if successfully returned to pool, false otherwise
	 */
	bool RecycleSelf();
};

/**
 * Configuration for actor pool
 */
USTRUCT(BlueprintType)
struct FActorPoolConfig
{
	GENERATED_BODY()
	
	/** Initial capacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InitialCapacity = 16;
	
	/** Max capacity (0 = unlimited) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCapacity = 0;
	
	/** Auto expand when pool is empty */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoExpand = true;
	
	/** Expand step size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ExpandStep = 8;
	
	/** Hide actors in pool */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHideInPool = true;
	
	/** Disable tick in pool */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDisableTickInPool = true;
	
	/** Move to hidden location when in pool */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PoolHiddenLocation = FVector(0, 0, -100000);
};

/**
 * Statistics for actor pool
 */
USTRUCT(BlueprintType)
struct FActorPoolStats
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	int32 TotalCreated = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 TotalSpawned = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 TotalReturned = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentActive = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentAvailable = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 PeakUsage = 0;
	
	UPROPERTY(BlueprintReadOnly)
	float AverageLifetime = 0.0f;
};

/**
 * Template actor pool for AActor types
 * Actors are not destroyed but disabled and reused
 */
template<typename T>
class TActorPool
{
	static_assert(TIsDerivedFrom<T, AActor>::IsDerived, "T must derive from AActor");
	
public:
	/** Delegate for custom spawn initialization */
	DECLARE_DELEGATE_TwoParams(FOnSpawnActor, T*, const FTransform&);
	
	/** Delegate for custom return cleanup */
	DECLARE_DELEGATE_OneParam(FOnReturnActor, T*);

	TActorPool(UWorld* InWorld, const FActorPoolConfig& InConfig);
	TActorPool(UWorld* InWorld, UClass* InActorClass, const FActorPoolConfig& InConfig);
	~TActorPool();
	
	/**
	 * Spawn an actor from pool at given transform
	 * @param Transform The world transform for the spawned actor
	 * @return The spawned actor, or nullptr if pool is empty and cannot expand
	 */
	T* Spawn(const FTransform& Transform);
	
	/**
	 * Spawn an actor from pool at given location
	 * @param Location The world location for the spawned actor
	 * @param Rotation The world rotation for the spawned actor (default: zero)
	 * @return The spawned actor, or nullptr if pool is empty and cannot expand
	 */
	T* Spawn(const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	
	/**
	 * Return actor to pool
	 * @param Actor The actor to return
	 */
	void Return(T* Actor);
	
	/** Pre-warm pool with actors */
	void Prewarm(int32 Count);
	
	/** Clear and destroy all pooled actors */
	void Clear();
	
	/** Get pool statistics */
	FActorPoolStats GetStats() const;
	
	/** Get available count */
	int32 GetAvailableCount() const;
	
	/** Get active count */
	int32 GetActiveCount() const;
	
	/** Set custom spawn delegate */
	void SetOnSpawnDelegate(FOnSpawnActor InDelegate) { OnSpawnDelegate = InDelegate; }
	
	/** Set custom return delegate */
	void SetOnReturnDelegate(FOnReturnActor InDelegate) { OnReturnDelegate = InDelegate; }
	
private:
	UWorld* World;
	UClass* ActorClass;
	FActorPoolConfig Config;
	mutable FActorPoolStats Stats;
	
	/** Available actors (disabled, ready to use) */
	TArray<T*> FreeList;
	
	/** All created actors (for cleanup) */
	TArray<T*> AllActors;
	
	/** Active actors */
	TSet<TWeakObjectPtr<T>> ActiveActors;
	
	/** Actors that have been fully initialized (BeginPlay called) */
	TSet<T*> InitializedActors;
	
	/** Thread safety */
	mutable FCriticalSection PoolLock;
	
	/** Custom delegates */
	FOnSpawnActor OnSpawnDelegate;
	FOnReturnActor OnReturnDelegate;
	
	/** Create new actor */
	T* CreateNewActor();
	
	/** Expand pool */
	void ExpandPool(int32 Count);
	
	/** Internal spawn without lock */
	T* SpawnInternal(const FTransform& Transform);
	
	/** Internal return without lock */
	void ReturnInternal(T* Actor);
	
	/** Activate actor */
	void ActivateActor(T* Actor, const FTransform& Transform);
	
	/** Deactivate actor */
	void DeactivateActor(T* Actor);
};

// ============================================================================
// Implementation
// ============================================================================
template<typename T>
TActorPool<T>::TActorPool(UWorld* InWorld, const FActorPoolConfig& InConfig)
	: World(InWorld)
	, ActorClass(T::StaticClass())
	, Config(InConfig)
{
	if (Config.InitialCapacity > 0 && World)
	{
		Prewarm(Config.InitialCapacity);
	}
}

template<typename T>
TActorPool<T>::TActorPool(UWorld* InWorld, UClass* InActorClass, const FActorPoolConfig& InConfig)
	: World(InWorld)
	, ActorClass(InActorClass)
	, Config(InConfig)
{
	if (Config.InitialCapacity > 0 && World)
	{
		Prewarm(Config.InitialCapacity);
	}
}

template<typename T>
TActorPool<T>::~TActorPool()
{
	Clear();
}

template<typename T>
T* TActorPool<T>::Spawn(const FTransform& Transform)
{
	if (!World)
		return nullptr;
	
	if (Config.bAutoExpand)
	{
		FScopeLock Lock(&PoolLock);
		return SpawnInternal(Transform);
	}
	else
	{
		FScopeLock Lock(&PoolLock);
		return SpawnInternal(Transform);
	}
}

template<typename T>
T* TActorPool<T>::Spawn(const FVector& Location, const FRotator& Rotation)
{
	return Spawn(FTransform(Rotation, Location));
}

template<typename T>
void TActorPool<T>::Return(T* Actor)
{
	if (!Actor)
		return;
	
	FScopeLock Lock(&PoolLock);
	ReturnInternal(Actor);
}

template<typename T>
void TActorPool<T>::Prewarm(int32 Count)
{
	if (!World)
		return;
	
	FScopeLock Lock(&PoolLock);
	ExpandPool(Count);
}

template<typename T>
void TActorPool<T>::Clear()
{
	FScopeLock Lock(&PoolLock);
	
	for (T* Actor : AllActors)
	{
		if (Actor && !Actor->IsPendingKillPending())
		{
			Actor->Destroy();
		}
	}
	
	AllActors.Empty();
	FreeList.Empty();
	ActiveActors.Empty();
	InitializedActors.Empty();
	Stats = FActorPoolStats();
}

template<typename T>
FActorPoolStats TActorPool<T>::GetStats() const
{
	FScopeLock Lock(&PoolLock);
	return Stats;
}

template<typename T>
int32 TActorPool<T>::GetAvailableCount() const
{
	FScopeLock Lock(&PoolLock);
	return FreeList.Num();
}

template<typename T>
int32 TActorPool<T>::GetActiveCount() const
{
	FScopeLock Lock(&PoolLock);
	return Stats.CurrentActive;
}

template<typename T>
T* TActorPool<T>::CreateNewActor()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	FTransform SpawnTransform(FRotator::ZeroRotator, Config.PoolHiddenLocation);
	
	T* TypedActor = World->SpawnActorDeferred<T>(ActorClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);	
	if (TypedActor)
	{
		AllActors.Add(TypedActor);
		Stats.TotalCreated++;
		
		DeactivateActor(TypedActor);
	}
	return TypedActor;
}

template<typename T>
void TActorPool<T>::ExpandPool(int32 Count)
{
	int32 ActualCount = Count;
	if (Config.MaxCapacity > 0)
	{
		int32 Remaining = Config.MaxCapacity - Stats.TotalCreated;
		ActualCount = FMath::Min(Count, Remaining);
	}
	
	for (int32 i = 0; i < ActualCount; i++)
	{
		T* NewActor = CreateNewActor();
		if (NewActor)
		{
			FreeList.Add(NewActor);
		}
	}
	
	Stats.CurrentAvailable = FreeList.Num();
}

template<typename T>
T* TActorPool<T>::SpawnInternal(const FTransform& Transform)
{
	T* Actor = nullptr;
	
	// Try to get from free list
	if (FreeList.Num() > 0)
	{
		Actor = FreeList.Pop();
	}
	// Try to expand
	else if (Config.bAutoExpand)
	{
		if (Config.MaxCapacity == 0 || Stats.TotalCreated < Config.MaxCapacity)
		{
			ExpandPool(Config.ExpandStep);
			if (FreeList.Num() > 0)
			{
				Actor = FreeList.Pop();
			}
		}
	}
	
	if (Actor)
	{
		ActivateActor(Actor, Transform);
		
		Stats.TotalSpawned++;
		Stats.CurrentActive++;
		Stats.CurrentAvailable = FreeList.Num();
		Stats.PeakUsage = FMath::Max(Stats.PeakUsage, Stats.CurrentActive);
		
		ActiveActors.Add(Actor);
	}
	
	return Actor;
}

template<typename T>
void TActorPool<T>::ReturnInternal(T* Actor)
{
	if (!Actor || !AllActors.Contains(Actor))
		return;
	
	// Check if already in pool
	if (FreeList.Contains(Actor))
		return;
	
	DeactivateActor(Actor);
	
	ActiveActors.Remove(Actor);
	FreeList.Add(Actor);
	
	Stats.TotalReturned++;
	Stats.CurrentActive--;
	Stats.CurrentAvailable = FreeList.Num();
}

template<typename T>
void TActorPool<T>::ActivateActor(T* Actor, const FTransform& Transform)
{
	if (!InitializedActors.Contains(Actor))
	{
		Actor->FinishSpawning(Transform);
		InitializedActors.Add(Actor);
	}
	else
	{
		Actor->SetActorTransform(Transform);
	}
	
	if (Config.bHideInPool)
	{
		Actor->SetActorHiddenInGame(false);
	}
	
	if (Config.bDisableTickInPool)
	{
		Actor->SetActorTickEnabled(true);
	}
	
	if (IPoolableActor* Poolable = Cast<IPoolableActor>(Actor))
	{
		Poolable->SetActorPoolActive(true);
		Poolable->OnActorPoolSpawn();
	}
	
	if (OnSpawnDelegate.IsBound())
	{
		OnSpawnDelegate.Execute(Actor, Transform);
	}
}

template<typename T>
void TActorPool<T>::DeactivateActor(T* Actor)
{
	if (IPoolableActor* Poolable = Cast<IPoolableActor>(Actor))
	{
		Poolable->SetActorPoolPendingReturn(true);
	}

	// Call custom delegate first
	if (OnReturnDelegate.IsBound())
	{
		OnReturnDelegate.Execute(Actor);
	}
	
	// Call interface
	if (IPoolableActor* Poolable = Cast<IPoolableActor>(Actor))
	{
		Poolable->ResetActorPoolState();
		Poolable->SetActorPoolActive(false);
		Poolable->SetActorPoolPendingReturn(false);
		Poolable->OnActorPoolReturn();
	}
	
	// Hide actor
	if (Config.bHideInPool)
	{
		Actor->SetActorHiddenInGame(true);
	}
	
	// Disable tick
	if (Config.bDisableTickInPool)
	{
		Actor->SetActorTickEnabled(false);
	}
	
	// Move to hidden location
	Actor->SetActorLocation(Config.PoolHiddenLocation);
}
