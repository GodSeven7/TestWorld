#include "GOAP.h"
#include "GOAPManager.h"
#include "GOAPActionSet.h"
#include "GOAPActionRegistry.h"
#include "GOAPGameTypes.h"
#include "GOAPGameActionExecutors.h"

DEFINE_LOG_CATEGORY(LogGOAP);

static void RegisterDefaultActions(UGOAPManager* Manager)
{
    if (!Manager)
    {
        return;
    }
	if (Manager->IsActionSetRegistered())
	{
		return;
	}

	Manager->MarkActionSetRegistered();

    UGOAPActionSet* DefaultActionSet = NewObject<UGOAPActionSet>();

    FGOAPActionData PatrolAction;
    PatrolAction.ActionID = EGOAPGameAction::Patrol;
    PatrolAction.ActionName = TEXT("Patrol");
    PatrolAction.BaseCost = 1.0f;
    PatrolAction.AddPrecondition(EGOAPGameStateKey::HasPatrolPath, 1);
    PatrolAction.AddEffect(EGOAPGameStateKey::IsPatrolling, 1);
    DefaultActionSet->AddAction(PatrolAction);

    FGOAPActionData ChaseAction;
    ChaseAction.ActionID = EGOAPGameAction::Chase;
    ChaseAction.ActionName = TEXT("Chase");
    ChaseAction.BaseCost = 2.0f;
    ChaseAction.AddPrecondition(EGOAPGameStateKey::HasTarget, 1);
    ChaseAction.AddEffect(EGOAPGameStateKey::TargetInAttackRange, 1);
    DefaultActionSet->AddAction(ChaseAction);

    FGOAPActionData AttackAction;
    AttackAction.ActionID = EGOAPGameAction::Attack;
    AttackAction.ActionName = TEXT("Attack");
    AttackAction.BaseCost = 3.0f;
    AttackAction.AddPrecondition(EGOAPGameStateKey::HasTarget, 1);
    AttackAction.AddPrecondition(EGOAPGameStateKey::TargetInAttackRange, 1);
    AttackAction.AddEffect(EGOAPGameStateKey::EnemyDead, 1);
    DefaultActionSet->AddAction(AttackAction);

    FGOAPActionData ReturnAction;
    ReturnAction.ActionID = EGOAPGameAction::ReturnToPatrol;
    ReturnAction.ActionName = TEXT("ReturnToPatrol");
    ReturnAction.BaseCost = 1.5f;
    ReturnAction.AddPrecondition(EGOAPGameStateKey::TargetOutOfRange, 1);
    ReturnAction.AddEffect(EGOAPGameStateKey::AtHomeLocation, 1);
    DefaultActionSet->AddAction(ReturnAction);

    FGOAPActionData MoveToSurroundAction;
    MoveToSurroundAction.ActionID = EGOAPGameAction::MoveToSurroundPosition;
    MoveToSurroundAction.ActionName = TEXT("MoveToSurroundPosition");
    MoveToSurroundAction.BaseCost = 2.5f;
    MoveToSurroundAction.AddPrecondition(EGOAPGameStateKey::HasTarget, 1);
    MoveToSurroundAction.AddPrecondition(EGOAPGameStateKey::HasSurroundAssignment, 1);
    MoveToSurroundAction.AddEffect(EGOAPGameStateKey::AtSurroundPosition, 1);
    DefaultActionSet->AddAction(MoveToSurroundAction);

    FGOAPActionData WaitAction;
    WaitAction.ActionID = EGOAPGameAction::WaitForAttackOpportunity;
    WaitAction.ActionName = TEXT("WaitForAttackOpportunity");
    WaitAction.BaseCost = 2.0f;
    WaitAction.AddPrecondition(EGOAPGameStateKey::HasTarget, 1);
    WaitAction.AddPrecondition(EGOAPGameStateKey::HasSurroundAssignment, 1);
    WaitAction.AddPrecondition(EGOAPGameStateKey::AtSurroundPosition, 1);
    WaitAction.AddEffect(EGOAPGameStateKey::TargetInAttackRange, 1);
    DefaultActionSet->AddAction(WaitAction);

    Manager->RegisterActionSet(1, DefaultActionSet);

    static TSharedPtr<FPatrolActionExecutor> PatrolExecutor = MakeShared<FPatrolActionExecutor>();
    static TSharedPtr<FChaseActionExecutor> ChaseExecutor = MakeShared<FChaseActionExecutor>();
    static TSharedPtr<FAttackActionExecutor> AttackExecutor = MakeShared<FAttackActionExecutor>();
    static TSharedPtr<FReturnActionExecutor> ReturnExecutor = MakeShared<FReturnActionExecutor>();
    static TSharedPtr<FMoveToSurroundPositionExecutor> MoveToSurroundExecutor = MakeShared<FMoveToSurroundPositionExecutor>();
    static TSharedPtr<FWaitForAttackOpportunityExecutor> WaitExecutor = MakeShared<FWaitForAttackOpportunityExecutor>();

    Manager->GetActionRegistry()->RegisterExecutor(1, EGOAPGameAction::Patrol, PatrolExecutor);
    Manager->GetActionRegistry()->RegisterExecutor(1, EGOAPGameAction::Chase, ChaseExecutor);
    Manager->GetActionRegistry()->RegisterExecutor(1, EGOAPGameAction::Attack, AttackExecutor);
    Manager->GetActionRegistry()->RegisterExecutor(1, EGOAPGameAction::ReturnToPatrol, ReturnExecutor);
    Manager->GetActionRegistry()->RegisterExecutor(1, EGOAPGameAction::MoveToSurroundPosition, MoveToSurroundExecutor);
    Manager->GetActionRegistry()->RegisterExecutor(1, EGOAPGameAction::WaitForAttackOpportunity, WaitExecutor);
}

class FGOAPModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        IModuleInterface::StartupModule();
    }

    virtual void ShutdownModule() override
    {
        IModuleInterface::ShutdownModule();
    }
};

IMPLEMENT_MODULE(FGOAPModule, GOAP);

void RegisterGOAPDefaultActions(UWorld* World)
{
    if (UGOAPManager* Manager = World->GetSubsystem<UGOAPManager>())
    {
        RegisterDefaultActions(Manager);
    }
}
