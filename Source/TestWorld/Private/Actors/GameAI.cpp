// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/GameAI.h"
#include "UObject/ConstructorHelpers.h"
#include "CharacterSkelMeshComponent.h"
#include "Components/GOAPAgentWithUnifiedDataComponent.h"

AGameAI::AGameAI()
{
    Velocity = FVector::ZeroVector;

    GOAPComponent = CreateDefaultSubobject<UGOAPAgentWithUnifiedDataComponent>(TEXT("GOAPComponent"));
}

void AGameAI::BeginPlay()
{
    Super::BeginPlay();

    if (!AIConfig)
    {
        AIConfig = GetClass()->GetDefaultObject<AGameAI>()->AIConfig;
        if (!AIConfig)
        {
            AIConfig = LoadObject<UAIConfig>(nullptr, TEXT("/Game/Config/DA_AIConfig_Default"));
        }
    }

    BoundsMin = GetConfigBoundsMin();
    BoundsMax = GetConfigBoundsMax();

    if (MeshRootComponent)
    {
        GOAPComponent->SetAssociatedComponent(MeshRootComponent);
    }
}

void AGameAI::OnActorPoolSpawn()
{
    Super::OnActorPoolSpawn();

    GOAPComponent->PatrolPoints.Empty();
	GOAPComponent->SetSpawnedPosition(GOAPComponent->QueryWorldPosition());

	float Radius = GetConfigPatrolRadius();
	FVector CurrentLoc = MeshComponent ? MeshComponent->GetComponentLocation() : GetActorLocation();

	GOAPComponent->PatrolPoints.Add(CurrentLoc + FVector(Radius, 0.0f, 0.0f));
	GOAPComponent->PatrolPoints.Add(CurrentLoc + FVector(0.0f, Radius, 0.0f));
	GOAPComponent->PatrolPoints.Add(CurrentLoc + FVector(-Radius, 0.0f, 0.0f));
	GOAPComponent->PatrolPoints.Add(CurrentLoc + FVector(0.0f, -Radius, 0.0f));

	GOAPComponent->SetPatrolGoal();
}

void AGameAI::OnActorPoolReturn()
{
    Super::OnActorPoolReturn();
}

void AGameAI::ResetActorPoolState()
{
    Super::ResetActorPoolState();

    GOAPComponent->PatrolPoints.Empty();
}

void AGameAI::SetMovementBounds(const FVector& Min, const FVector& Max)
{
    BoundsMin = Min;
    BoundsMax = Max;
}
