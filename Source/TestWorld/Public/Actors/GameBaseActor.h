// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActorPool.h"
#include "ActorUnifiedDataComponent.h"
#include "GameBaseActor.generated.h"

class USparseGridComponent;

UCLASS()
class TESTWORLD_API AGameBaseActor : public AActor, public IPoolableActor
{
    GENERATED_BODY()

public:
    AGameBaseActor();

    UActorUnifiedDataComponent* GetUnifiedDataComponent() const { return UnifiedDataComponent; }

protected:
    virtual void BeginPlay() override;

public:
    USparseGridComponent* GetSparseGridComponent() const { return SparseGridComponent; }

    //~ IPoolableActor interface
    virtual void OnActorPoolSpawn() override;
    virtual void OnActorPoolReturn() override;
    virtual void ResetActorPoolState() override;
    virtual void SetActorPoolActive(bool bActive) override;
    virtual bool IsActorPoolActive() const override { return bIsActive; }
    virtual void SetActorPoolPendingReturn(bool bReturn) override { bIsReturn = bReturn; }
    virtual bool IsActorPoolPendingReturn() const override { return bIsReturn; }
    virtual FOnActorReturnedToPool& GetOnReturnedToPoolDelegate() override { return OnReturnedToPoolDelegate; }
    //~ End IPoolableActor interface

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USparseGridComponent* SparseGridComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UActorUnifiedDataComponent* UnifiedDataComponent;

    UPROPERTY()
    bool bIsActive = true;

    UPROPERTY()
    bool bIsReturn = false;

private:
    FOnActorReturnedToPool OnReturnedToPoolDelegate;
};
