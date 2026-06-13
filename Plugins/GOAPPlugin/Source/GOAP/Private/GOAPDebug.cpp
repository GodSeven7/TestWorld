#include "GOAPDebug.h"
#include "GOAPManager.h"
#include "GOAPActionSet.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarShowAction(
    TEXT("GOAP.ShowAction"),
    0,
    TEXT("Show current GOAP action name above each agent (0=Off, 1=On)"),
    ECVF_Default
);

bool FGOAPDebug::IsActionDebugEnabled()
{
    return CVarShowAction.GetValueOnGameThread() != 0;
}

void FGOAPDebug::DrawDebug(UWorld* World, UGOAPManager* Manager)
{
    if (!World || !Manager)
    {
        return;
    }

    constexpr float LifeTime = 0.02f;

    for (const FGOAPAgentData& Data : Manager->Agents)
    {
        if (Data.State != EGOAPAgentState::Executing)
        {
            continue;
        }

        IGOAPAgentInterface* Agent = Data.AgentObject;
        if (!Agent)
        {
            continue;
        }

        const int32 ActionID = Data.GetCurrentActionID();
        if (ActionID == 0)
        {
            continue;
        }

        // Look up action name from ActionSets
        FName ActionName;
        if (UGOAPActionSet* const* SetPtr = Manager->ActionSets.Find(Data.ActionSetID))
        {
            const TArray<FGOAPActionData>& Actions = (*SetPtr)->GetActions();
            for (const FGOAPActionData& Action : Actions)
            {
                if (Action.ActionID == ActionID)
                {
                    ActionName = Action.ActionName;
                    break;
                }
            }
        }

        if (ActionName.IsNone())
        {
            ActionName = FName(*FString::Printf(TEXT("ID:%d"), ActionID));
        }

        FVector Position = Agent->QueryWorldPosition();
        Position.Z += 80.0f;

        DrawDebugString(World, Position, ActionName.ToString(), nullptr, FColor::Cyan, LifeTime, false, 1.0f);
    }
}
