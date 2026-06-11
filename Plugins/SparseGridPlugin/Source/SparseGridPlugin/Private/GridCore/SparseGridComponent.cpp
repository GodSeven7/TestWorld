// Copyright Epic Games, Inc. All Rights Reserved.

#include "SparseGridComponent.h"
#include "ActorUnifiedDataComponent.h"
#include "CrowdSurroundManager.h"
#include "SparseGridManager.h"
#include "SparseGridConfig.h"

USparseGridComponent::USparseGridComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USparseGridComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!AssociatedComponent)
    {
        AActor* Owner = GetOwner();
        if (Owner)
        {
            AssociatedComponent = Owner->GetRootComponent();
        }
    }
    // 注册延迟到 Actor 的 BeginPlay 中通过 RegisterToGrid() 调用
    // 确保所有组件（包括设置 Faction 的组件）已就绪
}

void USparseGridComponent::RegisterToGrid()
{
    if (GridHandle.IsValid())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    USparseGridManager* Manager = World->GetSubsystem<USparseGridManager>();
    if (Manager)
    {
        if (GetSparseGridCollisionRadius() <= 0.0f)
        {
            if (USparseGridConfig* Config = Manager->GetSparseGridConfig())
            {
                SetSparseGridCollisionRadius(Config->BaseRadius);
            }
        }
        GridHandle = Manager->CreateObject(this);
        OnSparseGridRegistered();
    }
}

void USparseGridComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GridHandle.IsValid())
    {
        UWorld* World = GetWorld();
        if (World)
        {
            USparseGridManager* Manager = World->GetSubsystem<USparseGridManager>();
            if (Manager)
            {
                Manager->DestroyObject(GridHandle);
                OnSparseGridUnregistered();
            }
        }
        GridHandle = FSparseGridHandle::Invalid;
    }

    Super::EndPlay(EndPlayReason);
}

FVector USparseGridComponent::GetSparseGridPosition() const
{
    if (AssociatedComponent)
    {
        return AssociatedComponent->GetComponentLocation();
    }
    
    AActor* Owner = GetOwner();
    return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

FVector USparseGridComponent::GetSparseGridForwardVector() const
{
    if (AssociatedComponent)
    {
        return AssociatedComponent->GetForwardVector();
    }

    AActor* Owner = GetOwner();
    return Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
}

UObject* USparseGridComponent::GetSparseGridParent() const
{
    return GetOwner();
}

bool USparseGridComponent::IsCrowdPushLocked() const
{
	UWorld* World = GetWorld();
	UCrowdSurroundManager* CrowdManager = World ? World->GetSubsystem<UCrowdSurroundManager>() : nullptr;
	if (!CrowdManager)
	{
		return false;
	}

	FCrowdSurroundAssignment Assignment;
	const int32 ObjectID = GetObjectID();
	return ObjectID != INDEX_NONE
		&& CrowdManager->GetSurroundAssignment(ObjectID, Assignment)
		&& Assignment.bLocksCrowdPosition;
}

int32 USparseGridComponent::GetObjectID() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return INDEX_NONE;
	}

	if (UActorUnifiedDataComponent* UnifiedData = Owner->FindComponentByClass<UActorUnifiedDataComponent>())
	{
		return UnifiedData->GetObjectID();
	}

	return INDEX_NONE;
}
