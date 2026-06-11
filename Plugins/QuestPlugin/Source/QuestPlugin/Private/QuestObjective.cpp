#include "QuestObjective.h"

void UQuestObjective::Setup(EQuestObjectiveResult InResultOnTrigger)
{
    ResultOnTrigger = InResultOnTrigger;
}

bool UQuestObjective::IsTriggered() const
{
    return bTriggered;
}

EQuestObjectiveResult UQuestObjective::GetResultOnTrigger() const
{
    return ResultOnTrigger;
}

float UQuestObjective::GetProgress() const
{
    return 0.f;
}
