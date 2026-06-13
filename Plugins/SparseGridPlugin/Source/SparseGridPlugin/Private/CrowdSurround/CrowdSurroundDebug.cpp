#include "CrowdSurroundManager.h"
#include "CrowdSurroundUtilities.h"
#include "SparseGridComponent.h"
#include "DrawDebugHelpers.h"

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

        // Draw target radius circle
        CrowdSurroundUtilities::DrawCircle(World, Group.AnchorPosition, Group.TargetRadius, FColor::Green, LifeTime, 1.5f);

        // Count assignments and slots for this group
        int32 AssignmentCount = 0;
        int32 AttackCount = 0;
        int32 WaitCount = 0;
        for (const auto& IntentPair : AgentIntents)
        {
            if (IntentPair.Value.TargetObjectID != GroupPair.Key)
            {
                continue;
            }

            const FCrowdSurroundAssignment* Assignment = Assignments.Find(IntentPair.Key);
            if (Assignment && Assignment->bHasAssignment)
            {
                ++AssignmentCount;
            }
            if (AttackSlotList.Contains(IntentPair.Key))
            {
                ++AttackCount;
            }
            if (WaitSlotList.Contains(IntentPair.Key))
            {
                ++WaitCount;
            }
        }

        DrawDebugString(
            World,
            Group.AnchorPosition + FVector(0.0f, 0.0f, 95.0f),
            FString::Printf(TEXT("Assignments:%d AttackSlots:%d WaitSlots:%d"),
                AssignmentCount,
                AttackCount,
                WaitCount),
            nullptr,
            FColor::White,
            LifeTime,
            false,
            0.9f);

        // Draw attack slots (orange/red)
        for (const auto& SlotPair : AttackSlotList)
        {
            if (SlotPair.Value.TargetObjectID != GroupPair.Key)
            {
                continue;
            }

            const FLockedSurroundSlot& Slot = SlotPair.Value;
            FVector LockedPos = Slot.LockedPosition;
            LockedPos.Z += 24.0f;
            DrawDebugSphere(World, LockedPos, 12.0f, 8, FColor(255, 128, 0), false, LifeTime, SDPG_World, 2.0f);
            DrawDebugLine(World, Group.AnchorPosition + FVector(0.0f, 0.0f, 12.0f), LockedPos, FColor(255, 128, 0), false, LifeTime, SDPG_World, 0.75f);
            DrawDebugString(
                World,
                LockedPos + FVector(0.0f, 0.0f, 16.0f),
                FString::Printf(TEXT("ATTACK ID%d"), Slot.ObjectID),
                nullptr,
                FColor(255, 128, 0),
                LifeTime,
                false,
                0.8f);
        }

        // Draw wait slots (cyan/blue)
        for (const auto& SlotPair : WaitSlotList)
        {
            if (SlotPair.Value.TargetObjectID != GroupPair.Key)
            {
                continue;
            }

            const FLockedSurroundSlot& Slot = SlotPair.Value;
            FVector LockedPos = Slot.LockedPosition;
            LockedPos.Z += 20.0f;
            DrawDebugSphere(World, LockedPos, 10.0f, 8, FColor(0, 180, 220), false, LifeTime, SDPG_World, 1.5f);
            DrawDebugLine(World, Group.AnchorPosition + FVector(0.0f, 0.0f, 10.0f), LockedPos, FColor(0, 180, 220), false, LifeTime, SDPG_World, 0.6f);
            DrawDebugString(
                World,
                LockedPos + FVector(0.0f, 0.0f, 14.0f),
                FString::Printf(TEXT("WAIT ID%d"), Slot.ObjectID),
                nullptr,
                FColor(0, 180, 220),
                LifeTime,
                false,
                0.8f);
        }

        // Draw agent assignments (yellow = moving, blue = stopped)
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
            DesiredPosition.Z += 34.0f;

            FColor AssignmentColor = Assignment->bShouldMove ? FColor::Yellow : FColor(80, 160, 255);
            DrawDebugSphere(World, DesiredPosition, 13.0f, 8, AssignmentColor, false, LifeTime, SDPG_World, 2.0f);
            DrawDebugLine(World, AgentPosition, DesiredPosition, AssignmentColor, false, LifeTime, SDPG_World, 0.8f);

            FString Label = FString::Printf(TEXT("A ID%d"), ObjectID);
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

            DrawDebugString(World, DesiredPosition + FVector(0.0f, 0.0f, 18.0f), Label, nullptr, AssignmentColor, LifeTime, false, 0.9f);
        }
    }
}
