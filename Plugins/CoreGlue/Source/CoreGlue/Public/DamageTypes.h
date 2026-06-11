#pragma once

#include "CoreMinimal.h"
#include "DamageTypes.generated.h"

UENUM(BlueprintType)
enum class EDamageType : uint8
{
    Physical,
    Magical,
    Real,
    Heal,
};
