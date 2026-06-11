#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "SparseGridHandle.h"
#include "SparseGridCollision.h"
#include "SparseGridComponent.h"
#include "WorldGridSubsystem.generated.h"

class USparseGridManager;
class AActor;
class ISparseGridObject;
class UGOAPManager;
class UBattleDamageManager;
class UCombatManager;
class UActorUnifiedDataComponent;
class UProjectileManager;
class UQuestManager;
class UGameDataSubsystem;
class USpawnerManager;
class UCharacterAnimManager;
class UPluginCoordinator;
struct FDamageInfo;

UCLASS()
class TESTWORLD_API UWorldGridSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	TArray<ISparseGridObject*> GetAllSparseGridObjects() const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid")
	void DestroyObject(const FSparseGridHandle& Handle);

	UFUNCTION(BlueprintCallable, Category = "WorldGrid")
	void UpdateObjectPosition(const FSparseGridHandle& Handle, const FVector& NewPosition);

	UFUNCTION(BlueprintCallable, Category = "WorldGrid")
	TArray<FSparseGridHandle> QuerySphere(const FVector& Center, float Radius) const;

	TArray<ISparseGridObject*> QuerySparseGridObjectsInSphere(const FVector& Center, float Radius) const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid")
	USparseGridManager* GetGridManager() const;

	bool IsInitialized() const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid")
	int32 GetTrackedActorCount() const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid")
	void ClearAll();

	UFUNCTION(BlueprintCallable, Category = "WorldGrid|Collision")
	void SetCollisionEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "WorldGrid|Collision")
	bool IsCollisionEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid|Collision")
	void SetCollisionMatrix(const FSparseGridCollisionMatrix& InMatrix);

	UFUNCTION(BlueprintCallable, Category = "WorldGrid|Collision")
	FSparseGridCollisionMatrix GetCollisionMatrix() const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid|Collision")
	FSparseGridCollisionStats GetCollisionStats() const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid|Collision")
	int32 GetActiveCollisionCount() const;

	AActor* FindNearestActor(const FVector& Position, float MaxRadius = 1000.0f) const;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	UProjectileManager* GetProjectileManager() const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid|Suspension")
	void SetGameplaySuspended(bool bSuspended);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WorldGrid|Suspension")
	bool IsGameplaySuspended() const;

	void RegisterUnifiedDataComponent(UActorUnifiedDataComponent* Component);
	void UnregisterUnifiedDataComponent(UActorUnifiedDataComponent* Component);
	const TArray<TObjectPtr<UActorUnifiedDataComponent>>& GetUnifiedDataComponents() const;

	UFUNCTION(BlueprintCallable, Category = "WorldGrid")
	UPluginCoordinator* GetCoordinator() const { return Coordinator; }

private:
	UPROPERTY()
	TObjectPtr<UPluginCoordinator> Coordinator;

	UPROPERTY()
	TArray<TObjectPtr<UActorUnifiedDataComponent>> UnifiedDataComponents;
};
