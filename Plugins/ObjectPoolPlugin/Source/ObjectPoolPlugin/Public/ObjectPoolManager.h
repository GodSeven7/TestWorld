// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ObjectPool.h"
#include "ActorPool.h"
#include "ObjectPoolManager.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorReturnedToPoolGlobal, AActor*);

UCLASS()
class OBJECTPOOLPLUGIN_API UObjectPoolManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	template<typename T>
	TSharedPtr<TActorPool<T>> GetOrCreateActorPool(const FActorPoolConfig& Config = FActorPoolConfig());
	
	template<typename T>
	TSharedPtr<TActorPool<T>> GetOrCreateActorPool(UClass* ActorClass, const FActorPoolConfig& Config = FActorPoolConfig());
	
	template<typename T>
	T* SpawnActor(UWorld* World, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	
	template<typename T>
	T* SpawnActor(UWorld* World, const FTransform& Transform);
	
	template<typename T>
	void ReturnActor(T* Actor);
	
	void ReturnActor(AActor* Actor);
	
	template<typename T>
	TSharedPtr<TObjectPool<T>> GetOrCreateObjectPool(const FObjectPoolConfig& Config = FObjectPoolConfig());
	
	template<typename T>
	T* SpawnObject();
	
	template<typename T>
	void ReturnObject(T* Object);
	
	template<typename T>
	void PrewarmActorPool(int32 Count);
	
	template<typename T>
	void PrewarmObjectPool(int32 Count);
	
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void ClearAllPools();
	
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void GetAllPoolStats(TMap<FString, FActorPoolStats>& OutActorStats, TMap<FString, FObjectPoolStats>& OutObjectStats);
	
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void DumpPoolStats();

	FOnActorReturnedToPoolGlobal OnActorReturnedToPool;

private:
	TMap<UClass*, TSharedPtr<void>> ActorPools;
	TMap<UClass*, FActorPoolConfig> ActorPoolConfigs;
	
	TMap<UClass*, TSharedPtr<void>> ObjectPools;
	TMap<UClass*, FObjectPoolConfig> ObjectPoolConfigs;
	
	mutable FCriticalSection PoolsLock;
	
	template<typename T>
	static FString GetTypeName() { return T::StaticClass()->GetName(); }
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename T>
TSharedPtr<TActorPool<T>> UObjectPoolManager::GetOrCreateActorPool(const FActorPoolConfig& Config)
{
	UClass* Class = T::StaticClass();
	return GetOrCreateActorPool<T>(Class, Config);
}

template<typename T>
TSharedPtr<TActorPool<T>> UObjectPoolManager::GetOrCreateActorPool(UClass* ActorClass, const FActorPoolConfig& Config)
{
	FScopeLock Lock(&PoolsLock);
	
	if (!ActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetOrCreateActorPool: ActorClass is null"));
		return nullptr;
	}
	
	// Check if pool exists
	auto* ExistingPool = ActorPools.Find(ActorClass);
	if (ExistingPool && ExistingPool->IsValid())
	{
		return StaticCastSharedPtr<TActorPool<T>>(*ExistingPool);
	}
	
	// Create new pool
	TSharedPtr<TActorPool<T>> NewPool = MakeShareable(new TActorPool<T>(GetWorld(), ActorClass, Config));
	ActorPools.Add(ActorClass, NewPool);
	ActorPoolConfigs.Add(ActorClass, Config);
	
	UE_LOG(LogTemp, Log, TEXT("Created actor pool for %s with capacity %d"), *ActorClass->GetName(), Config.InitialCapacity);
	
	return NewPool;
}

template<typename T>
T* UObjectPoolManager::SpawnActor(UWorld* World, const FVector& Location, const FRotator& Rotation)
{
	return SpawnActor<T>(World, FTransform(Rotation, Location));
}

template<typename T>
T* UObjectPoolManager::SpawnActor(UWorld* World, const FTransform& Transform)
{
	if (!World)
		return nullptr;
	
	// Get or create pool
	FActorPoolConfig Config;
	if (FActorPoolConfig* ExistingConfig = ActorPoolConfigs.Find(T::StaticClass()))
	{
		Config = *ExistingConfig;
	}
	
	TSharedPtr<TActorPool<T>> Pool = GetOrCreateActorPool<T>(Config);
	if (!Pool.IsValid())
		return nullptr;
	
	return Pool->Spawn(Transform);
}

template<typename T>
void UObjectPoolManager::ReturnActor(T* Actor)
{
	if (!Actor)
		return;
	
	FScopeLock Lock(&PoolsLock);
	
	UClass* Class = T::StaticClass();
	auto* PoolPtr = ActorPools.Find(Class);
	if (PoolPtr && PoolPtr->IsValid())
	{
		TSharedPtr<TActorPool<T>> Pool = StaticCastSharedPtr<TActorPool<T>>(*PoolPtr);
		Pool->Return(Actor);
	}
}

template<typename T>
TSharedPtr<TObjectPool<T>> UObjectPoolManager::GetOrCreateObjectPool(const FObjectPoolConfig& Config)
{
	FScopeLock Lock(&PoolsLock);
	
	UClass* Class = T::StaticClass();
	
	// Check if pool exists
	auto* ExistingPool = ObjectPools.Find(Class);
	if (ExistingPool && ExistingPool->IsValid())
	{
		return StaticCastSharedPtr<TObjectPool<T>>(*ExistingPool);
	}
	
	// Create new pool
	TSharedPtr<TObjectPool<T>> NewPool = MakeShareable(new TObjectPool<T>(this, Config));
	ObjectPools.Add(Class, NewPool);
	ObjectPoolConfigs.Add(Class, Config);
	
	UE_LOG(LogTemp, Log, TEXT("Created object pool for %s with capacity %d"), *GetTypeName<T>(), Config.InitialCapacity);
	
	return NewPool;
}

template<typename T>
T* UObjectPoolManager::SpawnObject()
{
	// Get or create pool
	FObjectPoolConfig Config;
	if (FObjectPoolConfig* ExistingConfig = ObjectPoolConfigs.Find(T::StaticClass()))
	{
		Config = *ExistingConfig;
	}
	
	TSharedPtr<TObjectPool<T>> Pool = GetOrCreateObjectPool<T>(Config);
	if (!Pool.IsValid())
		return nullptr;
	
	return Pool->Spawn();
}

template<typename T>
void UObjectPoolManager::ReturnObject(T* Object)
{
	if (!Object)
		return;
	
	FScopeLock Lock(&PoolsLock);
	
	UClass* Class = T::StaticClass();
	auto* PoolPtr = ObjectPools.Find(Class);
	if (PoolPtr && PoolPtr->IsValid())
	{
		TSharedPtr<TObjectPool<T>> Pool = StaticCastSharedPtr<TObjectPool<T>>(*PoolPtr);
		Pool->Return(Object);
	}
}

template<typename T>
void UObjectPoolManager::PrewarmActorPool(int32 Count)
{
	FActorPoolConfig Config;
	if (FActorPoolConfig* ExistingConfig = ActorPoolConfigs.Find(T::StaticClass()))
	{
		Config = *ExistingConfig;
	}
	Config.InitialCapacity = Count;
	
	TSharedPtr<TActorPool<T>> Pool = GetOrCreateActorPool<T>(Config);
	if (Pool.IsValid())
	{
		Pool->Prewarm(Count);
	}
}

template<typename T>
void UObjectPoolManager::PrewarmObjectPool(int32 Count)
{
	FObjectPoolConfig Config;
	if (FObjectPoolConfig* ExistingConfig = ObjectPoolConfigs.Find(T::StaticClass()))
	{
		Config = *ExistingConfig;
	}
	Config.InitialCapacity = Count;
	
	TSharedPtr<TObjectPool<T>> Pool = GetOrCreateObjectPool<T>(Config);
	if (Pool.IsValid())
	{
		Pool->Prewarm(Count);
	}
}
