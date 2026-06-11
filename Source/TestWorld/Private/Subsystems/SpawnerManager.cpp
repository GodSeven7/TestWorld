#include "Subsystems/SpawnerManager.h"
#include "Subsystems/CombatObjectFactory.h"
#include "BattleDamageManager.h"
#include "BattleDamageTypes.h"
#include "Actors/GamePawn.h"
#include "Engine/World.h"

void USpawnerManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Factory = GetWorld()->GetSubsystem<UCombatObjectFactory>();
    BattleDamageManager = GetWorld()->GetSubsystem<UBattleDamageManager>();

    if (BattleDamageManager)
    {
        BattleDamageManager->OnCombatantDeath.AddDynamic(this, &USpawnerManager::OnCombatantDeath);
    }

    UE_LOG(LogTemp, Log, TEXT("SpawnerManager initialized"));
}

void USpawnerManager::Deinitialize()
{
    if (BattleDamageManager)
    {
        BattleDamageManager->OnCombatantDeath.RemoveDynamic(this, &USpawnerManager::OnCombatantDeath);
    }

    StopSpawning();

    Factory = nullptr;
    BattleDamageManager = nullptr;

    Super::Deinitialize();
}

void USpawnerManager::StartSpawning(USpawnerConfig* Config)
{
    StopSpawning();

    if (!Config)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnerManager: Config is null"));
        return;
    }

    if (!Factory)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnerManager: CombatObjectFactory not available"));
        return;
    }

    for (const FSpawnRule& Rule : Config->SpawnRules)
    {
        if (!Rule.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("SpawnerManager: Skipping invalid spawn rule (TypeID=%d)"), Rule.TypeID);
            continue;
        }

        FActiveSpawnRule ActiveRule;
        ActiveRule.Rule = Rule;
        ActiveRule.TimeSinceLastSpawn = 0.f;
        ActiveRule.TimeSinceStart = 0.f;
        ActiveRule.NextSpawnPointIndex = 0;
        ActiveRule.bStarted = (Rule.InitialDelay <= 0.f);
        ActiveRules.Add(MoveTemp(ActiveRule));
    }

    bIsSpawning = true;
    UE_LOG(LogTemp, Log, TEXT("SpawnerManager: Started spawning with %d rules"), ActiveRules.Num());
}

void USpawnerManager::StopSpawning()
{
    if (!bIsSpawning)
    {
        return;
    }

    ReturnAllSpawnedActors();

    ActiveRules.Empty();
    bIsSpawning = false;

    UE_LOG(LogTemp, Log, TEXT("SpawnerManager: Stopped spawning"));
}

void USpawnerManager::TickSpawning(float DeltaTime)
{
    if (!bIsSpawning || !Factory)
    {
        return;
    }

    for (FActiveSpawnRule& ActiveRule : ActiveRules)
    {
        CleanupInvalidActors(ActiveRule);

        ActiveRule.TimeSinceStart += DeltaTime;

        if (!ActiveRule.bStarted)
        {
            if (ActiveRule.TimeSinceStart >= ActiveRule.Rule.InitialDelay)
            {
                ActiveRule.bStarted = true;
                ActiveRule.TimeSinceLastSpawn = ActiveRule.Rule.SpawnInterval;
            }
            else
            {
                continue;
            }
        }

        if (ActiveRule.AliveActors.Num() >= ActiveRule.Rule.MaxCount)
        {
            continue;
        }

        ActiveRule.TimeSinceLastSpawn += DeltaTime;

        if (ActiveRule.TimeSinceLastSpawn >= ActiveRule.Rule.SpawnInterval)
        {
            int32 ToSpawn = FMath::Min(ActiveRule.Rule.SpawnCount, ActiveRule.Rule.MaxCount - ActiveRule.AliveActors.Num());
            for (int32 i = 0; i < ToSpawn; i++)
            {
                SpawnOne(ActiveRule);
            }
            ActiveRule.TimeSinceLastSpawn = 0.f;
        }
    }
}

int32 USpawnerManager::GetAliveCount(int32 TypeID) const
{
    int32 Count = 0;
    for (const FActiveSpawnRule& ActiveRule : ActiveRules)
    {
        if (ActiveRule.Rule.TypeID == TypeID)
        {
            for (const TWeakObjectPtr<AGamePawn>& WeakPtr : ActiveRule.AliveActors)
            {
                if (WeakPtr.IsValid())
                {
                    Count++;
                }
            }
        }
    }
    return Count;
}

int32 USpawnerManager::GetTotalAliveCount() const
{
    int32 Count = 0;
    for (const FActiveSpawnRule& ActiveRule : ActiveRules)
    {
        for (const TWeakObjectPtr<AGamePawn>& WeakPtr : ActiveRule.AliveActors)
        {
            if (WeakPtr.IsValid())
            {
                Count++;
            }
        }
    }
    return Count;
}

void USpawnerManager::OnCombatantDeath(AActor* Combatant, const FDamageInfo& KillingBlow)
{
    if (!Combatant)
    {
        return;
    }

    AGamePawn* DeadPawn = Cast<AGamePawn>(Combatant);
    if (!DeadPawn)
    {
        return;
    }

    for (FActiveSpawnRule& ActiveRule : ActiveRules)
    {
        int32 Removed = ActiveRule.AliveActors.Remove(DeadPawn);
        if (Removed > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("SpawnerManager: TypeID=%d actor died, alive=%d/%d"),
                ActiveRule.Rule.TypeID, ActiveRule.AliveActors.Num(), ActiveRule.Rule.MaxCount);
        }
    }
}

void USpawnerManager::CleanupInvalidActors(FActiveSpawnRule& ActiveRule)
{
    ActiveRule.AliveActors.RemoveAll([](const TWeakObjectPtr<AGamePawn>& WeakPtr)
    {
        return !WeakPtr.IsValid();
    });
}

FVector USpawnerManager::ChooseSpawnLocation(FActiveSpawnRule& ActiveRule)
{
    const FSpawnRule& Rule = ActiveRule.Rule;

    FVector Location;

    if (Rule.LocationMode == ESpawnLocationMode::FixedPoints && Rule.SpawnPoints.Num() > 0)
    {
        Location = Rule.SpawnPoints[ActiveRule.NextSpawnPointIndex];
        ActiveRule.NextSpawnPointIndex = (ActiveRule.NextSpawnPointIndex + 1) % Rule.SpawnPoints.Num();
    }
    else
    {
        FVector2D RandomOffset2D = FMath::RandPointInCircle(Rule.SpawnRadius);
        Location = Rule.SpawnCenter + FVector(RandomOffset2D.X, RandomOffset2D.Y, 0.0f);
    }

    Location.Z = 0.0f;
    return Location;
}

void USpawnerManager::SpawnOne(FActiveSpawnRule& ActiveRule)
{
    FVector Location = ChooseSpawnLocation(ActiveRule);

    AGamePawn* SpawnedActor = Factory->SpawnCombatObject(ActiveRule.Rule.TypeID, Location);
    if (SpawnedActor)
    {
        ActiveRule.AliveActors.Add(SpawnedActor);
        UE_LOG(LogTemp, Log, TEXT("SpawnerManager: Spawned TypeID=%d at %s, alive=%d/%d"),
            ActiveRule.Rule.TypeID, *Location.ToString(), ActiveRule.AliveActors.Num(), ActiveRule.Rule.MaxCount);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnerManager: Failed to spawn TypeID=%d"), ActiveRule.Rule.TypeID);
    }
}

void USpawnerManager::ReturnAllSpawnedActors()
{
    for (FActiveSpawnRule& ActiveRule : ActiveRules)
    {
        for (TWeakObjectPtr<AGamePawn>& WeakPtr : ActiveRule.AliveActors)
        {
            if (WeakPtr.IsValid() && Factory)
            {
                Factory->ReturnCombatObject(WeakPtr.Get());
            }
        }
        ActiveRule.AliveActors.Empty();
    }
}
