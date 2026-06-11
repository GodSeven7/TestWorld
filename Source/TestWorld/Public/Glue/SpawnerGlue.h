#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "EventBus.h"
#include "SpawnerGlue.generated.h"

class USpawnerManager;
class USparseGridManager;

UCLASS()
class TESTWORLD_API USpawnerGlue : public UGlueBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void Tick(float DeltaTime) override;

    void Bind(
        USpawnerManager* InSpawnerManager,
        USparseGridManager* InGridManager,
        UEventBus* InEventBus);

    USpawnerManager* GetSpawnerManager() const { return SpawnerManager; }

private:
    UPROPERTY()
    TObjectPtr<USpawnerManager> SpawnerManager;

    UPROPERTY()
    TObjectPtr<USparseGridManager> GridManager;

    UPROPERTY()
    TObjectPtr<UEventBus> EventBus;
};
