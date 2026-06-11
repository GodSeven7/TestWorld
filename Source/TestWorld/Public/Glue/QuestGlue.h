#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "EventBus.h"
#include "QuestGlue.generated.h"

class UQuestManager;

UCLASS()
class TESTWORLD_API UQuestGlue : public UGlueBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void Tick(float DeltaTime) override;

    void Bind(
        UQuestManager* InQuestManager,
        UEventBus* InEventBus);

    UQuestManager* GetQuestManager() const { return QuestManager; }

private:
    UPROPERTY()
    TObjectPtr<UQuestManager> QuestManager;

    UPROPERTY()
    TObjectPtr<UEventBus> EventBus;

    UFUNCTION()
    void OnCombatantDeath(const FCombatantDeathEvent& Event);
};
