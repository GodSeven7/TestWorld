// Copyright Epic Games, Inc. All Rights Reserved.

#include "ObjectPoolManager.h"
#include "Engine/World.h"

void UObjectPoolManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTemp, Log, TEXT("ObjectPoolManager initialized"));
}

void UObjectPoolManager::Deinitialize()
{
	ClearAllPools();
	
	Super::Deinitialize();
	
	UE_LOG(LogTemp, Log, TEXT("ObjectPoolManager deinitialized"));
}

void UObjectPoolManager::ReturnActor(AActor* Actor)
{
	if (!Actor)
		return;
	
	OnActorReturnedToPool.Broadcast(Actor);
	
	FScopeLock Lock(&PoolsLock);
	
	UClass* Class = Actor->GetClass();
	
	for (auto& Pair : ActorPools)
	{
		if (Class->IsChildOf(Pair.Key))
		{
			break;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ReturnActor: No pool found for %s, destroying"), *Class->GetName());
	Actor->Destroy();
}

void UObjectPoolManager::ClearAllPools()
{
	FScopeLock Lock(&PoolsLock);
	
	for (auto& Pair : ActorPools)
	{
		if (Pair.Value.IsValid())
		{
		}
	}
	ActorPools.Empty();
	ActorPoolConfigs.Empty();
	
	for (auto& Pair : ObjectPools)
	{
		if (Pair.Value.IsValid())
		{
		}
	}
	ObjectPools.Empty();
	ObjectPoolConfigs.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("All pools cleared"));
}

void UObjectPoolManager::GetAllPoolStats(TMap<FString, FActorPoolStats>& OutActorStats, TMap<FString, FObjectPoolStats>& OutObjectStats)
{
	FScopeLock Lock(&PoolsLock);
	
	UE_LOG(LogTemp, Warning, TEXT("GetAllPoolStats: Use template version for specific pool types"));
}

void UObjectPoolManager::DumpPoolStats()
{
	FScopeLock Lock(&PoolsLock);
	
	UE_LOG(LogTemp, Log, TEXT("=== ObjectPoolManager Stats ==="));
	UE_LOG(LogTemp, Log, TEXT("Actor Pools: %d"), ActorPools.Num());
	UE_LOG(LogTemp, Log, TEXT("Object Pools: %d"), ObjectPools.Num());
}
