#include "GOAPGameActionExecutors.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FReturnActionExecutor::CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::TargetOutOfRange) == 1 ||
           CurrentWorldState.GetState(EGOAPGameStateKey::HasTarget) == 0;
}

bool FReturnActionExecutor::ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::HasTarget) == 1;
}

bool FReturnActionExecutor::CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::AtHomeLocation) == 1;
}

void FReturnActionExecutor::ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
    SCOPED_NAMED_EVENT(FReturnActionExecutor_ExecuteInternal, FColor::Blue);

    const UGOAPAgentConfig* Config = Agent->GetAgentConfig();
    if (!Config) { Context.bActionFailed = true; return; }

    FVector SpawnedPosition = Agent->QuerySpawnedPosition();
    if (SpawnedPosition.IsNearlyZero())
    {
        Context.bActionFailed = true;
        return;
    }

    int32 ObjID = Agent->GetObjectID();
    FAgentState* State = nullptr;
    {
        FScopeLock Lock(StateLock.Get());
        State = AgentStates.Find(ObjID);
        if (!State)
        {
            State = &AgentStates.Add(ObjID, FAgentState());
        }
    }

    if (!State->bTargetSet)
    {
        State->TargetLocation = SpawnedPosition;
        State->bTargetSet = true;
    }

    FVector CurrentPos = Agent->QueryWorldPosition();
    float Distance = FVector::Dist(CurrentPos, State->TargetLocation);

    if (Distance <= Config->ArrivalDistance)
    {
        Context.bActionCompleted = true;
        State->bTargetSet = false;
    }
    else
    {
        FVector Direction = (State->TargetLocation - CurrentPos).GetSafeNormal2D();
        Context.LookDirection = Direction;
        Context.MoveSpeed = Agent->QueryMoveSpeed();
        Context.TargetLocation = State->TargetLocation;
        Context.DesiredPosition = State->TargetLocation;
        Context.bHasDesiredPosition = true;
        Context.bActionCompleted = false;
    }
}

void FReturnActionExecutor::Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
}

void FReturnActionExecutor::OnExit(IGOAPAgentInterface* Agent, EGOAPActionExitReason Reason)
{
    FScopeLock Lock(StateLock.Get());
    AgentStates.Remove(Agent->GetObjectID());
}
