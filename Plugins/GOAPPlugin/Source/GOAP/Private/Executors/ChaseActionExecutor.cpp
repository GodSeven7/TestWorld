#include "GOAPGameActionExecutors.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "GOAPManager.h"
#include "CrowdSurroundService.h"
#include "GameFramework/Actor.h"
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
    return CurrentWorldState.GetState(EGOAPGameStateKey::TargetInAttackRange) == 1 ||
           CurrentWorldState.GetState(EGOAPGameStateKey::HasSurroundAssignment) == 1;
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

void FChaseActionExecutor::OnEnter(IGOAPAgentInterface* Agent)
{
    AActor* OwnerActor = Agent->QueryOwnerActor();
    if (!OwnerActor)
    {
        return;
    }

    UWorld* World = OwnerActor->GetWorld();
    if (!World)
    {
        return;
    }

    UGOAPManager* Manager = World->GetSubsystem<UGOAPManager>();
    if (!Manager)
    {
        return;
    }

    ICrowdSurroundService* SurroundService = Manager->GetCrowdSurroundService();
    if (!SurroundService)
    {
        return;
    }

    // 检查是否已有 assignment，避免重复 request
    FCrowdSurroundAssignment ExistingAssignment;
    if (SurroundService->GetSurroundAssignment(Agent->GetObjectID(), ExistingAssignment) && ExistingAssignment.bHasAssignment)
    {
        return;
    }

    UObject* TargetObj = Agent->QueryTargetObject();
    if (!TargetObj)
    {
        return;
    }

    FCrowdSurroundRequest Request;
    Request.ObjectID = Agent->GetObjectID();
    Request.AgentObject = OwnerActor;
    Request.TargetObject = TargetObj;
    Request.Params.AttackRange = Agent->QueryAttackRange();
    Request.Params.CollisionRadius = 40.0f;

    SurroundService->RequestSurroundAssignment(Request);
}

void FChaseActionExecutor::Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
}
