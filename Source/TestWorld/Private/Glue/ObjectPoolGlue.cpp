#include "Glue/ObjectPoolGlue.h"
#include "ObjectPoolManager.h"
#include "SparseGridManager.h"

void UObjectPoolGlue::Initialize(UWorld* World)
{
}

void UObjectPoolGlue::Shutdown()
{
	if (PoolManager && ActorReturnedToPoolHandle.IsValid())
	{
		PoolManager->OnActorReturnedToPool.Remove(ActorReturnedToPoolHandle);
		ActorReturnedToPoolHandle.Reset();
	}
}

void UObjectPoolGlue::Bind(
	UObjectPoolManager* InPoolManager,
	USparseGridManager* InGridManager)
{
	PoolManager = InPoolManager;
	GridManager = InGridManager;

	// 订阅对象池回收事件
	if (PoolManager)
	{
		ActorReturnedToPoolHandle = PoolManager->OnActorReturnedToPool.AddUObject(this, &UObjectPoolGlue::OnActorReturnedToPool);
	}
}

void UObjectPoolGlue::OnActorReturnedToPool(AActor* Actor)
{
	if (GridManager && Actor)
	{
		GridManager->NotifyActorRemoved(Actor);
	}
}
