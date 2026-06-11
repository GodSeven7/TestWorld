#include "CrowdSurroundManager.h"
#include "CrowdSurroundUtilities.h"
#include "SparseGridManager.h"
#include "SparseGridComponent.h"
#include "SparseGridConfig.h"
#include "SparseGridDebug.h"
#include "FlowFieldManager.h"
#include "PathfindingService.h"
#include "GameFramework/Actor.h"

void UCrowdSurroundManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UCrowdSurroundManager::Deinitialize()
{
    AgentIntents.Empty();
    LockedSlots.Empty();
    Assignments.Empty();
    SurroundGroups.Empty();
    Super::Deinitialize();
}

bool UCrowdSurroundManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

USparseGridComponent* UCrowdSurroundManager::ResolveGridComponent(UObject* Object) const
{
    if (!Object)
    {
        return nullptr;
    }

    if (USparseGridComponent* Component = Cast<USparseGridComponent>(Object))
    {
        return Component;
    }

    if (AActor* Actor = Cast<AActor>(Object))
    {
        return Actor->FindComponentByClass<USparseGridComponent>();
    }

    return nullptr;
}

bool UCrowdSurroundManager::RequestSurroundAssignment(const FCrowdSurroundRequest& Request, FCrowdSurroundAssignment& OutAssignment)
{
    OutAssignment = FCrowdSurroundAssignment();

    if (!Request.IsValid())
    {
        return false;
    }

    USparseGridComponent* TargetComponent = ResolveGridComponent(Request.TargetObject);
    if (!TargetComponent)
    {
        return false;
    }

    const int32 TargetObjectID = TargetComponent->GetObjectID();
    if (TargetObjectID == INDEX_NONE)
    {
        return false;
    }

    const int32 ObjectID = Request.ObjectID;

    // If agent already has intent targeting a different target, remove from old group's runtime data
    if (FCrowdSurroundAgentIntent* ExistingIntent = AgentIntents.Find(ObjectID))
    {
        if (ExistingIntent->TargetObjectID != TargetObjectID)
        {
            const int32 OldTargetID = ExistingIntent->TargetObjectID;
            if (FCrowdSurroundGroup* OldGroup = SurroundGroups.Find(OldTargetID))
            {
                // Remove this agent's anchors and wait points from old group
                OldGroup->AttackAnchors.RemoveAll([ObjectID](const FCrowdAttackAnchor& Anchor)
                {
                    return Anchor.OwnerObjectID == ObjectID;
                });

                TArray<int32> EmptyWaitClusters;
                for (auto& ClusterPair : OldGroup->WaitClusters)
                {
                    ClusterPair.Value.WaitPoints.RemoveAll([ObjectID](const FCrowdWaitPoint& WaitPoint)
                    {
                        return WaitPoint.OwnerObjectID == ObjectID;
                    });

                    if (ClusterPair.Value.WaitPoints.Num() == 0)
                    {
                        EmptyWaitClusters.Add(ClusterPair.Key);
                    }
                }

                for (int32 AnchorID : EmptyWaitClusters)
                {
                    OldGroup->WaitClusters.Remove(AnchorID);
                }
            }

            // Remove locked slot from old group
            LockedSlots.Remove(ObjectID);
        }
    }

    // Record agent intent
    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    FCrowdSurroundAgentIntent& Intent = AgentIntents.FindOrAdd(ObjectID);
    Intent.TargetObjectID = TargetObjectID;
    Intent.AttackRange = Request.Params.AttackRange;
    Intent.CollisionRadius = Request.Params.CollisionRadius;
    Intent.RequestTime = CurrentTime;

    // Ensure group exists
    if (!SurroundGroups.Contains(TargetObjectID))
    {
        FCrowdSurroundGroup NewGroup;
        NewGroup.TargetObjectID = TargetObjectID;
        NewGroup.TargetComponent = TargetComponent;
        SurroundGroups.Add(TargetObjectID, MoveTemp(NewGroup));
    }

    // Recompute assignments for this group
    RecomputeGroupAssignments(TargetObjectID);

    // Return cached assignment
    if (const FCrowdSurroundAssignment* Assignment = Assignments.Find(ObjectID))
    {
        if (Assignment->bHasAssignment)
        {
            OutAssignment = *Assignment;
            return true;
        }
    }

    return false;
}

bool UCrowdSurroundManager::LockSurroundAssignment(int32 ObjectID)
{
    const FCrowdSurroundAgentIntent* Intent = AgentIntents.Find(ObjectID);
    if (!Intent)
    {
        return false;
    }

    const int32 TargetObjectID = Intent->TargetObjectID;

    // Get group for position fallback
    const FCrowdSurroundGroup* Group = SurroundGroups.Find(TargetObjectID);

    // Get agent position from SparseGridManager
    const FVector AgentPosition = GetAgentPosition(ObjectID, Group ? *Group : FCrowdSurroundGroup());
    const float CollisionRadius = GetAgentCollisionRadius(ObjectID);

    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    // Record locked slot
    FLockedSurroundSlot& Slot = LockedSlots.FindOrAdd(ObjectID);
    Slot.ObjectID = ObjectID;
    Slot.LockedPosition = AgentPosition;
    Slot.CollisionRadius = CollisionRadius;
    Slot.LockTime = CurrentTime;
    Slot.LastProgressTime = CurrentTime;
    Slot.LastProgressDistance = TNumericLimits<float>::Max();

    // Recompute will happen in TickCrowdSurroundCorrection

    return true;
}

void UCrowdSurroundManager::UnlockSurroundAssignment(int32 ObjectID)
{
    const FCrowdSurroundAgentIntent* Intent = AgentIntents.Find(ObjectID);
    if (!Intent)
    {
        return;
    }

    const int32 TargetObjectID = Intent->TargetObjectID;

    // Remove locked slot
    LockedSlots.Remove(ObjectID);

    // Recompute will happen in TickCrowdSurroundCorrection
}

void UCrowdSurroundManager::RecomputeAllDirtyGroups()
{
    TSet<int32> DirtyTargetIDs;
    for (const auto& Pair : AgentIntents)
    {
        DirtyTargetIDs.Add(Pair.Value.TargetObjectID);
    }
    for (int32 TargetObjectID : DirtyTargetIDs)
    {
        RecomputeGroupAssignments(TargetObjectID);
    }
}

void UCrowdSurroundManager::Update(float DeltaTime)
{
    TArray<int32> GroupsToRemove;

    for (auto& Pair : SurroundGroups)
    {
        const FCrowdSurroundGroup& Group = Pair.Value;
        if (!Group.TargetComponent.IsValid())
        {
            GroupsToRemove.Add(Pair.Key);
        }
    }

    for (int32 TargetObjectID : GroupsToRemove)
    {
        // Collect all ObjectIDs that were in this group
        TArray<int32> AgentsInGroup = GetAgentsForGroup(TargetObjectID);

        // Remove all agent intents, locked slots, and assignments for agents in this group
        for (int32 ObjectID : AgentsInGroup)
        {
            AgentIntents.Remove(ObjectID);
            LockedSlots.Remove(ObjectID);
            Assignments.Remove(ObjectID);
        }

        SurroundGroups.Remove(TargetObjectID);
    }

    if (SparseGridDebug::IsCrowdDebugEnabled())
    {
        DrawDebugCrowd();
    }
}

bool UCrowdSurroundManager::GetSurroundAssignment(int32 ObjectID, FCrowdSurroundAssignment& OutAssignment) const
{
    const FCrowdSurroundAssignment* Assignment = Assignments.Find(ObjectID);
    if (!Assignment || !Assignment->bHasAssignment)
    {
        OutAssignment = FCrowdSurroundAssignment();
        return false;
    }

    OutAssignment = *Assignment;
    return true;
}

bool UCrowdSurroundManager::IsInSurroundGroup(int32 ObjectID) const
{
    return AgentIntents.Contains(ObjectID);
}
