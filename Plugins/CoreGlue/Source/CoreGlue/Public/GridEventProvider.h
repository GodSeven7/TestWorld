#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridEventProvider.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorRemovedFromGrid, AActor*);

UINTERFACE(MinimalAPI, Blueprintable)
class UGridEventProvider : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API IGridEventProvider
{
    GENERATED_BODY()

public:
    virtual FOnActorRemovedFromGrid& OnActorRemovedFromGrid() = 0;
};
