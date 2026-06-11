#pragma once

#include "CoreMinimal.h"
#include "GOAPTypes.h"
#include "GOAPActionExecutor.generated.h"

class IGOAPAgentInterface;

UENUM()
enum class EGOAPActionExitReason : uint8
{
    Completed,   // CheckCompletion = true
    Aborted,     // ShouldAbort = true
    Failed,      // bActionFailed = true
    PlanInvalid  // Plan 被作废
};

USTRUCT()
struct GOAP_API FGOAPActionExecutor
{
    GENERATED_BODY()

    virtual ~FGOAPActionExecutor() = default;

    virtual void Execute(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
    {
        if (!CheckPreconditions(Agent, CurrentWorldState))
        {
            Context.bActionFailed = true;
            return;
        }

        if (ShouldAbort(Agent, CurrentWorldState))
        {
            Context.bActionAborted = true;
            Context.bActionCompleted = true;
            return;
        }

        if (CheckCompletion(Agent, CurrentWorldState))
        {
            Context.bActionCompleted = true;
            return;
        }

        ExecuteInternal(Agent, Context, DeltaTime, CurrentWorldState);
    }

    virtual void OnEnter(IGOAPAgentInterface* Agent) {}
    virtual void OnExit(IGOAPAgentInterface* Agent, EGOAPActionExitReason Reason) {}
    virtual void Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) {}

    virtual bool CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const { return true; }
    virtual bool ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const { return false; }
    virtual bool CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const { return false; }
    virtual void ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState)
    {
        Context.bActionCompleted = true;
    }
};
