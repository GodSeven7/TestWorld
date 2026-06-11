// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ActorPool.h"
#include "Interfaces/GameObjectInfoInterface.h"
#include "ActorUnifiedDataComponent.h"
#include "Core/UnifiedDataExtensionComponent.h"
#include "GamePawn.generated.h"

class USparseGridWithUnifiedDataComponent;
class UBattleDamageWithUnifiedDataComponent;
class UCombatWithUnifiedDataComponent;

UCLASS()
class TESTWORLD_API AGamePawn : public APawn, public IPoolableActor, public IGameObjectInfo
{
	GENERATED_BODY()

public:
	AGamePawn();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void HandleDeath(const FDamageInfo& KillingBlow);
private:

	UFUNCTION()
	void HandleDamageReceived(const FDamageInfo& Info, const FDamageResult& Result);

	void ReturnToPool();

public:
	USparseGridWithUnifiedDataComponent* GetSparseGridComponent() const { return SparseGridComponent; }
	UBattleDamageWithUnifiedDataComponent* GetBattleDamageComponent() const { return BattleDamageComponent; }
	UCombatWithUnifiedDataComponent* GetCombatComponent() const { return CombatComponent; }
	UActorUnifiedDataComponent* GetUnifiedDataComponent() const { return UnifiedDataComponent; }
	UUnifiedDataExtensionComponent* GetExtensionComponent() const { return ExtensionComponent; }
	bool IsActive() const { return bIsActive; }

	virtual int32 GetFaction() const override { return Faction; }

	UFUNCTION(BlueprintCallable, Category = "Game Object")
	void SetFaction(int32 InFaction) { Faction = InFaction; }

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
	UActorUnifiedDataComponent* UnifiedDataComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UUnifiedDataExtensionComponent* ExtensionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USparseGridWithUnifiedDataComponent* SparseGridComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBattleDamageWithUnifiedDataComponent* BattleDamageComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCombatWithUnifiedDataComponent* CombatComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Object")
    int32 Faction = 0;

    UPROPERTY()
    bool bIsActive = true;

    UPROPERTY()
    bool bIsReturn = false;

private:
    FOnActorReturnedToPool OnReturnedToPoolDelegate;
};
