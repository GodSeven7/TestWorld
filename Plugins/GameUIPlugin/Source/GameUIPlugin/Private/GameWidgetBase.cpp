#include "GameWidgetBase.h"

void UGameWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    UGameInstance* GameInst = GetGameInstance();
    if (!GameInst)
    {
        return;
    }

    GameDataSubsystem = GameInst->GetSubsystem<UGameDataSubsystem>();
    if (!GameDataSubsystem)
    {
        return;
    }

    GameDataSubsystem->OnGameTimeChanged.AddDynamic(this, &UGameWidgetBase::OnGameTimeChanged);
    GameDataSubsystem->OnEnemyCountChanged.AddDynamic(this, &UGameWidgetBase::OnEnemyCountChanged);
    GameDataSubsystem->OnPlayerHealthChanged.AddDynamic(this, &UGameWidgetBase::OnPlayerHealthChanged);
    GameDataSubsystem->OnGameResultChanged.AddDynamic(this, &UGameWidgetBase::OnGameResultChanged);
}

void UGameWidgetBase::NativeDestruct()
{
    if (GameDataSubsystem)
    {
        GameDataSubsystem->OnGameTimeChanged.RemoveDynamic(this, &UGameWidgetBase::OnGameTimeChanged);
        GameDataSubsystem->OnEnemyCountChanged.RemoveDynamic(this, &UGameWidgetBase::OnEnemyCountChanged);
        GameDataSubsystem->OnPlayerHealthChanged.RemoveDynamic(this, &UGameWidgetBase::OnPlayerHealthChanged);
        GameDataSubsystem->OnGameResultChanged.RemoveDynamic(this, &UGameWidgetBase::OnGameResultChanged);
    }

    Super::NativeDestruct();
}

UGameDataSubsystem* UGameWidgetBase::GetGameData() const
{
    return GameDataSubsystem;
}
