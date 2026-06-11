#pragma once

#include "CoreMinimal.h"
#include "GameWidgetBase.h"
#include "GameOverMenuWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS(Abstract)
class GAMEUIPLUGIN_API UGameGameOverMenuWidget : public UGameWidgetBase
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void OnGameResultChanged_Implementation(EGameResult NewResult) override;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ResultText;

    UPROPERTY(meta = (BindWidget))
    UButton* RestartButton;

    UPROPERTY(meta = (BindWidget))
    UButton* QuitButton;

private:
    UFUNCTION()
    void OnRestartClicked();

    UFUNCTION()
    void OnQuitClicked();
};
