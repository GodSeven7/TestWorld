#include "GameDataSubsystem.h"

void UGameDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UGameDataSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UGameDataSubsystem::ResetGameData()
{
    float OldGameTime = GameTime;
    int32 OldEnemyCount = EnemyCount;
    float OldCurrentHealth = CurrentHealth;
    float OldMaxHealth = MaxHealth;
    EGameResult OldGameResult = GameResult;

    GameTime = 0.f;
    EnemyCount = 0;
    CurrentHealth = 100.f;
    MaxHealth = 100.f;
    GameResult = EGameResult::InProgress;

    if (!FMath::IsNearlyEqual(OldGameTime, GameTime))
    {
        OnGameTimeChanged.Broadcast(GameTime);
    }
    if (OldEnemyCount != EnemyCount)
    {
        OnEnemyCountChanged.Broadcast(EnemyCount);
    }
    if (!FMath::IsNearlyEqual(OldCurrentHealth, CurrentHealth) || !FMath::IsNearlyEqual(OldMaxHealth, MaxHealth))
    {
        OnPlayerHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    }
    if (OldGameResult != GameResult)
    {
        OnGameResultChanged.Broadcast(GameResult);
    }
}

void UGameDataSubsystem::SetGameTime(float InGameTime)
{
    GameTime = InGameTime;
    OnGameTimeChanged.Broadcast(GameTime);
}

void UGameDataSubsystem::SetEnemyCount(int32 InEnemyCount)
{
    if (EnemyCount == InEnemyCount)
    {
        return;
    }
    EnemyCount = InEnemyCount;
    OnEnemyCountChanged.Broadcast(EnemyCount);
}

void UGameDataSubsystem::SetPlayerHealth(float InCurrentHealth, float InMaxHealth)
{
    bool bHealthChanged = !FMath::IsNearlyEqual(CurrentHealth, InCurrentHealth);
    bool bMaxChanged = !FMath::IsNearlyEqual(MaxHealth, InMaxHealth);
    if (!bHealthChanged && !bMaxChanged)
    {
        return;
    }
    CurrentHealth = InCurrentHealth;
    MaxHealth = InMaxHealth;
    OnPlayerHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UGameDataSubsystem::SetGameResult(EGameResult InGameResult)
{
    if (GameResult == InGameResult)
    {
        return;
    }
    GameResult = InGameResult;
    OnGameResultChanged.Broadcast(GameResult);
}

float UGameDataSubsystem::GetGameTime() const
{
    return GameTime;
}

int32 UGameDataSubsystem::GetEnemyCount() const
{
    return EnemyCount;
}

float UGameDataSubsystem::GetCurrentHealth() const
{
    return CurrentHealth;
}

float UGameDataSubsystem::GetMaxHealth() const
{
    return MaxHealth;
}

float UGameDataSubsystem::GetHealthPercent() const
{
    if (MaxHealth <= 0.f)
    {
        return 0.f;
    }
    return CurrentHealth / MaxHealth;
}

EGameResult UGameDataSubsystem::GetGameResult() const
{
    return GameResult;
}
