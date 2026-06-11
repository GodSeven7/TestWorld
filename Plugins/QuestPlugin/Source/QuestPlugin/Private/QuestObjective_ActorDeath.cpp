#include "QuestObjective_ActorDeath.h"

void UQuestObjective_ActorDeath::Setup(TSubclassOf<AActor> InTargetClass, int32 InTargetCount,
                                        EQuestObjectiveResult InResultOnTrigger)
{
    TargetActorClass = InTargetClass;
    TargetCount = FMath::Max(1, InTargetCount);
    ResultOnTrigger = InResultOnTrigger;
}

void UQuestObjective_ActorDeath::NotifyActorDeath(AActor* DeadActor)
{
    if (bTriggered || !DeadActor || !TargetActorClass)
    {
        return;
    }

    if (DeadActor->IsA(TargetActorClass))
    {
        CurrentKillCount++;
        if (CurrentKillCount >= TargetCount)
        {
            bTriggered = true;
        }
    }
}

float UQuestObjective_ActorDeath::GetProgress() const
{
    return static_cast<float>(CurrentKillCount) / static_cast<float>(TargetCount);
}

int32 UQuestObjective_ActorDeath::GetKillCount() const
{
    return CurrentKillCount;
}

int32 UQuestObjective_ActorDeath::GetTargetCount() const
{
    return TargetCount;
}
