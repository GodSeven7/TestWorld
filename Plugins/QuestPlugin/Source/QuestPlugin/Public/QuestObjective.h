#pragma once

#include "CoreMinimal.h"
#include "QuestTypes.h"
#include "QuestObjective.generated.h"

UCLASS(Abstract, DefaultToInstanced)
class QUESTPLUGIN_API UQuestObjective : public UObject
{
    GENERATED_BODY()

public:
    void Setup(EQuestObjectiveResult InResultOnTrigger);

    virtual void Tick(float DeltaTime) {}
    virtual void NotifyActorDeath(AActor* DeadActor) {}

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    bool IsTriggered() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    EQuestObjectiveResult GetResultOnTrigger() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    virtual float GetProgress() const;

protected:
    EQuestObjectiveResult ResultOnTrigger = EQuestObjectiveResult::Victory;

    bool bTriggered = false;
};
