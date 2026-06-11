#include "GameOverMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameUISubsystem.h"

void UGameGameOverMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (RestartButton)
    {
        RestartButton->OnClicked.AddDynamic(this, &UGameGameOverMenuWidget::OnRestartClicked);
    }
    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &UGameGameOverMenuWidget::OnQuitClicked);
    }
}

void UGameGameOverMenuWidget::OnGameResultChanged_Implementation(EGameResult NewResult)
{
    if (ResultText)
    {
        switch (NewResult)
        {
        case EGameResult::Victory:
            ResultText->SetText(FText::FromString(TEXT("Victory")));
            break;
        case EGameResult::Defeat:
            ResultText->SetText(FText::FromString(TEXT("Defeat")));
            break;
        default:
            ResultText->SetText(FText::GetEmpty());
            break;
        }
    }
}

void UGameGameOverMenuWidget::OnRestartClicked()
{
    UGameInstance* GameInst = GetGameInstance();
    if (!GameInst)
    {
        return;
    }

    if (UGameUISubsystem* UISubsys = GameInst->GetSubsystem<UGameUISubsystem>())
    {
        UISubsys->RequestRestartGame();
    }
}

void UGameGameOverMenuWidget::OnQuitClicked()
{
    UGameInstance* GameInst = GetGameInstance();
    if (!GameInst)
    {
        return;
    }

    if (UGameUISubsystem* UISubsys = GameInst->GetSubsystem<UGameUISubsystem>())
    {
        UISubsys->RequestQuitGame();
    }
}
