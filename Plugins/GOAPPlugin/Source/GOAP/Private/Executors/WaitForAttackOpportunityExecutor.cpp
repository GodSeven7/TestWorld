#include "GOAPGameActionExecutors.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "GOAPManager.h"
#include "CrowdSurroundService.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FWaitForAttackOpportunityExecutor::CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::HasTarget) == 1
        && CurrentWorldState.GetState(EGOAPGameStateKey::HasSurroundAssignment) == 1
        && CurrentWorldState.GetState(EGOAPGameStateKey::AtSurroundPosition) == 1;
}

bool FWaitForAttackOpportunityExecutor::ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::TargetOutOfRange) == 1 ||
           CurrentWorldState.GetState(EGOAPGameStateKey::EnemyDead) == 1 ||
           CurrentWorldState.GetState(EGOAPGameStateKey::HasSurroundAssignment) == 0;
}

bool FWaitForAttackOpportunityExecutor::CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    // 由真实 WorldState 中的 TargetInAttackRange 驱动完成
    return CurrentWorldState.GetState(EGOAPGameStateKey::TargetInAttackRange) == 1;
}

void FWaitForAttackOpportunityExecutor::ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
    SCOPED_NAMED_EVENT(FWaitForAttackOpportunityExecutor_ExecuteInternal, FColor::Yellow);

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
    const UGOAPAgentConfig* Config = Agent->GetAgentConfig();
    float ArrivalDist = Config ? Config->ArrivalDistance : 50.0f;

    // 持续读取最新 Assignment.DesiredPosition
    Context.DesiredPosition = Assignment.DesiredPosition;
    Context.bHasDesiredPosition = true;
    Context.TargetObject = Agent->QueryTargetObject();
    Context.TargetLocation = Agent->QueryTargetPosition();

    float DistToDesired = FVector::Dist2D(CurrentPosition, Assignment.DesiredPosition);
    if (DistToDesired <= ArrivalDist)
    {
        // 已在 assignment 位置，停住
        Context.MoveSpeed = 0.0f;
        Context.LookDirection = (Agent->QueryTargetPosition() - CurrentPosition).GetSafeNormal2D();
    }
    else
    {
        // CrowdSurround 更新了位置，继续移动
        Context.MoveSpeed = Agent->QueryMoveSpeed();
        Context.LookDirection = (Assignment.DesiredPosition - CurrentPosition).GetSafeNormal2D();
    }
}

void FWaitForAttackOpportunityExecutor::Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
}
