#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameUITypes.h"
#include "GameDataSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameTimeChanged, float, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyCountChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerHealthChanged, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameResultChanged, EGameResult, NewResult);

UCLASS()
class GAMEUIPLUGIN_API UGameDataSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "GameData")
    void ResetGameData();

    UFUNCTION(BlueprintCallable, Category = "GameData")
    void SetGameTime(float InGameTime);

    UFUNCTION(BlueprintCallable, Category = "GameData")
    void SetEnemyCount(int32 InEnemyCount);

    UFUNCTION(BlueprintCallable, Category = "GameData")
    void SetPlayerHealth(float InCurrentHealth, float InMaxHealth);

    UFUNCTION(BlueprintCallable, Category = "GameData")
    void SetGameResult(EGameResult InGameResult);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameData")
    float GetGameTime() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameData")
    int32 GetEnemyCount() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameData")
    float GetCurrentHealth() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameData")
    float GetMaxHealth() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameData")
    float GetHealthPercent() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameData")
    EGameResult GetGameResult() const;

    UPROPERTY(BlueprintAssignable, Category = "GameData")
    FOnGameTimeChanged OnGameTimeChanged;

    UPROPERTY(BlueprintAssignable, Category = "GameData")
    FOnEnemyCountChanged OnEnemyCountChanged;

    UPROPERTY(BlueprintAssignable, Category = "GameData")
    FOnPlayerHealthChanged OnPlayerHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "GameData")
    FOnGameResultChanged OnGameResultChanged;

private:
    float GameTime = 0.f;
    int32 EnemyCount = 0;
    float CurrentHealth = 100.f;
    float MaxHealth = 100.f;
    EGameResult GameResult = EGameResult::InProgress;
};
