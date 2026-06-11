#include "QuestManager.h"

void UQuestManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UQuestManager::Deinitialize()
{
    ActiveQuests.Empty();
    Super::Deinitialize();
}

bool UQuestManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

UQuestInstance* UQuestManager::CreateQuest(FName QuestID)
{
    UQuestInstance* Quest = NewObject<UQuestInstance>(this);
    Quest->Setup(QuestID);
    return Quest;
}

void UQuestManager::StartQuest(UQuestInstance* Quest)
{
    if (!Quest)
    {
        return;
    }

    if (Quest->GetState() != EQuestState::Inactive)
    {
        return;
    }

    Quest->Start();
    ActiveQuests.Add(Quest);
}

void UQuestManager::StopQuest(FName QuestID)
{
    if (bIsTicking)
    {
        PendingStopQuestIDs.Add(QuestID);
        return;
    }

    int32 Index = ActiveQuests.IndexOfByPredicate(
        [QuestID](const UQuestInstance* Q) { return Q && Q->GetQuestID() == QuestID; });

    if (Index != INDEX_NONE)
    {
        ActiveQuests.RemoveAtSwap(Index);
    }
}

void UQuestManager::NotifyActorDeath(AActor* DeadActor)
{
    if (!DeadActor)
    {
        return;
    }

    for (UQuestInstance* Quest : ActiveQuests)
    {
        if (Quest && Quest->GetState() == EQuestState::Active)
        {
            Quest->NotifyActorDeath(DeadActor);
        }
    }
}

void UQuestManager::TickQuests(float DeltaTime)
{
    bIsTicking = true;

    TArray<UQuestInstance*> CompletedQuests;

    for (UQuestInstance* Quest : ActiveQuests)
    {
        if (!Quest || Quest->GetState() != EQuestState::Active)
        {
            continue;
        }

        Quest->Tick(DeltaTime);
        EQuestState CurrState = Quest->GetState();

        if (CurrState != EQuestState::Active)
        {
            CompletedQuests.Add(Quest);

            FString Reason;
            if (CurrState == EQuestState::Completed)
            {
                Reason = TEXT("Quest Completed");
            }
            else if (CurrState == EQuestState::Failed)
            {
                Reason = TEXT("Quest Failed");
            }

            OnQuestCompleted.Broadcast(Quest->GetQuestID(), Quest->GetResult(), Reason);
            Quest->OnQuestFinished.Broadcast(Quest->GetResult(), Reason);
        }
    }

    for (UQuestInstance* Quest : CompletedQuests)
    {
        ActiveQuests.Remove(Quest);
    }

    for (FName QuestID : PendingStopQuestIDs)
    {
        int32 Index = ActiveQuests.IndexOfByPredicate(
            [QuestID](const UQuestInstance* Q) { return Q && Q->GetQuestID() == QuestID; });
        if (Index != INDEX_NONE)
        {
            ActiveQuests.RemoveAtSwap(Index);
        }
    }
    PendingStopQuestIDs.Empty();

    bIsTicking = false;
}

UQuestInstance* UQuestManager::GetQuest(FName QuestID) const
{
    for (UQuestInstance* Quest : ActiveQuests)
    {
        if (Quest && Quest->GetQuestID() == QuestID)
        {
            return Quest;
        }
    }
    return nullptr;
}
