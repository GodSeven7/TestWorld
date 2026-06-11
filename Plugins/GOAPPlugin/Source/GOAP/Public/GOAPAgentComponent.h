#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GOAPAgentInterface.h"
#include "GOAPAgentConfig.h"
#include "GOAPManager.h"
#include "SpatialQueryService.h"
#include "ActorUnifiedDataComponent.h"
#include "GOAPAgentComponent.generated.h"

UCLASS(ClassGroup = "GOAP", meta = (BlueprintSpawnableComponent))
class GOAP_API UGOAPAgentComponent : public UActorComponent, public IGOAPAgentInterface
{
    GENERATED_BODY()

public:
    UGOAPAgentComponent();

    UPROPERTY(EditAnywhere, Category = "GOAP")
    TObjectPtr<UGOAPAgentConfig> AgentConfig;

    virtual FVector QueryWorldPosition() const override;
    virtual bool QueryHasTarget() const override { return false; }
    virtual FVector QueryTargetPosition() const override { return FVector::ZeroVector; }
    virtual float QueryMoveSpeed() const override;
    virtual const TArray<FVector>& QueryPatrolPoints() const override;
    virtual FVector QuerySpawnedPosition() const override { return CacheSpawnedPosition; }
    virtual AActor* QueryOwnerActor() const override { return GetOwner(); }
    virtual int32 QueryFaction() const override { return 0; }
    virtual bool QueryIsAlive() const override { return true; }
    virtual float QueryAttackRange() const override;

    virtual void UpdateWorldState(FGOAPWorldState& OutWorldState) override;
    virtual FGOAPWorldState GetGoalState() const override;
    virtual void SelectGoal(const FGOAPWorldState& CurrentWorldState) override;

    virtual void ApplyWorldPosition(const FVector& Position) override;
    virtual void ApplyWorldRotation(const FRotator& Rotation) override;
    virtual void ApplyForwardVector(const FVector& Forward) override;
    virtual void ApplyContext(const FGOAPAgentContext& Context, float DeltaTime) override;
    virtual void OnPlanStarted() override {}
    virtual void OnActionCompleted(int32 ActionID) override {}
    virtual void OnPlanCompleted(bool bSuccess) override {}
    virtual void OnTargetFound(UActorUnifiedDataComponent* TargetData) override {}
    virtual void OnTargetLost() override {}

    virtual const UGOAPAgentConfig* GetAgentConfig() const override { return AgentConfig; }

    void SetGoal(const FGOAPWorldState& NewGoal);
    void RequestReplan();
    const TArray<int32>& GetCurrentPlan() const;
    int32 GetCurrentActionIndex() const;
    bool HasValidPlan() const;
    bool IsExecutingPlan() const;

    UFUNCTION(BlueprintCallable, Category = "GOAP")
    void SetPatrolGoal();

    UFUNCTION(BlueprintCallable, Category = "GOAP")
    void SetCombatGoal();

    void SetSpawnedPosition(const FVector& Position) { CacheSpawnedPosition = Position; }

    void SetAssociatedComponent(USceneComponent* Component) { AssociatedComponent = Component; }
    USceneComponent* GetAssociatedComponent() const { return AssociatedComponent; }

    UPROPERTY(EditAnywhere, Category = "GOAP|Patrol")
    TArray<FVector> PatrolPoints;

    TArray<UActorUnifiedDataComponent*> QueryNearbyUnifiedData(float Radius, int32 ExcludeFactionID = -1) const;

    UActorUnifiedDataComponent* QueryNearestEnemy(float Radius) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    FGOAPWorldState CachedGoal;
    bool bHasCustomGoal = false;
    FVector CacheSpawnedPosition;

    UPROPERTY()
    USceneComponent* AssociatedComponent = nullptr;
};

