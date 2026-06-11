#include "QuestInstance.h"

void UQuestInstance::Setup(FName InQuestID)
{
    QuestID = InQuestID;
    State = EQuestState::Inactive;
}

void UQuestInstance::AddObjective(UQuestObjective* Objective)
{
    if (Objective)
    {
        Objectives.Add(Objective);
    }
}

void UQuestInstance::Start()
{
    if (State == EQuestState::Inactive)
    {
        State = EQuestState::Active;
    }
}

void UQuestInstance::Tick(float DeltaTime)
{
    if (State != EQuestState::Active)
    {
        return;
    }

    for (UQuestObjective* Objective : Objectives)
    {
        if (Objective)
        {
            Objective->Tick(DeltaTime);
        }
    }

    EvaluateObjectives();
}

void UQuestInstance::NotifyActorDeath(AActor* DeadActor)
{
    if (State != EQuestState::Active)
    {
        return;
    }

    for (UQuestObjective* Objective : Objectives)
    {
        if (Objective)
        {
            Objective->NotifyActorDeath(DeadActor);
        }
    }

    EvaluateObjectives();
}

EQuestState UQuestInstance::GetState() const
{
    return State;
}

FName UQuestInstance::GetQuestID() const
{
    return QuestID;
}

EQuestObjectiveResult UQuestInstance::GetResult() const
{
    return Result;
}

const TArray<UQuestObjective*>& UQuestInstance::GetObjectives() const
{
    return Objectives;
}

void UQuestInstance::EvaluateObjectives()
{
    for (UQuestObjective* Objective : Objectives)
    {
        if (Objective && Objective->IsTriggered())
        {
            Result = Objective->GetResultOnTrigger();
            State = (Result == EQuestObjectiveResult::Victory) ? EQuestState::Completed : EQuestState::Failed;
            return;
        }
    }
}
