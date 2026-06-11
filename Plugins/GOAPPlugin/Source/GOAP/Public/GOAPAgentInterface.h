#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GOAPTypes.h"
#include "GOAPAgentInterface.generated.h"

class UGOAPAgentConfig;
class UActorUnifiedDataComponent;

UINTERFACE(MinimalAPI)
class UGOAPAgentInterface : public UInterface
{
    GENERATED_BODY()
};

class GOAP_API IGOAPAgentInterface
{
    GENERATED_BODY()

public:

    // ─── A. 外部输入数据（GOAP查询，外部子类必须实现） ───

    virtual FVector QueryWorldPosition() const = 0;
    virtual bool QueryHasTarget() const = 0;
    virtual FVector QueryTargetPosition() const = 0;
    virtual UObject* QueryTargetObject() const { return nullptr; }
    virtual float QueryMoveSpeed() const = 0;
    virtual const TArray<FVector>& QueryPatrolPoints() const = 0;
    virtual FVector QuerySpawnedPosition() const = 0;
    virtual AActor* QueryOwnerActor() const = 0;
    virtual int32 QueryFaction() const = 0;
    virtual bool QueryIsAlive() const = 0;
    virtual float QueryAttackRange() const = 0;

    // ─── A. 内部更新方法（更新GOAP内部数据，基类提供默认实现，子类可覆写扩展） ───

    virtual void UpdateWorldState(FGOAPWorldState& OutWorldState) = 0;
    virtual FGOAPWorldState GetGoalState() const = 0;
    virtual void SelectGoal(const FGOAPWorldState& CurrentWorldState) = 0;

    // ─── B. 外部输出数据（GOAP推送，外部子类接收通知） ───

    virtual void ApplyWorldPosition(const FVector& Position) = 0;
    virtual void ApplyWorldRotation(const FRotator& Rotation) = 0;
    virtual void ApplyForwardVector(const FVector& Forward) = 0;
    virtual void ApplyContext(const FGOAPAgentContext& Context, float DeltaTime) = 0;
    virtual void OnPlanStarted() = 0;
    virtual void OnActionCompleted(int32 ActionID) = 0;
    virtual void OnPlanCompleted(bool bSuccess) = 0;
    virtual void OnTargetFound(UActorUnifiedDataComponent* TargetData) = 0;
    virtual void OnTargetLost() = 0;

    // ─── C. 外部配置数据（通过UGOAPAgentConfig DataAsset提供） ───

    virtual const UGOAPAgentConfig* GetAgentConfig() const = 0;

    // ─── D. GOAP内部数据（GOAP自行维护，外部无需关心） ───

    // Returns the ObjectID from UnifiedData. Agents should override this to return their actual ObjectID.
    virtual int32 GetObjectID() const { return INDEX_NONE; }

    EGOAPAgentState GetAgentState() const { return AgentState; }
    void SetAgentState(EGOAPAgentState State) { AgentState = State; }

    bool IsGOAPActive() const { return bIsActive; }
    void SetGOAPActive(bool bActive) { bIsActive = bActive; }

protected:
    EGOAPAgentState AgentState = EGOAPAgentState::Idle;
    bool bIsActive = true;
};
