#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameDataSubsystem.h"
#include "GameWidgetBase.generated.h"

UCLASS()
class GAMEUIPLUGIN_API UGameWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameWidget")
    UGameDataSubsystem* GetGameData() const;

protected:
    UFUNCTION(BlueprintNativeEvent, Category = "GameWidget")
    void OnGameTimeChanged(float NewTime);
    virtual void OnGameTimeChanged_Implementation(float NewTime) {}

    UFUNCTION(BlueprintNativeEvent, Category = "GameWidget")
    void OnEnemyCountChanged(int32 NewCount);
    virtual void OnEnemyCountChanged_Implementation(int32 NewCount) {}

    UFUNCTION(BlueprintNativeEvent, Category = "GameWidget")
    void OnPlayerHealthChanged(float CurrentHealth, float MaxHealth);
    virtual void OnPlayerHealthChanged_Implementation(float CurrentHealth, float MaxHealth) {}

    UFUNCTION(BlueprintNativeEvent, Category = "GameWidget")
    void OnGameResultChanged(EGameResult NewResult);
    virtual void OnGameResultChanged_Implementation(EGameResult NewResult) {}

private:
    UPROPERTY()
    UGameDataSubsystem* GameDataSubsystem;
};
