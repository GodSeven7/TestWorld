#include "CrowdSurroundManager.h"
#include "CrowdSurroundUtilities.h"
#include "SparseGridComponent.h"
#include "SparseGridManager.h"
#include "SparseGridConfig.h"

// ---------------------------------------------------------------------------
// GetAgentsForGroup: collect all ObjectIDs whose AgentIntent targets the given group
// ---------------------------------------------------------------------------
TArray<int32> UCrowdSurroundManager::GetAgentsForGroup(int32 TargetObjectID) const
{
    TArray<int32> Agents;
    for (const auto& Pair : AgentIntents)
    {
        if (Pair.Value.TargetObjectID == TargetObjectID)
        {
            Agents.Add(Pair.Key);
        }
    }
    return Agents;
}

// ---------------------------------------------------------------------------
// GetAgentPosition: resolve agent world position via SparseGridComponent
// ---------------------------------------------------------------------------
FVector UCrowdSurroundManager::GetAgentPosition(int32 ObjectID, const FCrowdSurroundGroup& Group) const
{
    if (USparseGridComponent* TargetComponent = Group.TargetComponent.Get())
    {
        if (UWorld* World = GetWorld())
        {
            if (USparseGridManager* GridManager = World->GetSubsystem<USparseGridManager>())
            {
                if (IGridObjectInfo* GridObject = GridManager->GetObjectByID(ObjectID))
                {
                    return CrowdSurroundUtilities::FlattenPosition(GridObject->GetGridPosition());
                }
            }
        }
    }

    return Group.AnchorPosition;
}

// ---------------------------------------------------------------------------
// GetAgentCollisionRadius: collision radius from intent or config fallback
// ---------------------------------------------------------------------------
float UCrowdSurroundManager::GetAgentCollisionRadius(int32 ObjectID) const
{
    const FCrowdSurroundAgentIntent* Intent = AgentIntents.Find(ObjectID);
    if (Intent && Intent->CollisionRadius > KINDA_SMALL_NUMBER)
    {
        return Intent->CollisionRadius;
    }
    return Config ? Config->BaseRadius : 50.0f;
}
