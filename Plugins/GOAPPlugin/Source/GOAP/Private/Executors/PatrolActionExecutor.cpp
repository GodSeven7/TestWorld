#include "GOAPGameActionExecutors.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FPatrolActionExecutor::CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::HasPatrolPath) == 1;
}

bool FPatrolActionExecutor::ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::HasTarget) == 1;
}

bool FPatrolActionExecutor::CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return false;
}

void FPatrolActionExecutor::ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
    SCOPED_NAMED_EVENT(FPatrolActionExecutor_ExecuteInternal, FColor::Cyan);

    const UGOAPAgentConfig* Config = Agent->GetAgentConfig();
    if (!Config) { Context.bActionFailed = true; return; }

    const TArray<FVector>& PatrolPoints = Agent->QueryPatrolPoints();
    if (PatrolPoints.Num() == 0)
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

    if (State->bIsWaiting)
    {
        State->WaitTimer -= DeltaTime;
        if (State->WaitTimer <= 0.0f)
        {
            State->bIsWaiting = false;
            State->CurrentIndex = (State->CurrentIndex + 1) % PatrolPoints.Num();
        }
        Context.bActionCompleted = false;
        return;
    }

    FVector CurrentPos = Agent->QueryWorldPosition();
    FVector TargetPoint = PatrolPoints[State->CurrentIndex];
    float Distance = FVector::Dist2D(CurrentPos, TargetPoint);

    if (Distance <= Config->ArrivalDistance)
    {
        State->bIsWaiting = true;
        State->WaitTimer = Config->PatrolWaitTime;
        Context.bActionCompleted = false;
    }
    else
    {
        FVector Direction = (TargetPoint - CurrentPos).GetSafeNormal2D();
        Context.LookDirection = Direction;
        Context.MoveSpeed = Agent->QueryMoveSpeed();
        Context.TargetLocation = TargetPoint;
        Context.DesiredPosition = TargetPoint;
        Context.bHasDesiredPosition = true;
        Context.bActionCompleted = false;
    }
}

void FPatrolActionExecutor::Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
}

void FPatrolActionExecutor::OnExit(IGOAPAgentInterface* Agent, EGOAPActionExitReason Reason)
{
    FScopeLock Lock(StateLock.Get());
    AgentStates.Remove(Agent->GetObjectID());
}
