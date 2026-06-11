#pragma once

#include "CoreMinimal.h"
#include "QuestTypes.h"
#include "QuestObjective.h"
#include "QuestInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnQuestFinished, EQuestObjectiveResult, Result, const FString&, Reason);

UCLASS()
class QUESTPLUGIN_API UQuestInstance : public UObject
{
    GENERATED_BODY()

public:
    void Setup(FName InQuestID);
    void AddObjective(UQuestObjective* Objective);

    void Start();
    void Tick(float DeltaTime);
    void NotifyActorDeath(AActor* DeadActor);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    EQuestState GetState() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    FName GetQuestID() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    EQuestObjectiveResult GetResult() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    const TArray<UQuestObjective*>& GetObjectives() const;

    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FOnQuestFinished OnQuestFinished;

private:
    void EvaluateObjectives();

    UPROPERTY()
    TArray<UQuestObjective*> Objectives;

    EQuestState State = EQuestState::Inactive;
    FName QuestID;
    EQuestObjectiveResult Result = EQuestObjectiveResult::Victory;
};
