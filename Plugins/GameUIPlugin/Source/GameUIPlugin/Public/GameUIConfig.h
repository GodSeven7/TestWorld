#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameUIConfig.generated.h"

class UGameHUDWidget;
class UGameStartMenuWidget;
class UGameSettingMenuWidget;
class UGameGameOverMenuWidget;

UCLASS(BlueprintType)
class GAMEUIPLUGIN_API UGameUIConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "GameUI")
    TSubclassOf<UGameHUDWidget> HUDWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "GameUI")
    TSubclassOf<UGameStartMenuWidget> StartMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "GameUI")
    TSubclassOf<UGameSettingMenuWidget> SettingMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "GameUI")
    TSubclassOf<UGameGameOverMenuWidget> GameOverMenuWidgetClass;
};
