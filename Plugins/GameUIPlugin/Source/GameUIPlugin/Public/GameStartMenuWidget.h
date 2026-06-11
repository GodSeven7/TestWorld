#pragma once

#include "CoreMinimal.h"
#include "GameWidgetBase.h"
#include "GameStartMenuWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS(Abstract)
class GAMEUIPLUGIN_API UGameStartMenuWidget : public UGameWidgetBase
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* StartButton;

    UPROPERTY(meta = (BindWidget))
    UButton* SettingsButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TitleText;

private:
    UFUNCTION()
    void OnStartClicked();

    UFUNCTION()
    void OnSettingsClicked();
};
