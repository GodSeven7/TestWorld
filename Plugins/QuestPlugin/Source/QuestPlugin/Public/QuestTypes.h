#pragma once

#include "CoreMinimal.h"
#include "QuestTypes.generated.h"

UENUM(BlueprintType)
enum class EQuestState : uint8
{
    Inactive UMETA(DisplayName = "Inactive"),
    Active UMETA(DisplayName = "Active"),
    Completed UMETA(DisplayName = "Completed"),
    Failed UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class EQuestObjectiveResult : uint8
{
    Victory UMETA(DisplayName = "Victory"),
    Defeat UMETA(DisplayName = "Defeat")
};
