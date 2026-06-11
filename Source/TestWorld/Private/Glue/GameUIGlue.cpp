#include "Glue/GameUIGlue.h"
#include "GameDataSubsystem.h"
#include "QuestManager.h"
#include "QuestInstance.h"
#include "QuestObjective_Timer.h"
#include "CombatManager.h"
#include "Actors/GamePawn.h"
#include "Components/BattleDamageWithUnifiedDataComponent.h"
#include "BattleDamageTypes.h"
#include "Kismet/GameplayStatics.h"

void UGameUIGlue::Initialize(UWorld* World)
{
}

void UGameUIGlue::Shutdown()
{
    if (EventBus)
    {
        EventBus->OnCombatantDeath.RemoveDynamic(this, &UGameUIGlue::OnCombatantDeath);
    }
}

void UGameUIGlue::Bind(
    UGameDataSubsystem* InGameDataSubsystem,
    UQuestManager* InQuestManager,
    UCombatManager* InCombatManager,
    UEventBus* InEventBus)
{
    GameDataSubsystem = InGameDataSubsystem;
    QuestManager = InQuestManager;
    CombatManager = InCombatManager;
    EventBus = InEventBus;

    if (EventBus)
    {
        EventBus->OnCombatantDeath.AddDynamic(this, &UGameUIGlue::OnCombatantDeath);
    }
}

void UGameUIGlue::Tick(float DeltaTime)
{
    if (!GameDataSubsystem)
        return;

    if (QuestManager)
    {
        UQuestInstance* MainQuest = QuestManager->GetQuest(FName("MainQuest"));
        if (MainQuest)
        {
            for (UQuestObjective* Obj : MainQuest->GetObjectives())
            {
                if (UQuestObjective_Timer* Timer = Cast<UQuestObjective_Timer>(Obj))
                {
                    GameDataSubsystem->SetGameTime(Timer->GetRemainingTime());
                    break;
                }
            }
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AGamePawn* PlayerPawn = Cast<AGamePawn>(PC->GetPawn()))
            {
                if (UBattleDamageWithUnifiedDataComponent* DmgComp = PlayerPawn->GetBattleDamageComponent())
                {
                    const FBattleAttributeData& Attr = DmgComp->GetAttributeData();
                    GameDataSubsystem->SetPlayerHealth(Attr.CurrentHealth, Attr.MaxHealth);
                }
            }
        }
    }
}

void UGameUIGlue::OnCombatantDeath(const FCombatantDeathEvent& Event)
{
    if (!GameDataSubsystem || !CombatManager)
        return;

    int32 Count = CombatManager->GetRegisteredComponentCount();
    GameDataSubsystem->SetEnemyCount(Count);
}
