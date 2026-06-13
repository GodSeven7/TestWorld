// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/GamePawn.h"
#include "Components/SparseGridWithUnifiedDataComponent.h"
#include "Components/BattleDamageWithUnifiedDataComponent.h"
#include "Components/CombatWithUnifiedDataComponent.h"
#include "ActorUnifiedDataComponent.h"
#include "Core/UnifiedDataExtensionComponent.h"
#include "BattleDamageComponent.h"
#include "BattleDamageTypes.h"
#include "AuraManager.h"
#include "Coordinator/PluginCoordinator.h"

AGamePawn::AGamePawn()
{
    PrimaryActorTick.bCanEverTick = false;

    UnifiedDataComponent = CreateDefaultSubobject<UActorUnifiedDataComponent>(TEXT("UnifiedDataComponent"));
    ExtensionComponent = CreateDefaultSubobject<UUnifiedDataExtensionComponent>(TEXT("ExtensionComponent"));
    SparseGridComponent = CreateDefaultSubobject<USparseGridWithUnifiedDataComponent>(TEXT("SparseGridComponent"));
    BattleDamageComponent = CreateDefaultSubobject<UBattleDamageWithUnifiedDataComponent>(TEXT("BattleDamageComponent"));
    CombatComponent = CreateDefaultSubobject<UCombatWithUnifiedDataComponent>(TEXT("CombatComponent"));
}

void AGamePawn::BeginPlay()
{
	if (UnifiedDataComponent)
	{
		UnifiedDataComponent->BeforeInitializeUnifiedData();
	}

    Super::BeginPlay();

	if (UnifiedDataComponent)
	{
		UnifiedDataComponent->InitializeUnifiedData();
	}

    if (SparseGridComponent)
    {
        SparseGridComponent->RegisterToGrid();
    }

    if (BattleDamageComponent)
    {
        BattleDamageComponent->OnDamageReceivedDelegate.AddDynamic(
            this, &AGamePawn::HandleDamageReceived);
        BattleDamageComponent->OnDeathDelegate.AddDynamic(
            this, &AGamePawn::HandleDeath);
    }
}

void AGamePawn::HandleDeath(const FDamageInfo& KillingBlow)
{
    UE_LOG(LogTemp, Log, TEXT("GamePawn %s died"), *GetName());

    float ReturnDelay = 0.5f;

    FTimerHandle ReturnTimer;
    GetWorld()->GetTimerManager().SetTimer(
        ReturnTimer,
        this,
        &AGamePawn::ReturnToPool,
        ReturnDelay,
        false);
}

void AGamePawn::ReturnToPool()
{
    if (IPoolableActor* Poolable = Cast<IPoolableActor>(this))
    {
        Poolable->RecycleSelf();
    }
    else
    {
        Destroy();
    }
}

void AGamePawn::HandleDamageReceived(const FDamageInfo& Info, const FDamageResult& Result)
{
    UE_LOG(LogTemp, VeryVerbose, TEXT("GamePawn %s took %f damage, Health: %f/%f"),
        *GetName(),
        Result.FinalDamage,
        BattleDamageComponent->Attributes.CurrentHealth,
        BattleDamageComponent->Attributes.MaxHealth);
}

void AGamePawn::OnActorPoolSpawn()
{
    UE_LOG(LogTemp, Verbose, TEXT("GamePawn %s spawned from pool"), *GetName());
}

void AGamePawn::OnActorPoolReturn()
{
    UE_LOG(LogTemp, Verbose, TEXT("GamePawn %s returned to pool"), *GetName());

    OnReturnedToPoolDelegate.Broadcast(this);
}

void AGamePawn::ResetActorPoolState()
{
    bIsActive = true;
    bIsReturn = false;

    if (SparseGridComponent)
    {
        SparseGridComponent->SetSparseGridActive(true);
        SparseGridComponent->SetSparseGridCollisionLayer(0);
        SparseGridComponent->SetEnableSparseGridCollision(true);
        SparseGridComponent->RegisterToGrid();
    }

    if (BattleDamageComponent)
    {
        BattleDamageComponent->Revive(1.0f);
    }
}

void AGamePawn::SetActorPoolActive(bool bActive)
{
    bIsActive = bActive;
    if (SparseGridComponent)
    {
        SparseGridComponent->SetSparseGridActive(bActive);
    }
}
