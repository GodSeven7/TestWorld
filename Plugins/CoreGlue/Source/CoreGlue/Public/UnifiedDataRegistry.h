#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UnifiedDataRegistry.generated.h"

class UActorUnifiedDataComponent;

UINTERFACE(MinimalAPI)
class UUnifiedDataRegistry : public UInterface
{
	GENERATED_BODY()
};

/**
 * 统一数据组件注册接口
 * UPluginCoordinator 实现此接口
 */
class COREGLUE_API IUnifiedDataRegistry
{
	GENERATED_BODY()

public:
	virtual void RegisterUnifiedDataComponent(UActorUnifiedDataComponent* Component) = 0;
	virtual void UnregisterUnifiedDataComponent(UActorUnifiedDataComponent* Component) = 0;
	virtual TArray<TObjectPtr<UActorUnifiedDataComponent>>& GetUnifiedDataComponents() = 0;
	virtual void NotifyFactionChanged(UActorUnifiedDataComponent* Component, int32 OldFaction, int32 NewFaction) {}

	// Resolve ObjectID to UnifiedDataComponent (O(1) lookup)
	virtual UActorUnifiedDataComponent* GetObjectByID(int32 ObjectID) const = 0;
};
