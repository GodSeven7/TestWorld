// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystems/CombatObjectFactory.h"
#include "Data/CombatObjectDataTable.h"
#include "Actors/GamePawn.h"
#include "Actors/GameCharacter.h"
#include "ObjectPoolManager.h"
#include "ActorPool.h"
#include "SparseGridManager.h"
#include "Components/SparseGridWithUnifiedDataComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UCombatObjectFactory::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        PoolManager = GameInstance->GetSubsystem<UObjectPoolManager>();
    }

    CachedGridManager = GetWorld()->GetSubsystem<USparseGridManager>();

    LoadConfigFromINI();

    UE_LOG(LogTemp, Log, TEXT("CombatObjectFactory initialized"));
}

void UCombatObjectFactory::Deinitialize()
{
    LoadedConfig = nullptr;
    ConfigMap.Empty();
    PoolManager = nullptr;

    Super::Deinitialize();
}

bool UCombatObjectFactory::LoadConfig(TSoftObjectPtr<UCombatObjectDataTable> ConfigAsset)
{
    if (ConfigAsset.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: ConfigAsset is null"));
        return false;
    }

    UCombatObjectDataTable* LoadedData = ConfigAsset.LoadSynchronous();
    if (!LoadedData)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: Failed to load config asset"));
        return false;
    }

    LoadedConfig = LoadedData;
    BuildConfigMap();

    UE_LOG(LogTemp, Log, TEXT("CombatObjectFactory: Loaded config with %d combat object types"), ConfigMap.Num());
    return true;
}

void UCombatObjectFactory::BuildConfigMap()
{
    ConfigMap.Empty();

    if (!LoadedConfig)
        return;

    const TArray<FCombatObjectConfig>& Configs = LoadedConfig->GetAllConfigs();
    for (const FCombatObjectConfig& Config : Configs)
    {
        if (Config.IsValid())
        {
            ConfigMap.Add(Config.TypeID, Config);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: Skipping invalid config for TypeID %d"), Config.TypeID);
        }
    }
}

AGamePawn* UCombatObjectFactory::SpawnCombatObject(int32 TypeID, const FVector& Location, const FRotator& Rotation)
{
    FCombatObjectConfig Config;
    if (!GetConfig(TypeID, Config))
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: TypeID %d not found in config"), TypeID);
        return nullptr;
    }

    if (!Config.ActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: ActorClass is null for TypeID %d"), TypeID);
        return nullptr;
    }

    AGamePawn* SpawnedActor = nullptr;

    if (Config.bUseObjectPool && PoolManager)
    {
        UWorld* World = GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: World is null"));
            return nullptr;
        }

        FActorPoolConfig PoolConfig = Config.PoolConfig;
        TSharedPtr<TActorPool<AGamePawn>> Pool = PoolManager->GetOrCreateActorPool<AGamePawn>(Config.ActorClass, PoolConfig);

        if (Pool.IsValid())
        {
            SpawnedActor = Pool->Spawn(Location, Rotation);
        }
    }
    else
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        SpawnedActor = GetWorld()->SpawnActor<AGamePawn>(Config.ActorClass, Location, Rotation, SpawnParams);
    }

    if (SpawnedActor)
    {
        ApplyConfigToActor(SpawnedActor, Config);
        UE_LOG(LogTemp, Log, TEXT("CombatObjectFactory: Spawned combat object TypeID=%d, Name=%s"), 
            TypeID, *Config.TypeName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: Failed to spawn combat object TypeID=%d"), TypeID);
    }

    return SpawnedActor;
}

void UCombatObjectFactory::ReturnCombatObject(AGamePawn* CombatObject)
{
    if (!CombatObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: Cannot return null combat object"));
        return;
    }

    if (PoolManager)
    {
        PoolManager->ReturnActor(CombatObject);
        UE_LOG(LogTemp, Log, TEXT("CombatObjectFactory: Returned combat object to pool"));
    }
    else
    {
        CombatObject->Destroy();
        UE_LOG(LogTemp, Log, TEXT("CombatObjectFactory: Destroyed combat object (no pool manager)"));
    }
}

bool UCombatObjectFactory::GetConfig(int32 TypeID, FCombatObjectConfig& OutConfig) const
{
    const FCombatObjectConfig* ConfigPtr = ConfigMap.Find(TypeID);
    if (ConfigPtr)
    {
        OutConfig = *ConfigPtr;
        return true;
    }
    return false;
}

TArray<int32> UCombatObjectFactory::GetAllTypeIDs() const
{
    TArray<int32> TypeIDs;
    ConfigMap.GetKeys(TypeIDs);
    return TypeIDs;
}

void UCombatObjectFactory::PrewarmPool(int32 TypeID, int32 Count)
{
    FCombatObjectConfig Config;
    if (!GetConfig(TypeID, Config))
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: Cannot prewarm pool - TypeID %d not found"), TypeID);
        return;
    }

    if (!Config.bUseObjectPool)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: TypeID %d does not use object pool"), TypeID);
        return;
    }

    if (!PoolManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: PoolManager is null, cannot prewarm"));
        return;
    }

    PoolManager->PrewarmActorPool<AGamePawn>(Count);
    UE_LOG(LogTemp, Log, TEXT("CombatObjectFactory: Prewarmed pool for TypeID %d with %d actors"), TypeID, Count);
}

void UCombatObjectFactory::ApplyConfigToActor(AGamePawn* Actor, const FCombatObjectConfig& Config)
{
    if (!Actor)
        return;

    Actor->GetUnifiedDataComponent()->SetFaction(Config.DefaultFaction);

    if (USparseGridWithUnifiedDataComponent* GridComponent = Actor->GetSparseGridComponent())
    {
        FSparseGridHandle Handle = GridComponent->GetGridHandle();
        if (Handle.IsValid())
        {
            CachedGridManager->UpdateObjectFaction(Handle, 0, Config.DefaultFaction);
        }
    }

    if (AGameCharacter* Character = Cast<AGameCharacter>(Actor))
    {
    }

}

void UCombatObjectFactory::LoadConfigFromINI()
{
    FString ConfigPath = TEXT("/Script/TestWorld.TestWorldSettings");

    FString ConfigAssetPath;
    if (GConfig && GConfig->GetString(*ConfigPath, TEXT("CombatObjectConfigPath"), ConfigAssetPath, GGameIni))
    {
        UCombatObjectDataTable* LoadedData = LoadObject<UCombatObjectDataTable>(nullptr, *ConfigAssetPath);
        
        if (LoadedData)
        {
            LoadedConfig = LoadedData;
            BuildConfigMap();
            UE_LOG(LogTemp, Log, TEXT("CombatObjectFactory: Auto-loaded config from INI: %s"), *ConfigAssetPath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObjectFactory: Failed to auto-load config from INI: %s"), *ConfigAssetPath);
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("CombatObjectFactory: No INI config found for CombatObjectConfigPath"));
    }
}
