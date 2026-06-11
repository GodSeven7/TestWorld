#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameUISubsystem.generated.h"

class UGameUIConfig;
class UGameWidgetBase;
class UGameHUDWidget;
class UGameStartMenuWidget;
class UGameSettingMenuWidget;
class UGameGameOverMenuWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestStartGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestRestartGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestQuitGame);

UCLASS()
class GAMEUIPLUGIN_API UGameUISubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void SetUIConfig(UGameUIConfig* InConfig);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameUI")
    UGameUIConfig* GetUIConfig() const;

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void ShowHUD();

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void HideHUD();

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void ShowStartMenu();

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void HideStartMenu();

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void ShowSettingMenu();

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void HideSettingMenu();

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void ShowGameOverMenu();

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void HideGameOverMenu();

    UFUNCTION(BlueprintCallable, Category = "GameUI")
    void HideAll();

    // ===== Game Flow Requests =====

    UFUNCTION(BlueprintCallable, Category = "GameUI|Flow")
    void RequestStartGame();

    UFUNCTION(BlueprintCallable, Category = "GameUI|Flow")
    void RequestRestartGame();

    UFUNCTION(BlueprintCallable, Category = "GameUI|Flow")
    void RequestQuitGame();

    UPROPERTY(BlueprintAssignable, Category = "GameUI|Flow")
    FOnRequestStartGame OnRequestStartGame;

    UPROPERTY(BlueprintAssignable, Category = "GameUI|Flow")
    FOnRequestRestartGame OnRequestRestartGame;

    UPROPERTY(BlueprintAssignable, Category = "GameUI|Flow")
    FOnRequestQuitGame OnRequestQuitGame;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameUI")
    UGameHUDWidget* GetHUDWidget() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameUI")
    UGameStartMenuWidget* GetStartMenuWidget() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameUI")
    UGameSettingMenuWidget* GetSettingMenuWidget() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameUI")
    UGameGameOverMenuWidget* GetGameOverMenuWidget() const;

	void OnWorldChanged(UWorld* World, bool bSessionEnded, bool bCleanupResources);

private:
    UPROPERTY()
    UGameUIConfig* UIConfig;

    UPROPERTY()
    UGameHUDWidget* HUDWidget;

    UPROPERTY()
    UGameStartMenuWidget* StartMenuWidget;

    UPROPERTY()
    UGameSettingMenuWidget* SettingMenuWidget;

    UPROPERTY()
    UGameGameOverMenuWidget* GameOverMenuWidget;

    void InvalidateAllWidgets();

    UWorld* GetCurrentWorld() const;

    UGameWidgetBase* CreateWidgetInstance(TSubclassOf<UGameWidgetBase> WidgetClass, int32 ZOrder);
    void RemoveWidgetInstance(UGameWidgetBase*& WidgetPtr);
};
