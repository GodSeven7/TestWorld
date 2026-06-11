#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "QuestTypes.h"
#include "QuestInstance.h"
#include "QuestManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnQuestCompleted, FName, QuestID,
    EQuestObjectiveResult, Result, const FString&, Reason);

UCLASS()
class QUESTPLUGIN_API UQuestManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    UFUNCTION(BlueprintCallable, Category = "Quest")
    UQuestInstance* CreateQuest(FName QuestID);

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void StartQuest(UQuestInstance* Quest);

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void StopQuest(FName QuestID);

    UFUNCTION(BlueprintCallable, Category = "Quest|Notification")
    void NotifyActorDeath(AActor* DeadActor);

    void TickQuests(float DeltaTime);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    UQuestInstance* GetQuest(FName QuestID) const;

    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FOnQuestCompleted OnQuestCompleted;

private:
    UPROPERTY()
    TArray<UQuestInstance*> ActiveQuests;

    TArray<FName> PendingStopQuestIDs;
    bool bIsTicking = false;
};
