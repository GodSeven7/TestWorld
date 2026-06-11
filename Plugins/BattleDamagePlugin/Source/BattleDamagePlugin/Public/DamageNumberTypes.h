#pragma once

#include "CoreMinimal.h"
#include "DamageNumberTypes.generated.h"

USTRUCT(BlueprintType)
struct BATTLEDAMAGEPLUGIN_API FDamageNumberSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Number")
    int32 MaxDamageNumberCount = 256;
};
