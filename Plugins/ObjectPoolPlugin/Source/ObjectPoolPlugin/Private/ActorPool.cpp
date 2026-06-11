// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActorPool.h"
#include "ObjectPoolManager.h"

bool IPoolableActor::RecycleSelf()
{
    AActor* Actor = Cast<AActor>(this);
    if (!Actor || Actor->IsPendingKillPending())
    {
        return false;
    }

    if (!CanReturnToActorPool())
    {
        return false;
    }

    if (IsActorPoolPendingReturn())
    {
        return false;
    }

    UWorld* World = Actor->GetWorld();
    if (!World)
    {
        return false;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return false;
    }

    UObjectPoolManager* PoolManager = GameInstance->GetSubsystem<UObjectPoolManager>();
    if (!PoolManager)
    {
        return false;
    }

    PoolManager->ReturnActor(Actor);
    return true;
}
