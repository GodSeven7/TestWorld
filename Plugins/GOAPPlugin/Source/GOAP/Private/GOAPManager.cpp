#include "GOAPManager.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "GOAPPlanner.h"
#include "GOAPActionRegistry.h"
#include "GOAPGameActionExecutors.h"
#include "Async/ParallelFor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UGOAPManager::SetCombatQueryService(ICombatQueryService* InService)
{
    CombatQuery = InService;
}

void UGOAPManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ActionRegistry = NewObject<UGOAPActionRegistry>();
    bActionSetRegistered = false;

    RegisterExecuter();
}

void UGOAPManager::Deinitialize()
{
    Agents.Empty();
    ObjectIDToIndex.Empty();
    ActionSets.Empty();
    PlanCache.Empty();
    if (ActionRegistry)
    {
        ActionRegistry->ClearAll();
        ActionRegistry = nullptr;
    }
    Super::Deinitialize();
}

bool UGOAPManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

int32 UGOAPManager::RegisterAgent(IGOAPAgentInterface* Agent)
{
    if (!Agent)
    {
        return INDEX_NONE;
    }

    // Use ObjectID from UnifiedData instead of internal auto-increment ID
    int32 ObjID = Agent->GetObjectID();
    if (ObjID == INDEX_NONE)
    {
        return INDEX_NONE;
    }

    const UGOAPAgentConfig* Config = Agent->GetAgentConfig();
    int32 ActionSetID = Config ? Config->ActionSetID : 1;

    int32 Index = Agents.AddDefaulted();

    FGOAPAgentData& Data = Agents[Index];
    Data.ObjectID = ObjID;
    Data.AgentObject = Agent;
    Data.ActionSetID = ActionSetID;
    Data.State = EGOAPAgentState::NeedsPlan;

    ObjectIDToIndex.Add(ObjID, Index);

    return ObjID;
}

void UGOAPManager::UnregisterAgent(IGOAPAgentInterface* Agent)
{
    UnregisterExecuter(Agent);

    int32 ObjID = Agent->GetObjectID();
    if (int32* Index = ObjectIDToIndex.Find(ObjID))
    {
        int32 Idx = *Index;

        Agents.RemoveAtSwap(Idx);

        if (Idx < Agents.Num())
        {
            int32 MovedObjID = Agents[Idx].ObjectID;
            ObjectIDToIndex[MovedObjID] = Idx;
        }

        ObjectIDToIndex.Remove(ObjID);
    }
}

void UGOAPManager::SetAgentGoal(int32 ObjectID, const FGOAPWorldState& Goal)
{
    if (FGOAPAgentData* Data = FindAgentData(ObjectID))
    {
        if (Data->GoalState == Goal)
        {
            if (Data->State == EGOAPAgentState::Completed)
            {
                Data->State = EGOAPAgentState::NeedsPlan;
            }
            return;
        }
        Data->GoalState = Goal;
        Data->State = EGOAPAgentState::NeedsPlan;
        Data->Plan.Empty();
        Data->CurrentActionIndex = 0;
    }
}

void UGOAPManager::RequestReplan(int32 ObjectID)
{
    if (FGOAPAgentData* Data = FindAgentData(ObjectID))
    {
        Data->State = EGOAPAgentState::NeedsPlan;
        Data->Plan.Empty();
        Data->CurrentActionIndex = 0;
    }
}

EGOAPAgentState UGOAPManager::GetAgentState(int32 ObjectID) const
{
    if (const FGOAPAgentData* Data = FindAgentData(ObjectID))
    {
        return Data->State;
    }
    return EGOAPAgentState::Idle;
}

const TArray<int32>& UGOAPManager::GetAgentPlan(int32 ObjectID) const
{
    static TArray<int32> EmptyPlan;
    if (const FGOAPAgentData* Data = FindAgentData(ObjectID))
    {
        return Data->Plan;
    }
    return EmptyPlan;
}

int32 UGOAPManager::GetAgentActionIndex(int32 ObjectID) const
{
    if (const FGOAPAgentData* Data = FindAgentData(ObjectID))
    {
        return Data->CurrentActionIndex;
    }
    return 0;
}

void UGOAPManager::RegisterActionSet(int32 SetID, UGOAPActionSet* ActionSet)
{
    if (ActionSet)
    {
        ActionSets.Add(SetID, ActionSet);
    }
}

void UGOAPManager::TickManager(float DeltaTime)
{
	UpdateWorldStates();
	UpdateGoals();
	BatchPlan();
	ExecuteActions(DeltaTime);
	ApplyContexts(DeltaTime);
}

void UGOAPManager::UpdateWorldStates()
{
    SCOPED_NAMED_EVENT(GOAP_UpdateWorldStates, FColor::Green);

    ParallelFor(Agents.Num(), [this](int32 Index)
    {
        FGOAPAgentData& Data = Agents[Index];
        if (IGOAPAgentInterface* Agent = Data.AgentObject)
        {
            if (Agent->IsGOAPActive())
            {
                Agent->UpdateWorldState(Data.CurrentWorldState);
            }
        }
    });
}

void UGOAPManager::UpdateGoals()
{
    SCOPED_NAMED_EVENT(GOAP_UpdateGoals, FColor::Blue);

    TArray<int32> DirtyAgentIndices;
    for (int32 i = 0; i < Agents.Num(); ++i)
    {
        FGOAPAgentData& Data = Agents[i];
        if (!Data.AgentObject || !Data.AgentObject->IsGOAPActive())
        {
            continue;
        }

        if (Data.CurrentWorldState.bDirty)
        {
            DirtyAgentIndices.Add(i);
        }
        else if (Data.State == EGOAPAgentState::Completed
                 && Data.Plan.Num() == 0
                 && !FGOAPPlanner::IsGoal(Data.CurrentWorldState, Data.GoalState))
        {
            Data.State = EGOAPAgentState::NeedsPlan;
        }
    }

    if (DirtyAgentIndices.Num() == 0)
    {
        return;
    }

    ParallelFor(DirtyAgentIndices.Num(), [&, this](int32 LocalIndex)
    {
        int32 AgentIndex = DirtyAgentIndices[LocalIndex];
        FGOAPAgentData& Data = Agents[AgentIndex];
        if (IGOAPAgentInterface* Agent = Data.AgentObject)
        {
            Agent->SelectGoal(Data.CurrentWorldState);
            Data.CurrentWorldState.ClearDirty();
        }
    });
}

void UGOAPManager::BatchPlan()
{
    SCOPED_NAMED_EVENT(GOAP_BatchPlan, FColor::Magenta);

    TArray<int32> NeedsPlanIndices;
    for (int32 i = 0; i < Agents.Num(); ++i)
    {
        if (Agents[i].State == EGOAPAgentState::NeedsPlan || Agents[i].State == EGOAPAgentState::Failed)
        {
            NeedsPlanIndices.Add(i);
        }
    }

    if (NeedsPlanIndices.Num() == 0)
    {
        return;
    }

    TMap<uint32, TArray<int32>> LocalCache;
    FRWLock LocalCacheLock;

    int32 PlansToProcess = FMath::Min(NeedsPlanIndices.Num(), MaxPlansPerFrame);

    ParallelFor(PlansToProcess, [&, this](int32 PlanIndex)
    {
        int32 AgentIndex = NeedsPlanIndices[PlanIndex];
        FGOAPAgentData& Data = Agents[AgentIndex];

        uint32 CacheKey = MakeCacheKey(Data.CurrentWorldState, Data.GoalState, Data.ActionSetID);

        {
            FReadScopeLock ReadLock(CacheLock);
            if (TArray<int32>* CachedPlan = PlanCache.Find(CacheKey))
            {
                Data.Plan = *CachedPlan;
                Data.State = EGOAPAgentState::Executing;
                Data.CurrentActionIndex = 0;
                if (IGOAPAgentInterface* Agent = Data.AgentObject)
                {
                    Agent->OnPlanStarted();
                }
                return;
            }
        }

        {
            FReadScopeLock ReadLock(LocalCacheLock);
            if (TArray<int32>* CachedPlan = LocalCache.Find(CacheKey))
            {
                Data.Plan = *CachedPlan;
                Data.State = EGOAPAgentState::Executing;
                Data.CurrentActionIndex = 0;
                if (IGOAPAgentInterface* Agent = Data.AgentObject)
                {
                    Agent->OnPlanStarted();
                }
                return;
            }
        }

        UGOAPActionSet* ActionSet = nullptr;
        {
            FReadScopeLock ReadLock(CacheLock);
            ActionSet = ActionSets.FindRef(Data.ActionSetID);
        }

        if (ActionSet)
        {
            TArray<int32> NewPlan;
            if (FGOAPPlanner::Plan(Data.CurrentWorldState, Data.GoalState, ActionSet->GetActions(), NewPlan))
            {
                if (NewPlan.Num() == 0)
                {
                    if (FGOAPPlanner::IsGoal(Data.CurrentWorldState, Data.GoalState))
                    {
                        if (Data.HasPlan())
                        {
                            Data.State = EGOAPAgentState::Executing;
                        }
                        else
                        {
                            Data.State = EGOAPAgentState::NeedsPlan;
                        }
                    }
                    else
                    {
                        Data.State = EGOAPAgentState::Failed;
                    }
                }
                else
                {
                    Data.Plan = NewPlan;
                    Data.State = EGOAPAgentState::Executing;
                    Data.CurrentActionIndex = 0;
                    if (IGOAPAgentInterface* Agent = Data.AgentObject)
                    {
                        Agent->OnPlanStarted();
                    }
                }

                FWriteScopeLock WriteLock(LocalCacheLock);
                LocalCache.Add(CacheKey, NewPlan);
            }
            else
            {
                Data.State = EGOAPAgentState::Failed;
            }
        }
        else
        {
            Data.State = EGOAPAgentState::Failed;
        }
    });

    {
        FWriteScopeLock WriteLock(CacheLock);
        for (auto& Pair : LocalCache)
        {
            PlanCache.Add(Pair.Key, Pair.Value);
        }
    }
}

void UGOAPManager::ExecuteActions(float DeltaTime)
{
    SCOPED_NAMED_EVENT(GOAP_ExecuteActions, FColor::Yellow);

    ParallelFor(Agents.Num(), [this, DeltaTime](int32 Index)
    {
        FGOAPAgentData& Data = Agents[Index];

		Data.Context.Reset();
        if (Data.State != EGOAPAgentState::Executing)
        {
            return;
        }

        IGOAPAgentInterface* Agent = Data.AgentObject;
        if (!Agent || !Agent->IsGOAPActive())
        {
            return;
        }

        int32 ActionID = Data.GetCurrentActionID();

        if (ActionID == 0)
        {
            return;
        }

        if (ActionRegistry)
        {
            TSharedPtr<FGOAPActionExecutor> Executor = ActionRegistry->GetExecutor(Data.ActionSetID, ActionID);
            if (Executor.IsValid())
            {
                if (Data.EnteredActionID != ActionID)
                {
                    Executor->OnEnter(Agent);
                    Data.EnteredActionID = ActionID;
                }
                Executor->Execute(Agent, Data.Context, DeltaTime, Data.CurrentWorldState);
            }
        }

        if (Data.Context.bActionFailed)
        {
            if (Data.EnteredActionID != 0)
            {
                if (ActionRegistry)
                {
                    TSharedPtr<FGOAPActionExecutor> Executor = ActionRegistry->GetExecutor(Data.ActionSetID, Data.EnteredActionID);
                    if (Executor.IsValid())
                    {
                        Executor->OnExit(Agent, EGOAPActionExitReason::Failed);
                    }
                }
                Data.EnteredActionID = 0;
            }
            Data.State = EGOAPAgentState::Failed;
        }
    });
}

void UGOAPManager::ApplyContexts(float DeltaTime)
{
    SCOPED_NAMED_EVENT(GOAP_ApplyContexts, FColor::Cyan);

    for (FGOAPAgentData& Data : Agents)
    {
        if (Data.State != EGOAPAgentState::Executing &&
            Data.State != EGOAPAgentState::Completed)
        {
            continue;
        }

        IGOAPAgentInterface* Agent = Data.AgentObject;
        if (!Agent)
        {
            continue;
        }

        int32 ActionID = Data.GetCurrentActionID();

        if (ActionRegistry)
        {
            TSharedPtr<FGOAPActionExecutor> Executor = ActionRegistry->GetExecutor(Data.ActionSetID, ActionID);
            if (Executor.IsValid())
            {
                Executor->Apply(Agent, Data.Context, DeltaTime, Data.CurrentWorldState);
            }
        }

        Agent->ApplyContext(Data.Context, DeltaTime);

        if (Data.Context.bActionCompleted)
        {
            int32 CompletedActionID = Data.Plan.IsValidIndex(Data.CurrentActionIndex)
                ? Data.Plan[Data.CurrentActionIndex]
                : 0;

            // OnExit
            if (Data.EnteredActionID != 0 && ActionRegistry)
            {
                TSharedPtr<FGOAPActionExecutor> ExitExecutor = ActionRegistry->GetExecutor(Data.ActionSetID, Data.EnteredActionID);
                if (ExitExecutor.IsValid())
                {
                    EGOAPActionExitReason ExitReason = Data.Context.bActionAborted
                        ? EGOAPActionExitReason::Aborted
                        : EGOAPActionExitReason::Completed;
                    ExitExecutor->OnExit(Agent, ExitReason);
                }
                Data.EnteredActionID = 0;
            }

            Agent->OnActionCompleted(CompletedActionID);

            Data.CurrentActionIndex++;
            if (Data.CurrentActionIndex >= Data.Plan.Num())
            {
                Data.State = EGOAPAgentState::Completed;
                Agent->OnPlanCompleted(true);
                Data.Plan.Empty();
                Data.CurrentActionIndex = 0;
            }
        }
    }
}

uint32 UGOAPManager::MakeCacheKey(const FGOAPWorldState& State, const FGOAPWorldState& Goal, int32 ActionSetID)
{
    uint32 Hash = State.GetHash();
    Hash = HashCombine(Hash, Goal.GetHash());
    Hash = HashCombine(Hash, GetTypeHash(ActionSetID));
    return Hash;
}

FGOAPAgentData* UGOAPManager::FindAgentData(int32 ObjectID)
{
    if (int32* Index = ObjectIDToIndex.Find(ObjectID))
    {
        return &Agents[*Index];
    }
    return nullptr;
}

const FGOAPAgentData* UGOAPManager::FindAgentData(int32 ObjectID) const
{
    if (const int32* Index = ObjectIDToIndex.Find(ObjectID))
    {
        return &Agents[*Index];
    }
    return nullptr;
}
