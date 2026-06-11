#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BattleDamageTypes.h"
#include "BattleDamageInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UBattleDamageInterface : public UInterface
{
    GENERATED_BODY()
};

class BATTLEDAMAGEPLUGIN_API IBattleDamageInterface
{
    GENERATED_BODY()

public:
    virtual const FBattleAttributeData& GetAttributeData() const = 0;
    virtual FBattleAttributeData& GetMutableAttributeData() = 0;

    virtual FBattleAttributeData GetAttributeDataThreadSafe() const
    {
        return GetAttributeData();
    }

    virtual void ApplyDamageResult(const FDamageResult& Result) = 0;

    virtual void OnDamageReceived(const FDamageInfo& Info, const FDamageResult& Result) {}
    virtual void OnDeath(const FDamageInfo& KillingBlow) {}
    virtual void OnHealReceived(float Amount, AActor* Healer) {}
    virtual void OnRevived(float HealthPercent) {}

    virtual bool IsAlive() const { return GetAttributeData().CurrentHealth > 0.0f; }
    virtual bool IsInvulnerable() const { return false; }
    virtual bool CanBeHealed() const { return IsAlive(); }

	virtual int32 GetFaction() const {
		return -1;
	};
};
