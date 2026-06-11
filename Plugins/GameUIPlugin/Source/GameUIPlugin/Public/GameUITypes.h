#pragma once

#include "CoreMinimal.h"
#include "GameUITypes.generated.h"

UENUM(BlueprintType)
enum class EGameResult : uint8
{
    InProgress UMETA(DisplayName = "In Progress"),
    Victory UMETA(DisplayName = "Victory"),
    Defeat UMETA(DisplayName = "Defeat")
};
