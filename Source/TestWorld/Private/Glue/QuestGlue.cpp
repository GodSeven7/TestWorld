#include "Glue/QuestGlue.h"
#include "QuestManager.h"

static bool bDisableQuest = false;
FAutoConsoleVariableRef CVarDisableQuest(
	TEXT("Disable.Quest"),
	bDisableQuest,
	TEXT(""));

void UQuestGlue::Initialize(UWorld* World)
{
}

void UQuestGlue::Shutdown()
{
    if (EventBus)
    {
        EventBus->OnCombatantDeath.RemoveDynamic(this, &UQuestGlue::OnCombatantDeath);
    }
}

void UQuestGlue::Bind(
    UQuestManager* InQuestManager,
    UEventBus* InEventBus)
{
    QuestManager = InQuestManager;
    EventBus = InEventBus;

    if (EventBus)
    {
        EventBus->OnCombatantDeath.AddDynamic(this, &UQuestGlue::OnCombatantDeath);
    }
}

void UQuestGlue::Tick(float DeltaTime)
{
    if (bSuspended || !QuestManager || bDisableQuest)
        return;

    QuestManager->TickQuests(DeltaTime);
}

void UQuestGlue::OnCombatantDeath(const FCombatantDeathEvent& Event)
{
    if (QuestManager && Event.DeadActor.IsValid())
    {
        QuestManager->NotifyActorDeath(Event.DeadActor.Get());
    }
}
