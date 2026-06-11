#include "QuestObjective_Timer.h"

void UQuestObjective_Timer::Setup(float InTimeLimit, EQuestObjectiveResult InResultOnTrigger)
{
    TimeLimit = FMath::Max(0.f, InTimeLimit);
    RemainingTime = TimeLimit;
    ResultOnTrigger = InResultOnTrigger;
}

void UQuestObjective_Timer::Tick(float DeltaTime)
{
    if (bTriggered)
    {
        return;
    }

    RemainingTime -= DeltaTime;
    if (RemainingTime <= 0.f)
    {
        RemainingTime = 0.f;
        bTriggered = true;
    }
}

float UQuestObjective_Timer::GetProgress() const
{
    if (TimeLimit <= 0.f)
    {
        return 1.f;
    }
    return 1.f - (RemainingTime / TimeLimit);
}

float UQuestObjective_Timer::GetRemainingTime() const
{
    return RemainingTime;
}

float UQuestObjective_Timer::GetTimeLimit() const
{
    return TimeLimit;
}
