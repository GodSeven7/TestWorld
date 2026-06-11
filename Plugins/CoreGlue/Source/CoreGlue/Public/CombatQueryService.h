#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatQueryService.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCombatQueryService : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API ICombatQueryService
{
    GENERATED_BODY()

public:
    virtual void RequestAttackForActor(AActor* Actor, AActor* Target) = 0;
};
