#include "GameMode/TestWorldGameMode.h"
#include "QuestManager.h"
#include "QuestInstance.h"
#include "QuestObjective_Timer.h"
#include "QuestObjective_ActorDeath.h"
#include "GameDataSubsystem.h"
#include "GameUISubsystem.h"
#include "GameUIConfig.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Actors/GameHero.h"
#include "Components/BattleDamageWithUnifiedDataComponent.h"
#include "Subsystems/SpawnerManager.h"
#include "Data/SpawnerConfig.h"
#include "Coordinator/PluginCoordinator.h"
#include "CombatManager.h"
#include "SparseGridConfig.h"
#include "SparseGridManager.h"

ATestWorldGameMode::ATestWorldGameMode()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ATestWorldGameMode::BeginPlay()
{
    Super::BeginPlay();

    UGameInstance* GI = GetGameInstance();

    GameDataSubsystem = GI ? GI->GetSubsystem<UGameDataSubsystem>() : nullptr;
    GameUISubsystem = GI ? GI->GetSubsystem<UGameUISubsystem>() : nullptr;

    if (GameDataSubsystem)
    {
        GameDataSubsystem->ResetGameData();
    }

    if (GameUISubsystem)
    {
        GameUISubsystem->OnRequestStartGame.AddDynamic(this, &ATestWorldGameMode::OnRequestStartGame);
        GameUISubsystem->OnRequestRestartGame.AddDynamic(this, &ATestWorldGameMode::OnRequestRestartGame);
        GameUISubsystem->OnRequestQuitGame.AddDynamic(this, &ATestWorldGameMode::OnRequestQuitGame);
    }

    if (GameUISubsystem && UIConfig)
    {
        GameUISubsystem->SetUIConfig(UIConfig);
        GameUISubsystem->ShowStartMenu();
    }

    if (USparseGridManager* GridMgr = GetWorld()->GetSubsystem<USparseGridManager>())
    {
        if (SparseGridConfig)
        {
            GridMgr->SetSparseGridConfig(SparseGridConfig);
        }
    }

    // ── 注入 Combat 配置表 ──
    if (UPluginCoordinator* Coordinator = GetWorld()->GetSubsystem<UPluginCoordinator>())
    {
        if (UCombatManager* CombatMgr = Coordinator->GetCombatManager())
        {
            if (SkillTable)     CombatMgr->SetSkillTable(SkillTable);
            if (WeaponTable)    CombatMgr->SetWeaponTable(WeaponTable);
            if (CharacterTable) CombatMgr->SetCharacterTable(CharacterTable);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("GameMode initialized - waiting for Start"));

    OnInitGameplay();
}

void ATestWorldGameMode::OnInitGameplay_Implementation()
{
}

void ATestWorldGameMode::OnRequestStartGame()
{
    if (bGameStarted)
    {
        return;
    }
    bGameStarted = true;

    QuestManager = GetWorld()->GetSubsystem<UQuestManager>();
    if (!QuestManager)
    {
        UE_LOG(LogTemp, Error, TEXT("TestWorldGameMode: QuestManager not found"));
        return;
    }

    MainQuest = QuestManager->CreateQuest(MainQuestID);

    UQuestObjective_Timer* TimerObj = NewObject<UQuestObjective_Timer>(MainQuest);
    TimerObj->Setup(QuestTimeLimit, EQuestObjectiveResult::Victory);
    MainQuest->AddObjective(TimerObj);

    if (EnemyClass)
    {
        UQuestObjective_ActorDeath* EnemyKill = NewObject<UQuestObjective_ActorDeath>(MainQuest);
        EnemyKill->Setup(EnemyClass, 1, EQuestObjectiveResult::Victory);
        MainQuest->AddObjective(EnemyKill);
    }

    if (PlayerClass)
    {
        UQuestObjective_ActorDeath* PlayerDeath = NewObject<UQuestObjective_ActorDeath>(MainQuest);
        PlayerDeath->Setup(PlayerClass, 1, EQuestObjectiveResult::Defeat);
        MainQuest->AddObjective(PlayerDeath);
    }

    MainQuest->OnQuestFinished.AddDynamic(this, &ATestWorldGameMode::OnMainQuestFinished);

    QuestManager->StartQuest(MainQuest);

    if (GameDataSubsystem)
    {
        AGameHero* Hero = Cast<AGameHero>(UGameplayStatics::GetPlayerPawn(this, 0));
        if (Hero && Hero->GetBattleDamageComponent())
        {
            const FBattleAttributeData& Attr = Hero->GetBattleDamageComponent()->GetAttributeData();
            GameDataSubsystem->SetPlayerHealth(Attr.CurrentHealth, Attr.MaxHealth);
        }
    }

    if (GameUISubsystem)
    {
        GameUISubsystem->HideStartMenu();
        GameUISubsystem->ShowHUD();
    }

    if (USpawnerManager* SpawnerMgr = GetWorld()->GetSubsystem<USpawnerManager>())
    {
        if (SpawnerConfig)
        {
            SpawnerMgr->StartSpawning(SpawnerConfig);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Game Started - MainQuest [%s] active, TimeLimit=%.1fs"),
        *MainQuestID.ToString(), QuestTimeLimit);
}

void ATestWorldGameMode::OnRequestRestartGame()
{
    RestartGame();
}

void ATestWorldGameMode::OnRequestQuitGame()
{
    UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(),
        EQuitPreference::Quit, false);
}

void ATestWorldGameMode::OnMainQuestFinished(EQuestObjectiveResult Result, const FString& Reason)
{
    if (bGameEnded)
    {
        return;
    }

    EndGame(Result, Reason);
}

void ATestWorldGameMode::EndGame(EQuestObjectiveResult Result, const FString& Reason)
{
    bGameEnded = true;

    if (QuestManager)
    {
        QuestManager->StopQuest(MainQuestID);
    }

    if (USpawnerManager* SpawnerMgr = GetWorld()->GetSubsystem<USpawnerManager>())
    {
        SpawnerMgr->StopSpawning();
    }

    if (GameDataSubsystem)
    {
        GameDataSubsystem->SetGameResult(
            Result == EQuestObjectiveResult::Victory ? EGameResult::Victory : EGameResult::Defeat);
    }

    if (GameUISubsystem)
    {
        GameUISubsystem->HideHUD();
        GameUISubsystem->ShowGameOverMenu();
    }

    if (Result == EQuestObjectiveResult::Victory)
    {
        UE_LOG(LogTemp, Log, TEXT("Game Victory! Reason: %s"), *Reason);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Game Defeat! Reason: %s"), *Reason);
    }
}

void ATestWorldGameMode::RestartGame()
{
    UE_LOG(LogTemp, Log, TEXT("Restarting Game..."));

    UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}
