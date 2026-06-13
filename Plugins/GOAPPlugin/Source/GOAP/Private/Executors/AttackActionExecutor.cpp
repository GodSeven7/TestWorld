#include "GOAPGameActionExecutors.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "GOAPManager.h"
#include "CrowdSurroundService.h"
#include "GameFramework/Actor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

bool FAttackActionExecutor::CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::HasTarget) == 1
        && CurrentWorldState.GetState(EGOAPGameStateKey::TargetInAttackRange) == 1;
}

bool FAttackActionExecutor::ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return CurrentWorldState.GetState(EGOAPGameStateKey::TargetOutOfRange) == 1 ||
           CurrentWorldState.GetState(EGOAPGameStateKey::EnemyDead) == 1;
}

bool FAttackActionExecutor::CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const
{
    return false;
}

void FAttackActionExecutor::ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
    SCOPED_NAMED_EVENT(FAttackActionExecutor_ExecuteInternal, FColor::Red);
    if (!Agent->QueryHasTarget())
    {
        Context.bActionFailed = true;
        return;
    }

    const UGOAPAgentConfig* Config = Agent->GetAgentConfig();
    if (!Config)
    {
        Context.bActionFailed = true;
        return;
    }

    Context.bIsAttacking = true;

    const FVector TargetPosition = Agent->QueryTargetPosition();
    const FVector AgentPosition = Agent->QueryWorldPosition();
    Context.LookDirection = (TargetPosition - AgentPosition).GetSafeNormal2D();
    Context.TargetLocation = TargetPosition;
    Context.TargetObject = Agent->QueryTargetObject();
    Context.DesiredPosition = TargetPosition;
    Context.bHasDesiredPosition = true;
	Context.MoveSpeed = 0;//Config->MoveSpeed* Config->ApproachMoveSpeedRatio;
}

void FAttackActionExecutor::OnEnter(IGOAPAgentInterface* Agent)
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
    if (SurroundService)
    {
        SurroundService->LockAttackSlot(Agent->GetObjectID());
    }
}

void FAttackActionExecutor::OnExit(IGOAPAgentInterface* Agent, EGOAPActionExitReason Reason)
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
    if (SurroundService)
    {
        SurroundService->UnlockAttackSlot(Agent->GetObjectID());
    }
}

void FAttackActionExecutor::Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
{
    if (!Context.bIsAttacking)
    {
        return;
    }

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

    if (Manager->CombatQuery)
    {
        Manager->CombatQuery->RequestAttackForActor(OwnerActor, nullptr);
    }
}
