#pragma once

#include "CoreMinimal.h"
#include "CombatWeapon.h"
#include "MeleeWeapon.generated.h"

UCLASS(Blueprintable)
class COMBATSYSTEMPLUGIN_API UMeleeWeapon : public UCombatWeapon
{
    GENERATED_BODY()

public:
    UMeleeWeapon();
};
