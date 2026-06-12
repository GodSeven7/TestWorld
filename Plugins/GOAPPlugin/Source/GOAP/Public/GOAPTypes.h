#pragma once

#include "CoreMinimal.h"
#include "GOAPTypes.generated.h"

UENUM()
enum class EGOAPAgentState : uint8
{
    Idle,
    NeedsPlan,
    Planning,
    Executing,
    Completed,
    Failed,
};

USTRUCT()
struct GOAP_API FGOAPWorldState
{
    GENERATED_BODY()

    uint32 StateBits = 0;
    bool bDirty = false;

    bool HasState(int32 Key) const
    {
        return (StateBits & (1u << Key)) != 0;
    }

    int32 GetState(int32 Key, int32 Default = 0) const
    {
        return (StateBits & (1u << Key)) ? 1 : 0;
    }

    void SetState(int32 Key, int32 Value)
    {
        uint32 Bit = (1u << Key);
        bool OldValue = (StateBits & Bit) != 0;
        if (OldValue != (Value != 0))
        {
            bDirty = true;
        }
        if (Value)
        {
            StateBits |= Bit;
        }
        else
        {
            StateBits &= ~Bit;
        }
    }

    bool Matches(const FGOAPWorldState& Other) const
    {
        uint32 GoalMask = Other.StateBits;
        uint32 CurrentRelevant = StateBits & GoalMask;
        return CurrentRelevant == GoalMask;
    }

    uint32 GetHash() const
    {
        return StateBits;
    }

    bool operator==(const FGOAPWorldState& Other) const
    {
        return StateBits == Other.StateBits;
    }

    void Reset()
    {
        StateBits = 0;
        bDirty = false;
    }

    void ClearDirty() { bDirty = false; }

    bool IsEmpty() const { return StateBits == 0; }
};

USTRUCT()
struct GOAP_API FGOAPActionData
{
    GENERATED_BODY()

    int32 ActionID = 0;
    FName ActionName;
    float BaseCost = 1.0f;

    enum { kMaxConditions = 4 };
    int32 PreconditionKeys[kMaxConditions];
    int32 PreconditionValues[kMaxConditions];
    int32 PreconditionNum;

    int32 EffectKeys[kMaxConditions];
    int32 EffectValues[kMaxConditions];
    int32 EffectNum;

    FGOAPActionData()
    {
        PreconditionNum = 0;
        EffectNum = 0;
    }

    void AddPrecondition(int32 Key, int32 Value)
    {
        if (PreconditionNum < kMaxConditions)
        {
            PreconditionKeys[PreconditionNum] = Key;
            PreconditionValues[PreconditionNum] = Value;
            ++PreconditionNum;
        }
    }

    void AddEffect(int32 Key, int32 Value)
    {
        if (EffectNum < kMaxConditions)
        {
            EffectKeys[EffectNum] = Key;
            EffectValues[EffectNum] = Value;
            ++EffectNum;
        }
    }

    bool CheckPreconditions(const FGOAPWorldState& State) const
    {
        for (int32 i = 0; i < PreconditionNum; ++i)
        {
            if (State.GetState(PreconditionKeys[i]) != PreconditionValues[i])
            {
                return false;
            }
        }
        return true;
    }

    void ApplyEffects(FGOAPWorldState& State) const
    {
        for (int32 i = 0; i < EffectNum; ++i)
        {
            State.SetState(EffectKeys[i], EffectValues[i]);
        }
    }

};

USTRUCT()
struct GOAP_API FGOAPAgentContext
{
    GENERATED_BODY()

    UPROPERTY()
    float MoveSpeed = 0.0f;

    UPROPERTY()
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector LookDirection = FVector::ZeroVector;

    UPROPERTY()
    TWeakObjectPtr<UObject> TargetObject;

    UPROPERTY()
    FVector DesiredPosition = FVector::ZeroVector;

    bool bHasDesiredPosition = false;

    bool bActionCompleted = false;
    bool bActionFailed = false;
    bool bActionAborted = false;
    bool bIsAttacking = false;
    bool bRequestSurroundAssignment = false;

    void Reset()
    {
        MoveSpeed = 0.0f;
        TargetLocation = FVector::ZeroVector;
        TargetObject.Reset();
        DesiredPosition = FVector::ZeroVector;
        bHasDesiredPosition = false;
        bActionCompleted = false;
        bActionFailed = false;
        bActionAborted = false;
        bIsAttacking = false;
        bRequestSurroundAssignment = false;
    }
};

USTRUCT()
struct GOAP_API FGOAPAgentData
{
    GENERATED_BODY()

    int32 ObjectID = INDEX_NONE;
    class IGOAPAgentInterface* AgentObject;

    FGOAPWorldState CurrentWorldState;
    FGOAPWorldState GoalState;

    TArray<int32> Plan;
    int32 CurrentActionIndex = 0;

    EGOAPAgentState State = EGOAPAgentState::Idle;
    int32 ActionSetID = 0;

    FGOAPAgentContext Context;

    int32 EnteredActionID = 0;

    bool HasPlan() const { return Plan.Num() > 0; }
    bool IsPlanComplete() const { return CurrentActionIndex >= Plan.Num(); }
    int32 GetCurrentActionID() const
    {
        return HasPlan() && !IsPlanComplete() ? Plan[CurrentActionIndex] : 0;
    }
};
