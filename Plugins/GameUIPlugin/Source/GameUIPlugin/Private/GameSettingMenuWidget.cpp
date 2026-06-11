#include "GameSettingMenuWidget.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "GameUISubsystem.h"

void UGameSettingMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (BackButton)
    {
        BackButton->OnClicked.AddDynamic(this, &UGameSettingMenuWidget::OnBackClicked);
    }
}

void UGameSettingMenuWidget::OnBackClicked()
{
    UGameInstance* GameInst = GetGameInstance();
    if (!GameInst)
    {
        return;
    }

    if (UGameUISubsystem* UISubsys = GameInst->GetSubsystem<UGameUISubsystem>())
    {
        UISubsys->HideSettingMenu();
        UISubsys->ShowStartMenu();
    }
}
