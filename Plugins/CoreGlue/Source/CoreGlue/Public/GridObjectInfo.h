#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridObjectInfo.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGridObjectInfo : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API IGridObjectInfo
{
    GENERATED_BODY()

public:
    static constexpr int32 INVALID_GROUP_ID = -1;

    virtual AActor* GetOwnerActor() const { return nullptr; }
    virtual int32 GetTeamFlag() const { return 0; }
    virtual int32 GetFaction() const { return 0; }
    virtual FVector GetGridPosition() const { return FVector::ZeroVector; }
    virtual float GetCollisionRadius() const { return 0.0f; }
    virtual int32 GetGroupID() const { return INVALID_GROUP_ID; }
    virtual uint8 GetGroupRoleRaw() const { return 0; }
    virtual bool IsGridObjectActive() const { return true; }
    virtual bool IsInGroup() const { return GetGroupID() != INVALID_GROUP_ID; }

    virtual void SetGroupID(int32 InGroupID) {}
    virtual void SetGroupRoleRaw(uint8 InRole) {}
    virtual void SetTeamFlag(int32 InTeamFlag) {}
    virtual void OnGroupJoined(int32 InGroupID, uint8 InRole) {}
    virtual void OnGroupLeft(int32 OldGroupID) {}
};
