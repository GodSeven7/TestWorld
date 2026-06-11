#pragma once

#include "CoreMinimal.h"
#include "GOAPTypes.h"
#include "GOAPActionSet.generated.h"

UCLASS(BlueprintType)
class GOAP_API UGOAPActionSet : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "GOAP")
    TArray<FGOAPActionData> Actions;

    const TArray<FGOAPActionData>& GetActions() const { return Actions; }

    void AddAction(const FGOAPActionData& Action)
    {
        Actions.Add(Action);
    }
};
