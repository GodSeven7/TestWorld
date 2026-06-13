#pragma once

#include "CoreMinimal.h"
#include "GOAPActionExecutor.h"
#include "GOAPGameTypes.h"
#include "GOAPGameActionExecutors.generated.h"

class IGOAPAgentInterface;

USTRUCT()
struct GOAP_API FPatrolActionExecutor : public FGOAPActionExecutor
{
    GENERATED_BODY()

    FPatrolActionExecutor()
        : StateLock(MakeShared<FCriticalSection>())
    {}

    struct FAgentState
    {
        int32 CurrentIndex = 0;
        float WaitTimer = 0.0f;
        bool bIsWaiting = false;
    };

    TMap<int32, FAgentState> AgentStates;
    TSharedPtr<FCriticalSection> StateLock;

    virtual bool CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual void ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void OnExit(IGOAPAgentInterface* Agent, EGOAPActionExitReason Reason) override;
};

USTRUCT()
struct GOAP_API FChaseActionExecutor : public FGOAPActionExecutor
{
    GENERATED_BODY()

    virtual bool CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual void ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void OnEnter(IGOAPAgentInterface* Agent) override;
};

USTRUCT()
struct GOAP_API FAttackActionExecutor : public FGOAPActionExecutor
{
    GENERATED_BODY()

    virtual bool CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual void ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void OnEnter(IGOAPAgentInterface* Agent) override;
    virtual void OnExit(IGOAPAgentInterface* Agent, EGOAPActionExitReason Reason) override;
};

USTRUCT()
struct GOAP_API FReturnActionExecutor : public FGOAPActionExecutor
{
    GENERATED_BODY()

    FReturnActionExecutor()
        : StateLock(MakeShared<FCriticalSection>())
    {}

    struct FAgentState
    {
        FVector TargetLocation = FVector::ZeroVector;
        bool bTargetSet = false;
    };

    TMap<int32, FAgentState> AgentStates;
    TSharedPtr<FCriticalSection> StateLock;

    virtual bool CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual void ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void OnExit(IGOAPAgentInterface* Agent, EGOAPActionExitReason Reason) override;
};

USTRUCT()
struct GOAP_API FMoveToSurroundPositionExecutor : public FGOAPActionExecutor
{
    GENERATED_BODY()

    virtual bool CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual void ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
};

USTRUCT()
struct GOAP_API FWaitForAttackOpportunityExecutor : public FGOAPActionExecutor
{
    GENERATED_BODY()

    virtual bool CheckPreconditions(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool ShouldAbort(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual bool CheckCompletion(IGOAPAgentInterface* Agent, const FGOAPWorldState& CurrentWorldState) const override;
    virtual void ExecuteInternal(IGOAPAgentInterface* Agent, FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void Apply(IGOAPAgentInterface* Agent, const FGOAPAgentContext& Context, float DeltaTime, const FGOAPWorldState& CurrentWorldState) override;
    virtual void OnEnter(IGOAPAgentInterface* Agent) override;
    virtual void OnExit(IGOAPAgentInterface* Agent, EGOAPActionExitReason Reason) override;
};
