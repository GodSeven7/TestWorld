#pragma once

#include "CoreMinimal.h"
#include "CombatWeapon.h"
#include "RangeWeapon.generated.h"

UCLASS(Blueprintable)
class COMBATSYSTEMPLUGIN_API URangeWeapon : public UCombatWeapon
{
    GENERATED_BODY()

public:
    URangeWeapon();

    virtual float ExecuteAttack(AActor* Owner, AActor* Target) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float ProjectileSpeed = 2000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Height = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float CollisionRadius = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float MaxLifetime = 5.0f;
};
