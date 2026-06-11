#pragma once

#include "CoreMinimal.h"
#include "GlueBase.h"
#include "EventBus.h"
#include "GameUIGlue.generated.h"

class UGameDataSubsystem;
class UQuestManager;
class UCombatManager;

UCLASS()
class TESTWORLD_API UGameUIGlue : public UGlueBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void Tick(float DeltaTime) override;

    void Bind(
        UGameDataSubsystem* InGameDataSubsystem,
        UQuestManager* InQuestManager,
        UCombatManager* InCombatManager,
        UEventBus* InEventBus);

    UGameDataSubsystem* GetGameDataSubsystem() const { return GameDataSubsystem; }

private:
    UPROPERTY()
    TObjectPtr<UGameDataSubsystem> GameDataSubsystem;

    UPROPERTY()
    TObjectPtr<UQuestManager> QuestManager;

    UPROPERTY()
    TObjectPtr<UCombatManager> CombatManager;

    UPROPERTY()
    TObjectPtr<UEventBus> EventBus;

    UFUNCTION()
    void OnCombatantDeath(const FCombatantDeathEvent& Event);
};
