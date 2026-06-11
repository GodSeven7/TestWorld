#include "GameHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"


void UGameHUDWidget::OnGameTimeChanged_Implementation(float NewTime)
{
    if (GameTimeText)
    {
        int32 Minutes = FMath::FloorToInt(NewTime) / 60;
        int32 Seconds = FMath::FloorToInt(NewTime) % 60;
        //GameTimeText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
		GameTimeText->SetText(FText::FromString(FString::Printf(TEXT("%.3f"), NewTime)));
    }
}

void UGameHUDWidget::OnEnemyCountChanged_Implementation(int32 NewCount)
{
    if (EnemyCountText)
    {
        EnemyCountText->SetText(FText::FromString(FString::FromInt(NewCount)));
    }
}

void UGameHUDWidget::OnPlayerHealthChanged_Implementation(float CurrentHealth, float MaxHealth)
{
    if (HealthBar)
    {
        float Percent = (MaxHealth > 0.f) ? CurrentHealth / MaxHealth : 0.f;
        HealthBar->SetPercent(Percent);
    }
    if (HealthText)
    {
        HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, MaxHealth)));
    }
}

void UGameHUDWidget::OnGameResultChanged_Implementation(EGameResult NewResult)
{
    if (GameResultText)
    {
        switch (NewResult)
        {
        case EGameResult::Victory:
            GameResultText->SetText(FText::FromString(TEXT("Victory")));
            break;
        case EGameResult::Defeat:
            GameResultText->SetText(FText::FromString(TEXT("Defeat")));
            break;
        default:
            GameResultText->SetText(FText::GetEmpty());
            break;
        }
    }
}
