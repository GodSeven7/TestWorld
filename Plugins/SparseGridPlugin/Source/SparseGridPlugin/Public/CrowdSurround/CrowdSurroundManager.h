#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CrowdSurroundRuntimeTypes.h"
#include "CrowdSurroundService.h"
#include "CrowdSurroundManager.generated.h"

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

    // Lightweight intent registration: records that ObjectID wants to surround TargetObject.
    // Triggers group assignment recomputation. Returns the computed assignment.
    virtual bool RequestSurroundAssignment(const FCrowdSurroundRequest& Request, FCrowdSurroundAssignment& OutAssignment) override;

    // Lock an attack slot: marks the agent as occupying an attack position.
    // Records position + radius from SparseGrid. Triggers group recomputation.
    virtual bool LockSurroundAssignment(int32 ObjectID) override;

    // Unlock an attack slot: removes the lock and triggers group recomputation.
    virtual void UnlockSurroundAssignment(int32 ObjectID) override;

    // Cleanup invalid groups only (no periodic recalculation).
    virtual void Update(float DeltaTime) override;

    // Recompute all groups that have dirty LockedSlots state.
    // Called once per frame from TickCrowdSurroundCorrection.
    void RecomputeAllDirtyGroups();

    // Return cached assignment for an agent.
    virtual bool GetSurroundAssignment(int32 ObjectID, FCrowdSurroundAssignment& OutAssignment) const override;

    // Check if agent has registered intent to surround a target.
    virtual bool IsInSurroundGroup(int32 ObjectID) const override;

    // --- Configuration ---
    void SetConfig(USparseGridConfig* InConfig) { Config = InConfig; }

    // --- Position queries (used by Solver) ---
    bool TryGetMoveToAttackPosition(const FCrowdSurroundGroup& Group, const FCrowdSurroundCandidate& Candidate, FVector& OutMovePosition) const;
    bool TryGetMoveToWaitPosition(const FCrowdSurroundGroup& Group, const FCrowdSurroundCandidate& Candidate, FVector& OutMovePosition) const;

private:
    // Group storage: TargetObjectID → Group data
    TMap<int32, FCrowdSurroundGroup> SurroundGroups;

    // Agent intent: ObjectID → {TargetObjectID, AttackRange, CollisionRadius, RequestTime}
    TMap<int32, FCrowdSurroundAgentIntent> AgentIntents;

    // Locked attack slots: ObjectID → {Position, Radius, LockTime, ProgressTracking}
    TMap<int32, FLockedSurroundSlot> LockedSlots;

    // Cached assignments: ObjectID → last computed assignment
    TMap<int32, FCrowdSurroundAssignment> Assignments;

    UPROPERTY()
    TObjectPtr<USparseGridConfig> Config = nullptr;

    // --- Group management ---
    USparseGridComponent* ResolveGridComponent(UObject* Object) const;

    // Build candidate list on-the-fly from AgentIntents + SparseGrid
    TMap<int32, FCrowdSurroundCandidate> BuildCandidatesForGroup(const FCrowdSurroundGroup& Group) const;

    // Get all ObjectIDs targeting a specific group
    TArray<int32> GetAgentsForGroup(int32 TargetObjectID) const;

    // Recompute assignments for a group (triggered by Lock/Unlock/Request)
    void RecomputeGroupAssignments(int32 TargetObjectID);

    // --- Core computation (Solver) ---
    void ComputeGroupMetrics(FCrowdSurroundGroup& Group, const TMap<int32, FCrowdSurroundCandidate>& Candidates) const;
    void AssignSurroundIntents(FCrowdSurroundGroup& Group, const TMap<int32, FCrowdSurroundCandidate>& Candidates);
    void CollectOccupiedPoints(const FCrowdSurroundGroup& Group, TArray<FCrowdSurroundOccupiedPoint>& OutOccupiedPoints) const;
    bool IsAttackPositionAvailable(const FCrowdSurroundCandidate& Candidate, const FVector& Position, const TArray<FCrowdSurroundOccupiedPoint>& OccupiedPoints) const;
    bool IsWaitPositionAvailable(const FCrowdSurroundGroup& Group, const FCrowdSurroundCandidate& Candidate, const FVector& Position, const TArray<FCrowdSurroundOccupiedPoint>& OccupiedPoints) const;
    int32 FindNearestAttackAnchorID(const FCrowdSurroundGroup& Group, const FVector& Position) const;
    void WriteAttackAssignment(FCrowdSurroundGroup& Group, int32 ObjectID, const FCrowdSurroundCandidate& Candidate, const FVector& AttackPosition, ECrowdAttackAnchorState AnchorState, double Now);
    void WriteWaitAssignment(FCrowdSurroundGroup& Group, int32 ObjectID, const FCrowdSurroundCandidate& Candidate, const FVector& WaitPosition, int32 ParentAnchorID, int32 WaitIndex);

    // --- Parameter queries ---
    FVector GetAgentPosition(int32 ObjectID, const FCrowdSurroundGroup& Group) const;
    float GetAgentOccupancyRadius(int32 ObjectID) const;
    float GetAgentContactPadding(int32 ObjectID) const;
    float GetAgentCollisionRadius(int32 ObjectID) const;
    float GetAgentAttackRange(int32 ObjectID) const;
    int32 CalculateCircularCapacity(float Radius, float Spacing) const;
    bool HasLineOfMovement(const FVector& Position) const;

    // --- Debug ---
    void DrawDebugCrowd() const;
};
