#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WithUnifiedData.generated.h"

class UActorUnifiedDataComponent;

UINTERFACE(MinimalAPI)
class UWithUnifiedData : public UInterface
{
	GENERATED_BODY()
};

class COREGLUE_API IWithUnifiedData
{
	GENERATED_BODY()

public:
	virtual UActorUnifiedDataComponent* GetUnifiedDataComponent() const = 0;
};
