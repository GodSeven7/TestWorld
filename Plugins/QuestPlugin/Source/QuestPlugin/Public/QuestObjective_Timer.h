#pragma once

#include "CoreMinimal.h"
#include "QuestObjective.h"
#include "QuestObjective_Timer.generated.h"

UCLASS()
class QUESTPLUGIN_API UQuestObjective_Timer : public UQuestObjective
{
    GENERATED_BODY()

public:
    void Setup(float InTimeLimit, EQuestObjectiveResult InResultOnTrigger);

    virtual void Tick(float DeltaTime) override;
    virtual float GetProgress() const override;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    float GetRemainingTime() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    float GetTimeLimit() const;

private:
    float TimeLimit = 60.f;

    float RemainingTime = 0.f;
};
