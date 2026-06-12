#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GOAPTypes.h"
#include "GOAPActionSet.h"
#include "GOAPActionRegistry.h"
#include "SpatialQueryService.h"
#include "CombatQueryService.h"
#include "CrowdSurroundService.h"
#include "GOAPManager.generated.h"

class IGOAPAgentInterface;
class FGOAPDebug;

UCLASS()
class GOAP_API UGOAPManager : public UWorldSubsystem
{
    GENERATED_BODY()

    friend class FGOAPDebug;
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    int32 RegisterAgent(IGOAPAgentInterface* Agent);
    void UnregisterAgent(IGOAPAgentInterface* Agent);

    void SetSpatialQueryService(ISpatialQueryService* InService) { SpatialQuery = InService; }
    ISpatialQueryService* GetSpatialQueryService() const { return SpatialQuery; }

    void SetCombatQueryService(ICombatQueryService* InService);
    ICombatQueryService* GetCombatQueryService() const { return CombatQuery; }

    void SetCrowdSurroundService(ICrowdSurroundService* InService) { CrowdSurroundService = InService; }
    ICrowdSurroundService* GetCrowdSurroundService() const { return CrowdSurroundService; }

    // ─── Surround Request 去重 ───
    bool HasSurroundRequest(int32 ObjectID) const;
    bool CanRequestSurroundAssignment(int32 ObjectID, UObject* TargetObject) const;
    void MarkSurroundRequestSubmitted(int32 ObjectID, UObject* TargetObject);
    void CleanupSurroundRequests();

    void SetAgentGoal(int32 ObjectID, const FGOAPWorldState& Goal);
    void RequestReplan(int32 ObjectID);

    EGOAPAgentState GetAgentState(int32 ObjectID) const;
    const TArray<int32>& GetAgentPlan(int32 ObjectID) const;
    int32 GetAgentActionIndex(int32 ObjectID) const;

    void RegisterActionSet(int32 SetID, UGOAPActionSet* ActionSet);

    UGOAPActionRegistry* GetActionRegistry() { return ActionRegistry; }

	virtual void RegisterExecuter() {};
	virtual void UnregisterExecuter(IGOAPAgentInterface* Agent) {};

	UFUNCTION(BlueprintCallable, Category = "GOAP")
	void TickManager(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "GOAP")
    void UpdateWorldStates();

    UFUNCTION(BlueprintCallable, Category = "GOAP")
    void UpdateGoals();

    UFUNCTION(BlueprintCallable, Category = "GOAP")
    void BatchPlan();

    UFUNCTION(BlueprintCallable, Category = "GOAP")
    void ExecuteActions(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "GOAP")
    void ApplyContexts(float DeltaTime);

	bool IsActionSetRegistered() { return bActionSetRegistered; }
	void MarkActionSetRegistered() { bActionSetRegistered = true; }

    ISpatialQueryService* SpatialQuery = nullptr;
    ICombatQueryService* CombatQuery = nullptr;
    ICrowdSurroundService* CrowdSurroundService = nullptr;

private:
    struct FSurroundRequestState
    {
        TWeakObjectPtr<UObject> TargetObject;
        bool bSubmitted = false;
    };

    bool bActionSetRegistered;
    TArray<FGOAPAgentData> Agents;
    TMap<int32, int32> ObjectIDToIndex;

    TMap<int32, FSurroundRequestState> SurroundRequestStates;

    UPROPERTY()
    TMap<int32, UGOAPActionSet*> ActionSets;

    UPROPERTY()
    UGOAPActionRegistry* ActionRegistry = nullptr;

    TMap<uint32, TArray<int32>> PlanCache;
    mutable FRWLock CacheLock;

    int32 MaxPlansPerFrame = 100;

    uint32 MakeCacheKey(const FGOAPWorldState& State, const FGOAPWorldState& Goal, int32 ActionSetID);

    FGOAPAgentData* FindAgentData(int32 ObjectID);
    const FGOAPAgentData* FindAgentData(int32 ObjectID) const;
};
