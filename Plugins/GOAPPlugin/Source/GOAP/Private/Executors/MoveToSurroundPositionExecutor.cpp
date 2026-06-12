#include "GOAPGameActionExecutors.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "GOAPManager.h"
#include "CrowdSurroundService.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FMoveToSurroundPositionExecutor::CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::HasTarget) == 1
        && CurrentWorldState.GetState(EGOAPGameStateKey::HasSurroundAssignment) == 1;
}

bool FMoveToSurroundPositionExecutor::ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::TargetOutOfRange) == 1 ||
           CurrentWorldState.GetState(EGOAPGameStateKey::EnemyDead) == 1 ||
           CurrentWorldState.GetState(EGOAPGameStateKey::HasSurroundAssignment) == 0;
}

bool FMoveToSurroundPositionExecutor::CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::AtSurroundPosition) == 1;
}

void FMoveToSurroundPositionExecutor::ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
    SCOPED_NAMED_EVENT(FMoveToSurroundPositionExecutor_ExecuteInternal, FColor::Orange);

    if (!Agent->QueryHasTarget())
    {
        Context.bActionFailed = true;
        return;
    }

    AActor* OwnerActor = Agent->QueryOwnerActor();
    if (!OwnerActor)
    {
        Context.bActionFailed = true;
        return;
    }

    UWorld* World = OwnerActor->GetWorld();
    if (!World)
    {
        Context.bActionFailed = true;
        return;
    }

    UGOAPManager* Manager = World->GetSubsystem<UGOAPManager>();
    if (!Manager)
    {
        Context.bActionFailed = true;
        return;
    }

    ICrowdSurroundService* SurroundService = Manager->GetCrowdSurroundService();
    if (!SurroundService)
    {
        Context.bActionFailed = true;
        return;
    }

    FCrowdSurroundAssignment Assignment;
    if (!SurroundService->GetSurroundAssignment(Agent->GetObjectID(), Assignment) || !Assignment.bHasAssignment)
    {
        // Assignment 已失效，触发 replan
        Context.bActionFailed = true;
        return;
    }

    const FVector CurrentPosition = Agent->QueryWorldPosition();
    Context.DesiredPosition = Assignment.DesiredPosition;
    Context.bHasDesiredPosition = true;
    Context.MoveSpeed = Agent->QueryMoveSpeed();
    Context.LookDirection = (Assignment.DesiredPosition - CurrentPosition).GetSafeNormal2D();
    Context.TargetObject = Agent->QueryTargetObject();
    Context.TargetLocation = Agent->QueryTargetPosition();
}

void FMoveToSurroundPositionExecutor::Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
}
