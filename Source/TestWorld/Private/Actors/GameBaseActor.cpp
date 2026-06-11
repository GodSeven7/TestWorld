// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/GameBaseActor.h"
#include "SparseGridComponent.h"

AGameBaseActor::AGameBaseActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SparseGridComponent = CreateDefaultSubobject<USparseGridComponent>(TEXT("SparseGridComponent"));
    UnifiedDataComponent = CreateDefaultSubobject<UActorUnifiedDataComponent>(TEXT("UnifiedDataComponent"));
}

void AGameBaseActor::BeginPlay()
{
    Super::BeginPlay();

    // 所有组件 BeginPlay 已完成，按顺序初始化
    if (UnifiedDataComponent)
    {
        UnifiedDataComponent->InitializeUnifiedData();
    }

    if (SparseGridComponent)
    {
        SparseGridComponent->RegisterToGrid();
    }
}

void AGameBaseActor::OnActorPoolSpawn()
{
    UE_LOG(LogTemp, Verbose, TEXT("GameBaseActor %s spawned from pool"), *GetName());
}

void AGameBaseActor::OnActorPoolReturn()
{
    UE_LOG(LogTemp, Verbose, TEXT("GameBaseActor %s returned to pool"), *GetName());
    
    OnReturnedToPoolDelegate.Broadcast(this);
}

void AGameBaseActor::ResetActorPoolState()
{
    bIsActive = true;

    if (SparseGridComponent)
    {
        SparseGridComponent->SetSparseGridActive(true);
        SparseGridComponent->SetSparseGridCollisionLayer(0);
        SparseGridComponent->SetEnableSparseGridCollision(true);
        SparseGridComponent->RegisterToGrid();
    }
}

void AGameBaseActor::SetActorPoolActive(bool bActive)
{
    bIsActive = bActive;
    if (SparseGridComponent)
    {
        SparseGridComponent->SetSparseGridActive(bActive);
    }
}
