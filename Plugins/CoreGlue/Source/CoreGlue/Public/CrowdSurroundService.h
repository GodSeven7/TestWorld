#pragma once

#include "CoreMinimal.h"
#include "CrowdSurroundTypes.h"
#include "UObject/Interface.h"
#include "CrowdSurroundService.generated.h"

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class COREGLUE_API UCrowdSurroundService : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API ICrowdSurroundService
{
    GENERATED_BODY()

public:
    virtual bool RequestSurroundAssignment(const FCrowdSurroundRequest& Request) = 0;

    // Attack slot: locked by AttackActionExecutor::OnEnter, unlocked on exit.
    // Locking records agent position as an occupied attack slot (blocks arc solver).
    virtual bool LockAttackSlot(int32 ObjectID) = 0;
    virtual void UnlockAttackSlot(int32 ObjectID) = 0;

    // Wait slot: locked when agent is waiting for an attack opportunity.
    // Locking records agent position as an occupied wait slot (blocks arc solver).
    virtual bool LockWaitSlot(int32 ObjectID) = 0;
    virtual void UnlockWaitSlot(int32 ObjectID) = 0;

    // Clear all surround state for an agent (intent, assignments, both slot lists).
    virtual void ClearSurroundState(int32 ObjectID) = 0;

    virtual void Update(float DeltaTime) = 0;

    virtual bool GetSurroundAssignment(int32 ObjectID, FCrowdSurroundAssignment& OutAssignment) const = 0;
    virtual bool IsInSurroundGroup(int32 ObjectID) const = 0;

    // Return locked position if agent is in AttackSlotList or WaitSlotList.
    virtual bool GetFixedSlotPosition(int32 ObjectID, FVector& OutPosition) const = 0;
};
