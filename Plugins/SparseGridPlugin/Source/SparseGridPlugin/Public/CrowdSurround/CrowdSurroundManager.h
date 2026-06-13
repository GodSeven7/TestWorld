#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CrowdSurroundRuntimeTypes.h"
#include "CrowdSurroundService.h"
#include "CrowdSurroundManager.generated.h"

struct FSparseGridArcObstacle;
class ISparseGridObject;
class USparseGridComponent;
class USparseGridConfig;

UCLASS()
class SPARSEGRIDPLUGIN_API UCrowdSurroundManager : public UWorldSubsystem, public ICrowdSurroundService
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    // --- ICrowdSurroundService ---

    virtual bool RequestSurroundAssignment(const FCrowdSurroundRequest& Request) override;

    virtual bool LockAttackSlot(int32 ObjectID) override;
    virtual void UnlockAttackSlot(int32 ObjectID) override;

    virtual bool LockWaitSlot(int32 ObjectID) override;
    virtual void UnlockWaitSlot(int32 ObjectID) override;

    virtual void ClearSurroundState(int32 ObjectID) override;

    virtual void Update(float DeltaTime) override;

    virtual bool GetSurroundAssignment(int32 ObjectID, FCrowdSurroundAssignment& OutAssignment) const override;
    virtual bool IsInSurroundGroup(int32 ObjectID) const override;

    virtual bool GetFixedSlotPosition(int32 ObjectID, FVector& OutPosition) const override;

    // --- Configuration ---
    void SetConfig(USparseGridConfig* InConfig) { Config = InConfig; }

private:
    // Group storage: TargetObjectID → Group data
    TMap<int32, FCrowdSurroundGroup> SurroundGroups;

    // Agent intent: ObjectID → {TargetObjectID, AttackRange, CollisionRadius, RequestTime}
    TMap<int32, FCrowdSurroundAgentIntent> AgentIntents;

    // Attack slots: agents actively attacking
    TMap<int32, FLockedSurroundSlot> AttackSlotList;

    // Wait slots: agents waiting for attack opportunity
    TMap<int32, FLockedSurroundSlot> WaitSlotList;

    // Cached assignments: ObjectID → last computed assignment
    TMap<int32, FCrowdSurroundAssignment> Assignments;

    UPROPERTY()
    TObjectPtr<USparseGridConfig> Config = nullptr;

    mutable FCriticalSection Mutex;

    USparseGridComponent* ResolveGridComponent(UObject* Object) const;

    TArray<int32> GetAgentsForGroup(int32 TargetObjectID) const;

    FVector GetAgentPosition(int32 ObjectID, const FCrowdSurroundGroup& Group) const;
    float GetAgentCollisionRadius(int32 ObjectID) const;

    // Build arc obstacles from both attack and wait slot lists for the given target.
    void BuildArcObstaclesForGroup(int32 TargetObjectID, int32 ExcludedObjectID, TArray<FSparseGridArcObstacle>& OutObstacles) const;

    // Recompute DesiredPosition for all agents with surround intent.
    void RecomputeAssignments();

    // Shared lock/unlock helpers
    bool LockSlot(int32 ObjectID, ECrowdSurroundSlotType SlotType);
    void UnlockSlot(int32 ObjectID);

    void DrawDebugCrowd() const;
};
