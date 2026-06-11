#include "CrowdSurroundManager.h"
#include "CrowdSurroundUtilities.h"
#include "SparseGridComponent.h"
#include "DrawDebugHelpers.h"

namespace
{
    FColor GetAnchorColor(ECrowdAttackAnchorState State)
    {
        switch (State)
        {
        case ECrowdAttackAnchorState::Reserved:
            return FColor::Yellow;
        case ECrowdAttackAnchorState::Occupied:
            return FColor(255, 128, 0);
        case ECrowdAttackAnchorState::Blocked:
            return FColor::Red;
        case ECrowdAttackAnchorState::Cooldown:
            return FColor::Purple;
        case ECrowdAttackAnchorState::Free:
        default:
            return FColor::Green;
        }
    }

    const TCHAR* GetAnchorStateText(ECrowdAttackAnchorState State)
    {
        switch (State)
        {
        case ECrowdAttackAnchorState::Reserved:
            return TEXT("RES");
        case ECrowdAttackAnchorState::Occupied:
            return TEXT("OCC");
        case ECrowdAttackAnchorState::Blocked:
            return TEXT("BLK");
        case ECrowdAttackAnchorState::Cooldown:
            return TEXT("CD");
        case ECrowdAttackAnchorState::Free:
        default:
            return TEXT("FREE");
        }
    }

    const TCHAR* GetAssignmentStateText(ECrowdSurroundAssignmentState State)
    {
        switch (State)
        {
        case ECrowdSurroundAssignmentState::MovingToAttack:
            return TEXT("MTA");
        case ECrowdSurroundAssignmentState::Attacking:
            return TEXT("ATK");
        case ECrowdSurroundAssignmentState::MovingToWait:
            return TEXT("MTW");
        case ECrowdSurroundAssignmentState::Waiting:
            return TEXT("WAIT");
        case ECrowdSurroundAssignmentState::Reassigning:
            return TEXT("REA");
        case ECrowdSurroundAssignmentState::None:
        default:
            return TEXT("NONE");
        }
    }
}

void UCrowdSurroundManager::DrawDebugCrowd() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    constexpr float LifeTime = 0.02f;

    for (const auto& GroupPair : SurroundGroups)
    {
        const FCrowdSurroundGroup& Group = GroupPair.Value;
        if (!Group.TargetComponent.IsValid())
        {
            continue;
        }

        CrowdSurroundUtilities::DrawCircle(World, Group.AnchorPosition, Group.AttackBandMin, FColor::Green, LifeTime, 1.5f);
        CrowdSurroundUtilities::DrawCircle(World, Group.AnchorPosition, Group.AttackBandMax, FColor(255, 128, 0), LifeTime, 1.5f);

        // Count attack/wait assignments for agents targeting this group
        int32 AttackCount = 0;
        int32 WaitCount = 0;
        for (const auto& IntentPair : AgentIntents)
        {
            if (IntentPair.Value.TargetObjectID != GroupPair.Key)
            {
                continue;
            }

            const FCrowdSurroundAssignment* Assignment = Assignments.Find(IntentPair.Key);
            if (!Assignment || !Assignment->bHasAssignment)
            {
                continue;
            }

            if (Assignment->Type == ECrowdSurroundAssignmentType::AttackAnchor)
            {
                ++AttackCount;
            }
            else if (Assignment->Type == ECrowdSurroundAssignmentType::WaitPoint)
            {
                ++WaitCount;
            }
        }

        DrawDebugString(
            World,
            Group.AnchorPosition + FVector(0.0f, 0.0f, 95.0f),
            FString::Printf(TEXT("Crowd Anchors:%d/%d Wait:%d R:%.0f-%.0f"),
                AttackCount,
                Group.AttackCapacity,
                WaitCount,
                Group.AttackBandMin,
                Group.AttackBandMax),
            nullptr,
            FColor::White,
            LifeTime,
            false,
            0.9f);

        for (const FCrowdAttackAnchor& Anchor : Group.AttackAnchors)
        {
            FVector AnchorPosition = Anchor.Position;
            AnchorPosition.Z += 24.0f;

            const FColor AnchorColor = GetAnchorColor(Anchor.State);
            DrawDebugSphere(World, AnchorPosition, 12.0f, 8, AnchorColor, false, LifeTime, SDPG_World, 2.0f);
            DrawDebugLine(World, Group.AnchorPosition + FVector(0.0f, 0.0f, 12.0f), AnchorPosition, AnchorColor, false, LifeTime, SDPG_World, 0.75f);
            DrawDebugString(
                World,
                AnchorPosition + FVector(0.0f, 0.0f, 16.0f),
                FString::Printf(TEXT("H%d %s O%d"), Anchor.AnchorID, GetAnchorStateText(Anchor.State), Anchor.OwnerObjectID),
                nullptr,
                AnchorColor,
                LifeTime,
                false,
                0.8f);

            if (const FCrowdWaitCluster* Cluster = Group.WaitClusters.Find(Anchor.AnchorID))
            {
                for (const FCrowdWaitPoint& WaitPoint : Cluster->WaitPoints)
                {
                    FVector WaitPosition = WaitPoint.Position;
                    WaitPosition.Z += 18.0f;

                    FVector ParentPosition = AnchorPosition;
                    if (WaitPoint.ParentObjectID != INDEX_NONE)
                    {
                        // Look up the parent agent's position from AgentIntents + Assignments
                        const FCrowdSurroundAgentIntent* ParentIntent = AgentIntents.Find(WaitPoint.ParentObjectID);
                        if (ParentIntent && ParentIntent->TargetObjectID == GroupPair.Key)
                        {
                            ParentPosition = GetAgentPosition(WaitPoint.ParentObjectID, Group);
                            ParentPosition.Z += 18.0f;
                        }
                    }

                    DrawDebugSphere(World, WaitPosition, 8.0f, 8, FColor::Cyan, false, LifeTime, SDPG_World, 1.5f);
                    DrawDebugLine(World, ParentPosition, WaitPosition, FColor::Cyan, false, LifeTime, SDPG_World, 0.6f);
                    DrawDebugString(
                        World,
                        WaitPosition + FVector(0.0f, 0.0f, 12.0f),
                        FString::Printf(TEXT("W%d L%d B%d A%d P%d"),
                            WaitPoint.WaitIndex,
                            WaitPoint.WaitLayer,
                            WaitPoint.WaitBranchIndex,
                            WaitPoint.OwnerObjectID,
                            WaitPoint.ParentObjectID),
                        nullptr,
                        FColor::Cyan,
                        LifeTime,
                        false,
                        0.75f);
                }
            }
        }

        // Draw agent assignments for agents targeting this group
        for (const auto& IntentPair : AgentIntents)
        {
            if (IntentPair.Value.TargetObjectID != GroupPair.Key)
            {
                continue;
            }

            const int32 ObjectID = IntentPair.Key;
            const FCrowdSurroundAssignment* Assignment = Assignments.Find(ObjectID);
            if (!Assignment || !Assignment->bHasAssignment)
            {
                continue;
            }

            FVector AgentPosition = GetAgentPosition(ObjectID, Group);
            AgentPosition.Z += 18.0f;

            FVector DesiredPosition = Assignment->DesiredPosition;
            DesiredPosition.Z += Assignment->Type == ECrowdSurroundAssignmentType::AttackAnchor ? 34.0f : 24.0f;

            const FColor AssignmentColor = CrowdSurroundUtilities::GetAssignmentColor(Assignment->State, Assignment->Type);
            const float MarkerRadius = Assignment->Type == ECrowdSurroundAssignmentType::AttackAnchor ? 13.0f : 9.0f;
            DrawDebugSphere(World, DesiredPosition, MarkerRadius, 8, AssignmentColor, false, LifeTime, SDPG_World, 2.0f);
            DrawDebugLine(World, AgentPosition, DesiredPosition, AssignmentColor, false, LifeTime, SDPG_World, 0.8f);

            FString Label = FString::Printf(
                TEXT("%s ID%d H%d W%d L%d B%d %s"),
                Assignment->Type == ECrowdSurroundAssignmentType::AttackAnchor ? TEXT("A") : TEXT("Q"),
                ObjectID,
                Assignment->AnchorID,
                Assignment->WaitIndex,
                Assignment->WaitLayer,
                Assignment->WaitBranchIndex,
                GetAssignmentStateText(Assignment->State));

            if (Assignment->bHasAttackPermission)
            {
                Label += TEXT(" OK");
            }
            if (Assignment->bIsInPosition)
            {
                Label += TEXT(" I");
            }
            if (Assignment->bShouldMove)
            {
                Label += TEXT(" M");
            }
            if (Assignment->bLocksCrowdPosition)
            {
                Label += TEXT(" L");
            }
            if (Assignment->bIsRefillCandidate)
            {
                Label += TEXT(" R");
            }

            DrawDebugString(World, DesiredPosition + FVector(0.0f, 0.0f, 18.0f), Label, nullptr, AssignmentColor, LifeTime, false, 0.9f);
        }
    }
}
