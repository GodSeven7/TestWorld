#include "GameUISubsystem.h"
#include "GameHUDWidget.h"
#include "GameStartMenuWidget.h"
#include "GameSettingMenuWidget.h"
#include "GameOverMenuWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameUIConfig.h"

void UGameUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FWorldDelegates::OnPostWorldCleanup.AddUObject(
        this, &UGameUISubsystem::OnWorldChanged);
}

void UGameUISubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldCleanup.RemoveAll(this);

    InvalidateAllWidgets();

    Super::Deinitialize();
}

void UGameUISubsystem::SetUIConfig(UGameUIConfig* InConfig)
{
    UIConfig = InConfig;
}

UGameUIConfig* UGameUISubsystem::GetUIConfig() const
{
    return UIConfig;
}

void UGameUISubsystem::ShowHUD()
{
    if (HUDWidget)
    {
        HUDWidget->AddToViewport();
        return;
    }

    if (!UIConfig || !UIConfig->HUDWidgetClass)
    {
        return;
    }

    HUDWidget = Cast<UGameHUDWidget>(CreateWidgetInstance(UIConfig->HUDWidgetClass, 0));
}

void UGameUISubsystem::HideHUD()
{
    RemoveWidgetInstance(reinterpret_cast<UGameWidgetBase*&>(HUDWidget));
}

void UGameUISubsystem::ShowStartMenu()
{
    if (StartMenuWidget)
    {
        StartMenuWidget->AddToViewport();
        return;
    }

    if (!UIConfig || !UIConfig->StartMenuWidgetClass)
    {
        return;
    }

    StartMenuWidget = Cast<UGameStartMenuWidget>(CreateWidgetInstance(UIConfig->StartMenuWidgetClass, 1));
}

void UGameUISubsystem::HideStartMenu()
{
    RemoveWidgetInstance(reinterpret_cast<UGameWidgetBase*&>(StartMenuWidget));
}

void UGameUISubsystem::ShowSettingMenu()
{
    if (SettingMenuWidget)
    {
        SettingMenuWidget->AddToViewport();
        return;
    }

    if (!UIConfig || !UIConfig->SettingMenuWidgetClass)
    {
        return;
    }

    SettingMenuWidget = Cast<UGameSettingMenuWidget>(CreateWidgetInstance(UIConfig->SettingMenuWidgetClass, 2));
}

void UGameUISubsystem::HideSettingMenu()
{
    RemoveWidgetInstance(reinterpret_cast<UGameWidgetBase*&>(SettingMenuWidget));
}

void UGameUISubsystem::ShowGameOverMenu()
{
    if (GameOverMenuWidget)
    {
        GameOverMenuWidget->AddToViewport();
        return;
    }

    if (!UIConfig || !UIConfig->GameOverMenuWidgetClass)
    {
        return;
    }

    GameOverMenuWidget = Cast<UGameGameOverMenuWidget>(CreateWidgetInstance(UIConfig->GameOverMenuWidgetClass, 3));
}

void UGameUISubsystem::HideGameOverMenu()
{
    RemoveWidgetInstance(reinterpret_cast<UGameWidgetBase*&>(GameOverMenuWidget));
}

void UGameUISubsystem::HideAll()
{
    HideHUD();
    HideStartMenu();
    HideSettingMenu();
    HideGameOverMenu();
}

void UGameUISubsystem::RequestStartGame()
{
    OnRequestStartGame.Broadcast();
}

void UGameUISubsystem::RequestRestartGame()
{
    OnRequestRestartGame.Broadcast();
}

void UGameUISubsystem::RequestQuitGame()
{
    OnRequestQuitGame.Broadcast();
}

UGameHUDWidget* UGameUISubsystem::GetHUDWidget() const
{
    return HUDWidget;
}

UGameStartMenuWidget* UGameUISubsystem::GetStartMenuWidget() const
{
    return StartMenuWidget;
}

UGameSettingMenuWidget* UGameUISubsystem::GetSettingMenuWidget() const
{
    return SettingMenuWidget;
}

UGameGameOverMenuWidget* UGameUISubsystem::GetGameOverMenuWidget() const
{
    return GameOverMenuWidget;
}

void UGameUISubsystem::OnWorldChanged(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    InvalidateAllWidgets();
}

void UGameUISubsystem::InvalidateAllWidgets()
{
    HUDWidget = nullptr;
    StartMenuWidget = nullptr;
    SettingMenuWidget = nullptr;
    GameOverMenuWidget = nullptr;
}

UWorld* UGameUISubsystem::GetCurrentWorld() const
{
    return GetGameInstance()->GetWorld();
}

UGameWidgetBase* UGameUISubsystem::CreateWidgetInstance(TSubclassOf<UGameWidgetBase> WidgetClass, int32 ZOrder)
{
    UWorld* World = GetCurrentWorld();
    if (!World || !WidgetClass)
    {
        return nullptr;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return nullptr;
    }

    UGameWidgetBase* Widget = CreateWidget<UGameWidgetBase>(PC, WidgetClass);
    if (Widget)
    {
        Widget->AddToViewport(ZOrder);
    }

    return Widget;
}

void UGameUISubsystem::RemoveWidgetInstance(UGameWidgetBase*& WidgetPtr)
{
    if (WidgetPtr)
    {
        WidgetPtr->RemoveFromParent();
        WidgetPtr = nullptr;
    }
}
