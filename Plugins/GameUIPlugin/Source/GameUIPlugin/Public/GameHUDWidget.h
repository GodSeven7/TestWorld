#pragma once

#include "CoreMinimal.h"
#include "GameWidgetBase.h"
#include "GameHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Abstract)
class GAMEUIPLUGIN_API UGameHUDWidget : public UGameWidgetBase
{
    GENERATED_BODY()

protected:
    virtual void OnGameTimeChanged_Implementation(float NewTime) override;
    virtual void OnEnemyCountChanged_Implementation(int32 NewCount) override;
    virtual void OnPlayerHealthChanged_Implementation(float CurrentHealth, float MaxHealth) override;
    virtual void OnGameResultChanged_Implementation(EGameResult NewResult) override;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* GameTimeText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* EnemyCountText;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* HealthBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* HealthText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* GameResultText;
};
