#pragma once

#include "CoreMinimal.h"
#include "QuestObjective.h"
#include "QuestObjective_ActorDeath.generated.h"

UCLASS()
class QUESTPLUGIN_API UQuestObjective_ActorDeath : public UQuestObjective
{
    GENERATED_BODY()

public:
    void Setup(TSubclassOf<AActor> InTargetClass, int32 InTargetCount,
               EQuestObjectiveResult InResultOnTrigger);

    virtual void NotifyActorDeath(AActor* DeadActor) override;
    virtual float GetProgress() const override;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    int32 GetKillCount() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    int32 GetTargetCount() const;

private:
    UPROPERTY()
    TSubclassOf<AActor> TargetActorClass;

    int32 TargetCount = 1;

    int32 CurrentKillCount = 0;
};
