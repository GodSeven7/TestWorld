#pragma once

#include "CoreMinimal.h"
#include "GameWidgetBase.h"
#include "GameSettingMenuWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;

UCLASS(Abstract)
class GAMEUIPLUGIN_API UGameSettingMenuWidget : public UGameWidgetBase
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* BackButton;

    UPROPERTY(meta = (BindWidget))
    USlider* VolumeSlider;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* VolumeText;

private:
    UFUNCTION()
    void OnBackClicked();
};
