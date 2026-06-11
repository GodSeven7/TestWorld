#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "QuestTypes.h"
#include "TestWorldGameMode.generated.h"

class UQuestInstance;
class UQuestManager;
class UGameDataSubsystem;
class UGameUISubsystem;
class UGameUIConfig;
class USpawnerConfig;
class USparseGridConfig;
class UDataTable;

UCLASS()
class ATestWorldGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATestWorldGameMode();

    virtual void BeginPlay() override;

protected:
    UFUNCTION(BlueprintNativeEvent, Category = "Gameplay")
    void OnInitGameplay();
    virtual void OnInitGameplay_Implementation();

    UFUNCTION()
    void OnRequestStartGame();

    UFUNCTION()
    void OnRequestRestartGame();

    UFUNCTION()
    void OnRequestQuitGame();

    UFUNCTION()
    void OnMainQuestFinished(EQuestObjectiveResult Result, const FString& Reason);

    virtual void EndGame(EQuestObjectiveResult Result, const FString& Reason);

    void RestartGame();

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Quest")
    FName MainQuestID = FName("MainQuest");

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Quest")
    float QuestTimeLimit = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Quest")
    TSubclassOf<AActor> EnemyClass;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Quest")
    TSubclassOf<AActor> PlayerClass;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|UI")
    UGameUIConfig* UIConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Spawner")
    USpawnerConfig* SpawnerConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|SparseGrid")
    USparseGridConfig* SparseGridConfig;

    // ── Combat 配置表 ──
    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Combat")
    UDataTable* SkillTable;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Combat")
    UDataTable* WeaponTable;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Combat")
    UDataTable* CharacterTable;

    UPROPERTY()
    UQuestInstance* MainQuest;

    UPROPERTY()
    UQuestManager* QuestManager;

    UPROPERTY()
    UGameDataSubsystem* GameDataSubsystem;

    UPROPERTY()
    UGameUISubsystem* GameUISubsystem;

    bool bGameStarted = false;
    bool bGameEnded = false;
};
