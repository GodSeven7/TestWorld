// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/CombatObjectConfig.h"
#include "CombatObjectFactory.generated.h"

class UCombatObjectDataTable;
class UObjectPoolManager;
class USparseGridManager;
class AGamePawn;

UCLASS()
class TESTWORLD_API UCombatObjectFactory : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Combat Object Factory")
    bool LoadConfig(TSoftObjectPtr<UCombatObjectDataTable> ConfigAsset);

    UFUNCTION(BlueprintCallable, Category = "Combat Object Factory")
    AGamePawn* SpawnCombatObject(int32 TypeID, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);

    UFUNCTION(BlueprintCallable, Category = "Combat Object Factory")
    void ReturnCombatObject(AGamePawn* CombatObject);

    bool GetConfig(int32 TypeID, FCombatObjectConfig& OutConfig) const;

    UFUNCTION(BlueprintCallable, Category = "Combat Object Factory")
    TArray<int32> GetAllTypeIDs() const;

    UFUNCTION(BlueprintCallable, Category = "Combat Object Factory")
    void PrewarmPool(int32 TypeID, int32 Count);

    UFUNCTION(BlueprintCallable, Category = "Combat Object Factory")
    bool IsConfigLoaded() const { return LoadedConfig != nullptr; }

    USparseGridManager* GetGridManager() const { return CachedGridManager; }

private:
    UPROPERTY()
    UCombatObjectDataTable* LoadedConfig = nullptr;

    UPROPERTY()
    UObjectPoolManager* PoolManager = nullptr;

    UPROPERTY()
    USparseGridManager* CachedGridManager = nullptr;

    TMap<int32, FCombatObjectConfig> ConfigMap;

    void ApplyConfigToActor(AGamePawn* Actor, const FCombatObjectConfig& Config);

    void BuildConfigMap();

    void LoadConfigFromINI();
};
