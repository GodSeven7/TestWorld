// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ObjectPool.generated.h"

/**
 * Interface for objects that can be pooled.
 * Implement this on UObject or Actor classes to enable pooling.
 */
class OBJECTPOOLPLUGIN_API IPoolableObject
{
public:
	virtual ~IPoolableObject() = default;
	
	/** Called when object is spawned from pool */
	virtual void OnSpawnFromPool() {}
	
	/** Called when object is returned to pool */
	virtual void OnReturnToPool() {}
	
	/** Reset state before returning to pool */
	virtual void ResetPoolState() {}
	
	/** Check if object can be returned to pool */
	virtual bool CanReturnToPool() const { return true; }
};

/**
 * Configuration for object pool
 */
USTRUCT(BlueprintType)
struct FObjectPoolConfig
{
	GENERATED_BODY()
	
	/** Initial capacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InitialCapacity = 32;
	
	/** Max capacity (0 = unlimited) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCapacity = 0;
	
	/** Auto expand when pool is empty */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoExpand = true;
	
	/** Expand step size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ExpandStep = 16;
	
	/** Thread safe operations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bThreadSafe = true;
};

/**
 * Statistics for object pool
 */
USTRUCT(BlueprintType)
struct FObjectPoolStats
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
};

/**
 * Template object pool for UObject types
 */
template<typename T>
class TObjectPool
{
	static_assert(TIsDerivedFrom<T, UObject>::IsDerived, "T must derive from UObject");
	
public:
	TObjectPool(UObject* InOuter, const FObjectPoolConfig& InConfig);
	~TObjectPool();
	
	/** Spawn an object from pool */
	T* Spawn();
	
	/** Return object to pool */
	void Return(T* Object);
	
	/** Pre-warm pool with objects */
	void Prewarm(int32 Count);
	
	/** Clear all pooled objects */
	void Clear();
	
	/** Get pool statistics */
	FObjectPoolStats GetStats() const;
	
	/** Get pool capacity */
	int32 GetCapacity() const { return Config.MaxCapacity; }
	
	/** Get available count */
	int32 GetAvailableCount() const;
	
private:
	UObject* Outer;
	FObjectPoolConfig Config;
	mutable FObjectPoolStats Stats;
	
	/** Available objects */
	TArray<T*> FreeList;
	
	/** All created objects (for cleanup) */
	TArray<T*> AllObjects;
	
	/** Thread safety */
	mutable FCriticalSection PoolLock;
	
	/** Create new object */
	T* CreateNewObject();
	
	/** Expand pool */
	void ExpandPool(int32 Count);
};

// ============================================================================
// Implementation
// ============================================================================

template<typename T>
TObjectPool<T>::TObjectPool(UObject* InOuter, const FObjectPoolConfig& InConfig)
	: Outer(InOuter)
	, Config(InConfig)
{
	if (Config.InitialCapacity > 0)
	{
		Prewarm(Config.InitialCapacity);
	}
}

template<typename T>
TObjectPool<T>::~TObjectPool()
{
	Clear();
}

template<typename T>
T* TObjectPool<T>::Spawn()
{
	if (Config.bThreadSafe)
	{
		FScopeLock Lock(&PoolLock);
		
		T* Object = nullptr;
		
		if (FreeList.Num() > 0)
		{
			Object = FreeList.Pop();
		}
		else if (Config.bAutoExpand)
		{
			if (Config.MaxCapacity == 0 || Stats.TotalCreated < Config.MaxCapacity)
			{
				ExpandPool(Config.ExpandStep);
				if (FreeList.Num() > 0)
				{
					Object = FreeList.Pop();
				}
			}
		}
		
		if (Object)
		{
			Stats.TotalSpawned++;
			Stats.CurrentActive++;
			Stats.CurrentAvailable = FreeList.Num();
			Stats.PeakUsage = FMath::Max(Stats.PeakUsage, Stats.CurrentActive);
			
			// Call interface
			if (IPoolableObject* Poolable = Cast<IPoolableObject>(Object))
			{
				Poolable->OnSpawnFromPool();
			}
		}
		
		return Object;
	}
	else
	{
		T* Object = nullptr;
		
		if (FreeList.Num() > 0)
		{
			Object = FreeList.Pop();
		}
		else if (Config.bAutoExpand)
		{
			if (Config.MaxCapacity == 0 || Stats.TotalCreated < Config.MaxCapacity)
			{
				ExpandPool(Config.ExpandStep);
				if (FreeList.Num() > 0)
				{
					Object = FreeList.Pop();
				}
			}
		}
		
		if (Object)
		{
			Stats.TotalSpawned++;
			Stats.CurrentActive++;
			Stats.CurrentAvailable = FreeList.Num();
			Stats.PeakUsage = FMath::Max(Stats.PeakUsage, Stats.CurrentActive);
			
			if (IPoolableObject* Poolable = Cast<IPoolableObject>(Object))
			{
				Poolable->OnSpawnFromPool();
			}
		}
		
		return Object;
	}
}

template<typename T>
void TObjectPool<T>::Return(T* Object)
{
	if (!Object)
		return;
	
	if (Config.bThreadSafe)
	{
		FScopeLock Lock(&PoolLock);
		
		// Call interface
		if (IPoolableObject* Poolable = Cast<IPoolableObject>(Object))
		{
			Poolable->ResetPoolState();
			Poolable->OnReturnToPool();
		}
		
		FreeList.Add(Object);
		Stats.TotalReturned++;
		Stats.CurrentActive--;
		Stats.CurrentAvailable = FreeList.Num();
	}
	else
	{
		if (IPoolableObject* Poolable = Cast<IPoolableObject>(Object))
		{
			Poolable->ResetPoolState();
			Poolable->OnReturnToPool();
		}
		
		FreeList.Add(Object);
		Stats.TotalReturned++;
		Stats.CurrentActive--;
		Stats.CurrentAvailable = FreeList.Num();
	}
}

template<typename T>
void TObjectPool<T>::Prewarm(int32 Count)
{
	if (Config.bThreadSafe)
	{
		FScopeLock Lock(&PoolLock);
		ExpandPool(Count);
	}
	else
	{
		ExpandPool(Count);
	}
}

template<typename T>
void TObjectPool<T>::Clear()
{
	if (Config.bThreadSafe)
	{
		FScopeLock Lock(&PoolLock);
		
		for (T* Object : AllObjects)
		{
			if (Object && !Object->IsPendingKillPending())
			{
				Object->ConditionalBeginDestroy();
			}
		}
		
		AllObjects.Empty();
		FreeList.Empty();
		Stats = FObjectPoolStats();
	}
	else
	{
		for (T* Object : AllObjects)
		{
			if (Object && !Object->IsPendingKillPending())
			{
				Object->ConditionalBeginDestroy();
			}
		}
		
		AllObjects.Empty();
		FreeList.Empty();
		Stats = FObjectPoolStats();
	}
}

template<typename T>
FObjectPoolStats TObjectPool<T>::GetStats() const
{
	if (Config.bThreadSafe)
	{
		FScopeLock Lock(&PoolLock);
		return Stats;
	}
	return Stats;
}

template<typename T>
int32 TObjectPool<T>::GetAvailableCount() const
{
	if (Config.bThreadSafe)
	{
		FScopeLock Lock(&PoolLock);
		return FreeList.Num();
	}
	return FreeList.Num();
}

template<typename T>
T* TObjectPool<T>::CreateNewObject()
{
	// Use ::NewObject to avoid conflict with local template parameter
	T* NewObj = ::NewObject<T>(Outer);
	if (NewObj)
	{
		AllObjects.Add(NewObj);
		Stats.TotalCreated++;
	}
	return NewObj;
}

template<typename T>
void TObjectPool<T>::ExpandPool(int32 Count)
{
	int32 ActualCount = Count;
	if (Config.MaxCapacity > 0)
	{
		int32 Remaining = Config.MaxCapacity - Stats.TotalCreated;
		ActualCount = FMath::Min(Count, Remaining);
	}
	
	for (int32 i = 0; i < ActualCount; i++)
	{
		T* NewObject = CreateNewObject();
		if (NewObject)
		{
			FreeList.Add(NewObject);
		}
	}
	
	Stats.CurrentAvailable = FreeList.Num();
}
