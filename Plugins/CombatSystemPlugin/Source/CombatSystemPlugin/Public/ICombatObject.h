#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ICombatObject.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCombatObject : public UInterface
{
    GENERATED_BODY()
};

class COMBATSYSTEMPLUGIN_API ICombatObject
{
    GENERATED_BODY()

public:
    virtual FVector GetCombatWorldPosition() const { return FVector::ZeroVector; }
    virtual FVector GetCombatForwardVector() const { return FVector::ForwardVector; }
    virtual int32 GetCombatFaction() const { return 0; }
    virtual UObject* GetCombatParent() const { return nullptr; }

    virtual int32 GetFaction() const = 0;
    
    bool IsHostileTo(const ICombatObject* Other) const
    {
        if (!Other)
            return false;

        int32 MyFaction = this->GetFaction();
        int32 OtherFaction = Other->GetFaction();

        if (MyFaction == 0 || OtherFaction == 0)
        {
            return MyFaction != OtherFaction;
        }

        return MyFaction != OtherFaction;
    }

    bool IsFriendlyTo(const ICombatObject* Other) const
    {
        if (!Other)
            return false;

        int32 MyFaction = this->GetFaction();
        int32 OtherFaction = Other->GetFaction();

        if (MyFaction == 0 || OtherFaction == 0)
        {
            return MyFaction == OtherFaction;
        }

        return MyFaction == OtherFaction;
    }

    virtual void OnCombatAttackExecuted(float Damage, bool bIsCritical) {}
    virtual void OnCombatHitReceived(float Damage) {}
};
