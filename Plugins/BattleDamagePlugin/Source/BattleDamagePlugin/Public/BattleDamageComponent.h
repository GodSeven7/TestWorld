#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleDamageInterface.h"
#include "BattleDamageComponent.generated.h"

class UBattleDamageManager;

UCLASS(ClassGroup = "BattleDamage", meta = (BlueprintSpawnableComponent))
class BATTLEDAMAGEPLUGIN_API UBattleDamageComponent : public UActorComponent, public IBattleDamageInterface
{
    GENERATED_BODY()

public:
    UBattleDamageComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Damage")
    FBattleAttributeData Attributes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Damage")
    bool bAutoRegister = true;

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    void SetHealth(float NewHealth);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    void SetMaxHealth(float NewMaxHealth);

    UFUNCTION(BlueprintCallable, Category = "Battle Damage")
    void Revive(float HealthPercent = 1.0f);

    virtual const FBattleAttributeData& GetAttributeData() const override;
    virtual FBattleAttributeData& GetMutableAttributeData() override;
    virtual FBattleAttributeData GetAttributeDataThreadSafe() const override;
    virtual void ApplyDamageResult(const FDamageResult& Result) override;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageReceived, const FDamageInfo&, Info, const FDamageResult&, Result);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, const FDamageInfo&, KillingBlow);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealReceived, float, Amount, AActor*, Healer);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRevived, float, HealthPercent);

    UPROPERTY(BlueprintAssignable, Category = "Battle Damage")
    FOnDamageReceived OnDamageReceivedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Battle Damage")
    FOnDeath OnDeathDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Battle Damage")
    FOnHealReceived OnHealReceivedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Battle Damage")
    FOnRevived OnRevivedDelegate;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnDamageReceived(const FDamageInfo& Info, const FDamageResult& Result) override;
    virtual void OnDeath(const FDamageInfo& KillingBlow) override;
    virtual void OnHealReceived(float Amount, AActor* Healer) override;
    virtual void OnRevived(float HealthPercent) override;

private:
    UBattleDamageManager* GetManager() const;

    mutable FCriticalSection AttributeLock;
};
