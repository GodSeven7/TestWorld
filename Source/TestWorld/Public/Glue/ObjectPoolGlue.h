#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "ObjectPoolGlue.generated.h"

class UObjectPoolManager;
class USparseGridManager;

UCLASS()
class TESTWORLD_API UObjectPoolGlue : public UGlueBase
{
	GENERATED_BODY()

public:
	virtual void Initialize(UWorld* World) override;
	virtual void Shutdown() override;

	void Bind(
		UObjectPoolManager* InPoolManager,
		USparseGridManager* InGridManager);

	UObjectPoolManager* GetPoolManager() const { return PoolManager; }

private:
	UPROPERTY()
	TObjectPtr<UObjectPoolManager> PoolManager;

	UPROPERTY()
	TObjectPtr<USparseGridManager> GridManager;

	FDelegateHandle ActorReturnedToPoolHandle;

	UFUNCTION()
	void OnActorReturnedToPool(AActor* Actor);
};
