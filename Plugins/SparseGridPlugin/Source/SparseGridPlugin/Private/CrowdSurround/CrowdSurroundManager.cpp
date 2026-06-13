#include "CrowdSurroundManager.h"
#include "CrowdSurroundUtilities.h"
#include "SparseGridManager.h"
#include "SparseGridComponent.h"
#include "SparseGridConfig.h"
#include "SparseGridDebug.h"
#include "SparseGridQueryUtils.h"
#include "GameFramework/Actor.h"

void UCrowdSurroundManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UCrowdSurroundManager::Deinitialize()
{
    AgentIntents.Empty();
    AttackSlotList.Empty();
    WaitSlotList.Empty();
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

// =========================================================================
// Request: only record intent, do NOT solve or produce assignment
// =========================================================================

bool UCrowdSurroundManager::RequestSurroundAssignment(const FCrowdSurroundRequest& Request)
{
    FScopeLock Lock(&Mutex);

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

    // If agent already has intent targeting a different target, clean up old data
    if (FCrowdSurroundAgentIntent* ExistingIntent = AgentIntents.Find(ObjectID))
    {
        if (ExistingIntent->TargetObjectID != TargetObjectID)
        {
            AttackSlotList.Remove(ObjectID);
            WaitSlotList.Remove(ObjectID);
            Assignments.Remove(ObjectID);
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

    return true;
}

// =========================================================================
// Recompute: per-Tick assignment update
// =========================================================================

void UCrowdSurroundManager::RecomputeAssignments()
{
    for (auto& GroupPair : SurroundGroups)
    {
        const int32 TargetObjectID = GroupPair.Key;
        FCrowdSurroundGroup& Group = GroupPair.Value;

        if (!Group.TargetComponent.IsValid())
        {
            continue;
        }

        Group.AnchorPosition = CrowdSurroundUtilities::FlattenPosition(Group.TargetComponent->GetSparseGridPosition());
        Group.TargetRadius = Group.TargetComponent->GetSparseGridCollisionRadius();

        for (const auto& IntentPair : AgentIntents)
        {
            if (IntentPair.Value.TargetObjectID != TargetObjectID)
            {
                continue;
            }

            const int32 ObjectID = IntentPair.Key;
            const FCrowdSurroundAgentIntent& Intent = IntentPair.Value;

            FCrowdSurroundAssignment& Assignment = Assignments.FindOrAdd(ObjectID);

            if (const FLockedSurroundSlot* AttackSlot = AttackSlotList.Find(ObjectID))
            {
                Assignment.DesiredPosition = AttackSlot->LockedPosition;
                Assignment.bHasAssignment = true;
                Assignment.bShouldMove = false;
                Assignment.bIsInPosition = true;
                Assignment.bLocksCrowdPosition = true;
                continue;
            }
			else if (const FLockedSurroundSlot* WaitSlot = WaitSlotList.Find(ObjectID))
            {
                Assignment.DesiredPosition = WaitSlot->LockedPosition;
                Assignment.bHasAssignment = true;
                Assignment.bShouldMove = false;
                Assignment.bIsInPosition = true;
                Assignment.bLocksCrowdPosition = true;
                continue;
            }

            // Unlocked intent: solve arc clipping
            const FVector AgentPosition = GetAgentPosition(ObjectID, Group);
            FVector DirectionFromTarget = (AgentPosition - Group.AnchorPosition).GetSafeNormal2D();
            if (DirectionFromTarget.IsNearlyZero())
            {
                // Cannot determine a direction; keep previous assignment if any
                continue;
            }

            TArray<FSparseGridArcObstacle> Obstacles;
            BuildArcObstaclesForGroup(TargetObjectID, ObjectID, Obstacles);

            const float AgentRadius = Intent.AttackRange;
            FVector DesiredPosition;
            if (FSparseGridQueryUtils::SolveArcClippingPosition(
                Group.AnchorPosition,
                Group.TargetRadius,
                AgentRadius,
                DirectionFromTarget,
                Obstacles,
                DesiredPosition))
            {
                DesiredPosition.Z = Group.AnchorPosition.Z;

                Assignment.DesiredPosition = DesiredPosition;
                Assignment.bHasAssignment = true;
                Assignment.bShouldMove = true;
                Assignment.bIsInPosition = false;
                Assignment.bLocksCrowdPosition = false;
            }
            // Solver failed: keep previous assignment state or leave bHasAssignment=false
        }
    }
}

// =========================================================================
// Lock / Unlock
// =========================================================================

bool UCrowdSurroundManager::LockSlot(int32 ObjectID, ECrowdSurroundSlotType SlotType)
{
    // Determine TargetObjectID: prefer AgentIntents, but don't fail if missing
    int32 TargetObjectID = INDEX_NONE;
    if (const FCrowdSurroundAgentIntent* Intent = AgentIntents.Find(ObjectID))
    {
        TargetObjectID = Intent->TargetObjectID;
    }

    // Lock position: prefer Assignment.DesiredPosition, fallback to agent location
    const FCrowdSurroundAssignment* Assignment = Assignments.Find(ObjectID);
    FVector LockedPosition;
    if (Assignment && Assignment->bHasAssignment)
    {
        LockedPosition = Assignment->DesiredPosition;
    }
    else
    {
        // Fallback: query agent world position via GridManager
        LockedPosition = FVector::ZeroVector;
        if (UWorld* World = GetWorld())
        {
            if (USparseGridManager* GridManager = World->GetSubsystem<USparseGridManager>())
            {
                if (IGridObjectInfo* GridObject = GridManager->GetObjectByID(ObjectID))
                {
                    LockedPosition = CrowdSurroundUtilities::FlattenPosition(GridObject->GetGridPosition());
                }
            }
        }
    }

    const float CollisionRadius = GetAgentCollisionRadius(ObjectID);
    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    if (SlotType == ECrowdSurroundSlotType::Attack)
    {
        // Remove from wait slot first to prevent dual membership
        WaitSlotList.Remove(ObjectID);

        FLockedSurroundSlot& Slot = AttackSlotList.FindOrAdd(ObjectID);
        Slot.ObjectID = ObjectID;
        Slot.TargetObjectID = TargetObjectID;
        Slot.Type = ECrowdSurroundSlotType::Attack;
        Slot.LockedPosition = LockedPosition;
        Slot.CollisionRadius = CollisionRadius;
        Slot.LockTime = CurrentTime;
    }
    else
    {
        // Don't add if already in attack slot
        if (AttackSlotList.Contains(ObjectID))
        {
            return false;
        }

        FLockedSurroundSlot& Slot = WaitSlotList.FindOrAdd(ObjectID);
        Slot.ObjectID = ObjectID;
        Slot.TargetObjectID = TargetObjectID;
        Slot.Type = ECrowdSurroundSlotType::Wait;
        Slot.LockedPosition = LockedPosition;
        Slot.CollisionRadius = CollisionRadius;
        Slot.LockTime = CurrentTime;
    }

    // Update assignment to reflect locked state
    if (FCrowdSurroundAssignment* AssignPtr = Assignments.Find(ObjectID))
    {
        AssignPtr->DesiredPosition = LockedPosition;
        AssignPtr->bShouldMove = false;
        AssignPtr->bIsInPosition = true;
        AssignPtr->bLocksCrowdPosition = true;
    }

    return true;
}

void UCrowdSurroundManager::UnlockSlot(int32 ObjectID)
{
    AttackSlotList.Remove(ObjectID);
    WaitSlotList.Remove(ObjectID);
}

bool UCrowdSurroundManager::LockAttackSlot(int32 ObjectID)
{
    FScopeLock Lock(&Mutex);
    return LockSlot(ObjectID, ECrowdSurroundSlotType::Attack);
}

void UCrowdSurroundManager::UnlockAttackSlot(int32 ObjectID)
{
    FScopeLock Lock(&Mutex);
    UnlockSlot(ObjectID);
}

bool UCrowdSurroundManager::LockWaitSlot(int32 ObjectID)
{
    FScopeLock Lock(&Mutex);
    return LockSlot(ObjectID, ECrowdSurroundSlotType::Wait);
}

void UCrowdSurroundManager::UnlockWaitSlot(int32 ObjectID)
{
    FScopeLock Lock(&Mutex);
    UnlockSlot(ObjectID);
}

void UCrowdSurroundManager::ClearSurroundState(int32 ObjectID)
{
    FScopeLock Lock(&Mutex);

    AgentIntents.Remove(ObjectID);
    AttackSlotList.Remove(ObjectID);
    WaitSlotList.Remove(ObjectID);
    Assignments.Remove(ObjectID);
}

// =========================================================================
// Obstacles
// =========================================================================

void UCrowdSurroundManager::BuildArcObstaclesForGroup(
    int32 TargetObjectID,
    int32 ExcludedObjectID,
    TArray<FSparseGridArcObstacle>& OutObstacles) const
{
    OutObstacles.Reset();

    auto AddSlot = [&](const FLockedSurroundSlot& Slot)
    {
        if (Slot.ObjectID == ExcludedObjectID)
        {
            return;
        }
        if (Slot.TargetObjectID != TargetObjectID && Slot.TargetObjectID != INDEX_NONE)
        {
            return;
        }
        FSparseGridArcObstacle Obstacle;
        Obstacle.Position = CrowdSurroundUtilities::FlattenPosition(Slot.LockedPosition);
        Obstacle.Radius = FMath::Max(1.0f, Slot.CollisionRadius);
        OutObstacles.Add(Obstacle);
    };

    for (const auto& Pair : AttackSlotList)  AddSlot(Pair.Value);
    for (const auto& Pair : WaitSlotList)    AddSlot(Pair.Value);
}

// =========================================================================
// Update / Cleanup
// =========================================================================

void UCrowdSurroundManager::Update(float DeltaTime)
{
    FScopeLock Lock(&Mutex);

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
        TArray<int32> AgentsInGroup = GetAgentsForGroup(TargetObjectID);

        for (int32 ObjectID : AgentsInGroup)
        {
            AgentIntents.Remove(ObjectID);
            AttackSlotList.Remove(ObjectID);
            WaitSlotList.Remove(ObjectID);
            Assignments.Remove(ObjectID);
        }

        SurroundGroups.Remove(TargetObjectID);
    }

    RecomputeAssignments();

    if (SparseGridDebug::IsCrowdDebugEnabled())
    {
        DrawDebugCrowd();
    }
}

// =========================================================================
// Queries
// =========================================================================

bool UCrowdSurroundManager::GetSurroundAssignment(int32 ObjectID, FCrowdSurroundAssignment& OutAssignment) const
{
    FScopeLock Lock(&Mutex);

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
    FScopeLock Lock(&Mutex);
    return AgentIntents.Contains(ObjectID);
}

bool UCrowdSurroundManager::GetFixedSlotPosition(int32 ObjectID, FVector& OutPosition) const
{
    FScopeLock Lock(&Mutex);
    if (const FLockedSurroundSlot* Slot = AttackSlotList.Find(ObjectID))
    {
        OutPosition = Slot->LockedPosition;
        return true;
    }
    if (const FLockedSurroundSlot* Slot = WaitSlotList.Find(ObjectID))
    {
        OutPosition = Slot->LockedPosition;
        return true;
    }
    return false;
}
