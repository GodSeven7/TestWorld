#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/SpawnerConfig.h"
#include "SpawnerManager.generated.h"

class UCombatObjectFactory;
class UBattleDamageManager;
class AGamePawn;
struct FDamageInfo;

USTRUCT()
struct FActiveSpawnRule
{
    GENERATED_BODY()

    UPROPERTY()
    FSpawnRule Rule;

    UPROPERTY()
    TArray<TWeakObjectPtr<AGamePawn>> AliveActors;

    float TimeSinceLastSpawn = 0.f;
    float TimeSinceStart = 0.f;
    int32 NextSpawnPointIndex = 0;
    bool bStarted = false;
};

UCLASS()
class TESTWORLD_API USpawnerManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StartSpawning(USpawnerConfig* Config);

    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StopSpawning();

    void TickSpawning(float DeltaTime);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spawner")
    int32 GetAliveCount(int32 TypeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spawner")
    int32 GetTotalAliveCount() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spawner")
    bool IsSpawning() const { return bIsSpawning; }

private:
    UPROPERTY()
    UCombatObjectFactory* Factory = nullptr;

    UPROPERTY()
    UBattleDamageManager* BattleDamageManager = nullptr;

    UPROPERTY()
    TArray<FActiveSpawnRule> ActiveRules;

    bool bIsSpawning = false;

    UFUNCTION()
    void OnCombatantDeath(AActor* Combatant, const FDamageInfo& KillingBlow);

    void CleanupInvalidActors(FActiveSpawnRule& ActiveRule);
    FVector ChooseSpawnLocation(FActiveSpawnRule& ActiveRule);
    void SpawnOne(FActiveSpawnRule& ActiveRule);
    void ReturnAllSpawnedActors();
};
