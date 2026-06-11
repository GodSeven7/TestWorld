#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EventBus.h"
#include "SparseGridHandle.h"
#include "SparseGridCollision.h"
#include "UnifiedDataRegistry.h"
#include "PluginCoordinator.generated.h"

class UGlueBase;
class UCombatGlue;
class UGOAPGlue;
class UProjectileGlue;
class UQuestGlue;
class UCharacterGlue;
class UGameUIGlue;
class USpawnerGlue;
class UCameraAdvanceGlue;
class USpatialGlue;
class UObjectPoolGlue;
class USparseGridManager;
class UFlowFieldManager;
class UGOAPManager;
class UBattleDamageManager;
class UCombatManager;
class UProjectileManager;
class UQuestManager;
class UGameDataSubsystem;
class USpawnerManager;
class UCharacterAnimManager;
class UCameraShakeManager;
class UObjectPoolManager;
class UCrowdSurroundManager;
class UActorUnifiedDataComponent;
class ISparseGridObject;

UCLASS()
class TESTWORLD_API UPluginCoordinator : public UTickableWorldSubsystem, public IUnifiedDataRegistry
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    UCombatGlue* GetCombatGlue() const { return CombatGlue; }
    UGOAPGlue* GetGOAPGlue() const { return GOAPGlue; }
    UProjectileGlue* GetProjectileGlue() const { return ProjectileGlue; }
    UQuestGlue* GetQuestGlue() const { return QuestGlue; }
    UCharacterGlue* GetCharacterGlue() const { return CharacterGlue; }
    UGameUIGlue* GetGameUIGlue() const { return GameUIGlue; }
    USpawnerGlue* GetSpawnerGlue() const { return SpawnerGlue; }
    UCameraAdvanceGlue* GetCameraAdvanceGlue() const { return CameraAdvanceGlue; }
    USpatialGlue* GetSpatialGlue() const { return SpatialGlue; }
    UObjectPoolGlue* GetObjectPoolGlue() const { return ObjectPoolGlue; }
    UEventBus* GetEventBus() const { return EventBus; }

    USparseGridManager* GetGridManager() const { return GridManager; }
    UFlowFieldManager* GetFlowFieldManager() const { return FlowFieldManager; }
    UBattleDamageManager* GetBattleDamageManager() const { return BattleDamageManager; }
    UCombatManager* GetCombatManager() const { return CombatManager; }
    UGOAPManager* GetGOAPManager() const { return GOAPManager; }
    UProjectileManager* GetProjectileManager() const { return ProjectileManager; }
    UQuestManager* GetQuestManager() const { return QuestManager; }
    UGameDataSubsystem* GetGameDataSubsystem() const { return GameDataSubsystem; }
    USpawnerManager* GetSpawnerManager() const { return SpawnerManager; }
    UCharacterAnimManager* GetCharacterAnimManager() const { return CharacterAnimManager; }
    UCameraShakeManager* GetCameraShakeManager() const { return CameraShakeManager; }
    UCrowdSurroundManager* GetCrowdSurroundManager() const { return CrowdSurroundManager; }

    void SetGameplaySuspended(bool bSuspended);
    bool IsGameplaySuspended() const { return bGameplaySuspended; }

    virtual void RegisterUnifiedDataComponent(UActorUnifiedDataComponent* Component) override;
    virtual void UnregisterUnifiedDataComponent(UActorUnifiedDataComponent* Component) override;
    virtual TArray<TObjectPtr<UActorUnifiedDataComponent>>& GetUnifiedDataComponents() override { return UnifiedDataComponents; }
    virtual void NotifyFactionChanged(UActorUnifiedDataComponent* Component, int32 OldFaction, int32 NewFaction) override;
    virtual UActorUnifiedDataComponent* GetObjectByID(int32 ObjectID) const override;

    int32 GetTrackedActorCount() const;
    TArray<ISparseGridObject*> GetAllSparseGridObjects() const;
    void DestroyObject(const FSparseGridHandle& Handle);
    void UpdateObjectPosition(const FSparseGridHandle& Handle, const FVector& NewPosition);
    TArray<FSparseGridHandle> QuerySphere(const FVector& Center, float Radius) const;
    TArray<ISparseGridObject*> QuerySparseGridObjectsInSphere(const FVector& Center, float Radius) const;
    AActor* FindNearestActor(const FVector& Position, float MaxRadius) const;
    void ClearAll();

    void SetCollisionEnabled(bool bEnabled);
    bool IsCollisionEnabled() const;
    void SetCollisionMatrix(const FSparseGridCollisionMatrix& InMatrix);
    FSparseGridCollisionMatrix GetCollisionMatrix() const;
    FSparseGridCollisionStats GetCollisionStats() const;
    int32 GetActiveCollisionCount() const;

    void GetDebugInfo(FString& OutInfo) const;

private:
    UPROPERTY()
    TObjectPtr<UCombatGlue> CombatGlue;

    UPROPERTY()
    TObjectPtr<UGOAPGlue> GOAPGlue;

    UPROPERTY()
    TObjectPtr<UProjectileGlue> ProjectileGlue;

    UPROPERTY()
    TObjectPtr<UQuestGlue> QuestGlue;

    UPROPERTY()
    TObjectPtr<UCharacterGlue> CharacterGlue;

    UPROPERTY()
    TObjectPtr<UGameUIGlue> GameUIGlue;

    UPROPERTY()
    TObjectPtr<USpawnerGlue> SpawnerGlue;

    UPROPERTY()
    TObjectPtr<UCameraAdvanceGlue> CameraAdvanceGlue;

    UPROPERTY()
    TObjectPtr<USpatialGlue> SpatialGlue;

    UPROPERTY()
    TObjectPtr<UObjectPoolGlue> ObjectPoolGlue;

    UPROPERTY()
    TObjectPtr<UEventBus> EventBus;

    UPROPERTY()
    TObjectPtr<USparseGridManager> GridManager;

    UPROPERTY()
    TObjectPtr<UFlowFieldManager> FlowFieldManager;

    UPROPERTY()
    TObjectPtr<UGOAPManager> GOAPManager;

    UPROPERTY()
    TObjectPtr<UBattleDamageManager> BattleDamageManager;

    UPROPERTY()
    TObjectPtr<UCombatManager> CombatManager;

    UPROPERTY()
    TObjectPtr<UProjectileManager> ProjectileManager;

    UPROPERTY()
    TObjectPtr<UQuestManager> QuestManager;

    UPROPERTY()
    TObjectPtr<UGameDataSubsystem> GameDataSubsystem;

    UPROPERTY()
    TObjectPtr<USpawnerManager> SpawnerManager;

    UPROPERTY()
    TObjectPtr<UCharacterAnimManager> CharacterAnimManager;

    UPROPERTY()
    TObjectPtr<UCameraShakeManager> CameraShakeManager;

    UPROPERTY()
    TObjectPtr<UCrowdSurroundManager> CrowdSurroundManager;

    UPROPERTY()
    TArray<TObjectPtr<UActorUnifiedDataComponent>> UnifiedDataComponents;

    // ObjectID → UnifiedDataComponent mapping for O(1) lookup
    TMap<int32, TObjectPtr<UActorUnifiedDataComponent>> ObjectIDToDataMap;

    bool bInitialized = false;
    bool bGameplaySuspended = false;
};
