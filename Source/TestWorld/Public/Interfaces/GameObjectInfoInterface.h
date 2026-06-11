#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameObjectInfoInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGameObjectInfo : public UInterface
{
    GENERATED_BODY()
};

class TESTWORLD_API IGameObjectInfo
{
    GENERATED_BODY()

public:
    virtual int32 GetFaction() const = 0;
    virtual FName GetObjectType() const { return FName(); }
};
