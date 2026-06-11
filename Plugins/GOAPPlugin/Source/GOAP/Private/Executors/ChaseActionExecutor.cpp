#include "GOAPGameActionExecutors.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FChaseActionExecutor::CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::HasTarget) == 1;
}

bool FChaseActionExecutor::ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::TargetOutOfRange) == 1 ||
           CurrentWorldState.GetState(EGOAPGameStateKey::EnemyDead) == 1;
}

bool FChaseActionExecutor::CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::TargetInAttackRange) == 1;
}

void FChaseActionExecutor::ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
    SCOPED_NAMED_EVENT(FChaseActionExecutor_ExecuteInternal, FColor::Orange);
    if (!Agent->QueryHasTarget())
    {
        Context.bActionFailed = true;
        return;
    }

    const FVector CurrentPosition = Agent->QueryWorldPosition();
    const FVector TargetPosition = Agent->QueryTargetPosition();

    Context.LookDirection = (TargetPosition - CurrentPosition).GetSafeNormal2D();
    Context.TargetLocation = TargetPosition;
    Context.TargetObject = Agent->QueryTargetObject();
    Context.DesiredPosition = TargetPosition;
    Context.MoveSpeed = Agent->QueryMoveSpeed();
    Context.bHasDesiredPosition = true;
}

void FChaseActionExecutor::Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
}
