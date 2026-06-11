// Copyright Epic Games, Inc. All Rights Reserved.

#include "Test/CombatObjectTestCommands.h"
#include "Subsystems/CombatObjectFactory.h"
#include "Actors/GamePawn.h"
#include "ObjectPoolManager.h"
#include "ActorPool.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

TArray<TWeakObjectPtr<AGamePawn>> FCombatObjectTestCommands::SpawnedTestActors;
int32 FCombatObjectTestCommands::CurrentTestTypeID = -1;

static FAutoConsoleCommand CVarCombatObjectSpawn(
    TEXT("CombatObject.Spawn"),
    TEXT("Spawn combat objects by TypeID. Usage: CombatObject.Spawn <TypeID> [Count]"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 1)
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: CombatObject.Spawn <TypeID> [Count]"));
            return;
        }

        int32 TypeID = FCString::Atoi(*Args[0]);
        int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;

        UWorld* World = GEngine->GameViewport->GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.Spawn: World is null"));
            return;
        }

        UCombatObjectFactory* Factory = World->GetSubsystem<UCombatObjectFactory>();
        if (!Factory)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.Spawn: CombatObjectFactory not found"));
            return;
        }

        if (!Factory->IsConfigLoaded())
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.Spawn: Config not loaded. Please load config first."));
            return;
        }

        FVector PlayerLocation(0.0f);
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* PlayerPawn = PC->GetPawn())
            {
                PlayerLocation = PlayerPawn->GetActorLocation();
            }
        }

        int32 SpawnedCount = 0;
        for (int32 i = 0; i < Count; i++)
        {
            FVector2D RandomOffset2D = FMath::RandPointInCircle(FMath::RandRange(200.0f, 1000.0f));
            FVector SpawnLocation = PlayerLocation + FVector(RandomOffset2D.X, RandomOffset2D.Y, 0.0f);

            AGamePawn* SpawnedActor = Factory->SpawnCombatObject(TypeID, SpawnLocation);
            if (SpawnedActor)
            {
                FCombatObjectTestCommands::SpawnedTestActors.Add(SpawnedActor);
                SpawnedCount++;
            }
        }

        FCombatObjectTestCommands::CurrentTestTypeID = TypeID;
        UE_LOG(LogTemp, Log, TEXT("CombatObject.Spawn: Spawned %d/%d objects of TypeID %d"), 
            SpawnedCount, Count, TypeID);
    })
);

static FAutoConsoleCommand CVarCombatObjectReturnAll(
    TEXT("CombatObject.ReturnAll"),
    TEXT("Return all spawned test combat objects to pool"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        UWorld* World = GEngine->GameViewport->GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.ReturnAll: World is null"));
            return;
        }

        UCombatObjectFactory* Factory = World->GetSubsystem<UCombatObjectFactory>();
        if (!Factory)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.ReturnAll: CombatObjectFactory not found"));
            return;
        }

        int32 ReturnedCount = 0;
        for (int32 i = FCombatObjectTestCommands::SpawnedTestActors.Num() - 1; i >= 0; i--)
        {
            if (AGamePawn* Actor = FCombatObjectTestCommands::SpawnedTestActors[i].Get())
            {
                Factory->ReturnCombatObject(Actor);
                ReturnedCount++;
            }
        }

        FCombatObjectTestCommands::SpawnedTestActors.Empty();
        UE_LOG(LogTemp, Log, TEXT("CombatObject.ReturnAll: Returned %d objects to pool"), ReturnedCount);
    })
);

static FAutoConsoleCommand CVarCombatObjectListTypes(
    TEXT("CombatObject.ListTypes"),
    TEXT("List all available combat object types"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        UWorld* World = GEngine->GameViewport->GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.ListTypes: World is null"));
            return;
        }

        UCombatObjectFactory* Factory = World->GetSubsystem<UCombatObjectFactory>();
        if (!Factory)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.ListTypes: CombatObjectFactory not found"));
            return;
        }

        if (!Factory->IsConfigLoaded())
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.ListTypes: Config not loaded"));
            return;
        }

        TArray<int32> TypeIDs = Factory->GetAllTypeIDs();
        UE_LOG(LogTemp, Log, TEXT("=== Available Combat Object Types ==="));
        UE_LOG(LogTemp, Log, TEXT("Total: %d types"), TypeIDs.Num());

        for (int32 TypeID : TypeIDs)
        {
            FCombatObjectConfig Config;
            if (Factory->GetConfig(TypeID, Config))
            {
                UE_LOG(LogTemp, Log, TEXT("  TypeID: %d, Name: %s, Class: %s"),
                    TypeID, *Config.TypeName, *Config.ActorClass->GetName());
            }
        }
    })
);

static FAutoConsoleCommand CVarCombatObjectStats(
    TEXT("CombatObject.Stats"),
    TEXT("Show combat object pool statistics"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        UWorld* World = GEngine->GameViewport->GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.Stats: World is null"));
            return;
        }

        UCombatObjectFactory* Factory = World->GetSubsystem<UCombatObjectFactory>();
        if (!Factory)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.Stats: CombatObjectFactory not found"));
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("=== Combat Object Factory Stats ==="));
        UE_LOG(LogTemp, Log, TEXT("  Config Loaded: %s"), Factory->IsConfigLoaded() ? TEXT("Yes") : TEXT("No"));
        UE_LOG(LogTemp, Log, TEXT("  Available Types: %d"), Factory->GetAllTypeIDs().Num());
        UE_LOG(LogTemp, Log, TEXT("  Active Test Actors: %d"), FCombatObjectTestCommands::SpawnedTestActors.Num());
        UE_LOG(LogTemp, Log, TEXT("  Current Test TypeID: %d"), FCombatObjectTestCommands::CurrentTestTypeID);

        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UObjectPoolManager* PoolManager = GameInstance->GetSubsystem<UObjectPoolManager>())
            {
                TMap<FString, FActorPoolStats> ActorStats;
                TMap<FString, FObjectPoolStats> ObjectStats;
                PoolManager->GetAllPoolStats(ActorStats, ObjectStats);

                UE_LOG(LogTemp, Log, TEXT("  === Object Pool Stats ==="));
                for (const auto& Pair : ActorStats)
                {
                    UE_LOG(LogTemp, Log, TEXT("    Pool: %s"), *Pair.Key);
                    UE_LOG(LogTemp, Log, TEXT("      Total Created: %d"), Pair.Value.TotalCreated);
                    UE_LOG(LogTemp, Log, TEXT("      Current Active: %d"), Pair.Value.CurrentActive);
                    UE_LOG(LogTemp, Log, TEXT("      Current Available: %d"), Pair.Value.CurrentAvailable);
                    UE_LOG(LogTemp, Log, TEXT("      Peak Usage: %d"), Pair.Value.PeakUsage);
                }
            }
        }
    })
);

static FAutoConsoleCommand CVarCombatObjectPrewarm(
    TEXT("CombatObject.Prewarm"),
    TEXT("Prewarm combat object pool. Usage: CombatObject.Prewarm <TypeID> <Count>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 2)
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: CombatObject.Prewarm <TypeID> <Count>"));
            return;
        }

        int32 TypeID = FCString::Atoi(*Args[0]);
        int32 Count = FCString::Atoi(*Args[1]);

        UWorld* World = GEngine->GameViewport->GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.Prewarm: World is null"));
            return;
        }

        UCombatObjectFactory* Factory = World->GetSubsystem<UCombatObjectFactory>();
        if (!Factory)
        {
            UE_LOG(LogTemp, Warning, TEXT("CombatObject.Prewarm: CombatObjectFactory not found"));
            return;
        }

        Factory->PrewarmPool(TypeID, Count);
    })
);

void FCombatObjectTestCommands::Initialize()
{
    SpawnedTestActors.Empty();
    CurrentTestTypeID = -1;
}

void FCombatObjectTestCommands::Cleanup()
{
    SpawnedTestActors.Empty();
    CurrentTestTypeID = -1;
}
