#include "GameStartMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameUISubsystem.h"

void UGameStartMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (StartButton)
    {
        StartButton->OnClicked.AddDynamic(this, &UGameStartMenuWidget::OnStartClicked);
    }
    if (SettingsButton)
    {
        SettingsButton->OnClicked.AddDynamic(this, &UGameStartMenuWidget::OnSettingsClicked);
    }
}

void UGameStartMenuWidget::OnStartClicked()
{
    UGameInstance* GameInst = GetGameInstance();
    if (!GameInst)
    {
        return;
    }

    if (UGameUISubsystem* UISubsys = GameInst->GetSubsystem<UGameUISubsystem>())
    {
        UISubsys->RequestStartGame();
    }
}

void UGameStartMenuWidget::OnSettingsClicked()
{
    UGameInstance* GameInst = GetGameInstance();
    if (!GameInst)
    {
        return;
    }

    if (UGameUISubsystem* UISubsys = GameInst->GetSubsystem<UGameUISubsystem>())
    {
        UISubsys->HideStartMenu();
        UISubsys->ShowSettingMenu();
    }
}
