// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridObjectInfo.h"
#include "SparseGridHandle.h"
#include "SparseGridObject.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USparseGridObject : public UGridObjectInfo
{
    GENERATED_BODY()
};

class SPARSEGRIDPLUGIN_API ISparseGridObject : public IGridObjectInfo
{
    GENERATED_BODY()

public:
	virtual AActor* GetSparseGridOwnerActor() const { return nullptr; }

	virtual UObject* QueryUObject() const { return nullptr; }

	virtual FSparseGridHandle GetGridHandle() const { return FSparseGridHandle::Invalid; }

	virtual int32 GetObjectID() const { return INDEX_NONE; }

    bool IsSparseGridObjectActive() const { return bIsActive; }
    void SetSparseGridObjectActive(bool bActive) { bIsActive = bActive; }

    virtual FVector GetSparseGridPosition() const { return FVector::ZeroVector; }
    virtual FVector GetSparseGridForwardVector() const { return FVector::ForwardVector; }
    virtual FVector QueryMovementVelocity() const { return FVector::ZeroVector; }
    virtual float QueryMoveSpeed() const { return 0.0f; }
    virtual FVector QueryDesiredPosition() const { return FVector::ZeroVector; }
    virtual bool IsNavigationObstacle() const { return bIsNavObstacle; }
    void SetNavigationObstacle(bool bObstacle) { bIsNavObstacle = bObstacle; }
    virtual bool IsCrowdPushLocked() const;
    virtual UObject* GetSparseGridParent() const { return nullptr; }

    virtual int32 GetFaction() const { return 0; }

    float GetSparseGridRadius() const { return SparseGridRadius; }
    void SetSparseGridRadius(float Radius) { SparseGridRadius = Radius; }

    bool ShouldTrackInSparseGrid() const { return bShouldTrack; }
    void SetShouldTrackInSparseGrid(bool bTrack) { bShouldTrack = bTrack; }

    virtual void OnSparseGridRegistered() {}
    virtual void OnSparseGridUnregistered() {}

    void MarkForSparseGridDestroy() { bPendingSparseGridDestroy = true; }
    bool IsPendingSparseGridDestroy() const { return bPendingSparseGridDestroy; }
    void ClearSparseGridDestroyFlag() { bPendingSparseGridDestroy = false; }

    float GetSparseGridCollisionRadius() const { return CollisionRadius; }
    void SetSparseGridCollisionRadius(float Radius) { CollisionRadius = Radius; }

    int32 GetSparseGridCollisionLayer() const { return CollisionLayer; }
    void SetSparseGridCollisionLayer(int32 Layer) { CollisionLayer = Layer; }

    bool EnableSparseGridCollision() const { return bEnableCollision; }
    void SetEnableSparseGridCollision(bool bEnable) { bEnableCollision = bEnable; }

    virtual void OnSparseGridCollisionBegin(ISparseGridObject* Other, const FVector& ContactPoint) {}
    virtual void OnSparseGridCollisionEnd(ISparseGridObject* Other) {}

    bool EnableSparseGridRaycast() const { return bEnableRaycast; }
    void SetEnableSparseGridRaycast(bool bEnable) { bEnableRaycast = bEnable; }

    bool EnableSteering() const { return bEnableSteering; }
    void SetEnableSteering(bool bEnable) { bEnableSteering = bEnable; }

    int32 GetSparseGridRaycastLayer() const { return RaycastLayer; }
    void SetSparseGridRaycastLayer(int32 Layer) { RaycastLayer = Layer; }

    virtual int32 GetTeamFlag() const override { return TeamFlag; }
    virtual void SetTeamFlag(int32 InTeamFlag) override { TeamFlag = InTeamFlag; }

    virtual AActor* GetOwnerActor() const override { return Cast<AActor>(GetSparseGridParent()); }
    virtual FVector GetGridPosition() const override { return GetSparseGridPosition(); }
    virtual float GetCollisionRadius() const override { return GetSparseGridCollisionRadius(); }

    virtual int32 GetGroupID() const override { return GroupID; }
    virtual void SetGroupID(int32 InGroupID) override { GroupID = InGroupID; }

    virtual uint8 GetGroupRoleRaw() const override { return GroupRole; }
    virtual void SetGroupRoleRaw(uint8 InRole) override { GroupRole = InRole; }

    virtual bool IsGridObjectActive() const override { return bIsActive; }
    virtual bool IsInGroup() const override { return GroupID != INVALID_GROUP_ID; }

    virtual void OnGroupJoined(int32 InGroupID, uint8 InRole) override {}
    virtual void OnGroupLeft(int32 OldGroupID) override {}

protected:
    // 基础属性
    bool bIsActive = true;
    bool bShouldTrack = true;
    float SparseGridRadius = 10.0f;

    // 碰撞属性
    float CollisionRadius = 0.0f;
    int32 CollisionLayer = 0;
    bool bEnableCollision = true;

    // 射线检测属性
    bool bEnableRaycast = true;
    int32 RaycastLayer = 0;

    // 转向属性
    bool bEnableSteering = true;

    // 寻路障碍物属性
    bool bIsNavObstacle = false;

    // 分组属性
    int32 GroupID = INVALID_GROUP_ID;
    uint8 GroupRole = 0;
    int32 TeamFlag = 0;

private:
    bool bPendingSparseGridDestroy = false;
};
