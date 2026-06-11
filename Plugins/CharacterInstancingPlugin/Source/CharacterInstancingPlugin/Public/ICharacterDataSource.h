#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ICharacterDataSource.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UCharacterDataSource : public UInterface
{
    GENERATED_BODY()
};

class ICharacterDataSource
{
    GENERATED_BODY()

public:
    virtual FVector GetPosition() const = 0;
    virtual FRotator GetRotation() const = 0;
    virtual bool IsDead() const = 0;
    virtual bool IsAttacking() const = 0;
    virtual bool IsMoving() const = 0;
    virtual int32 GetMeshType() const = 0;
    virtual void SetMeshVisibility(bool bVisible) = 0;
};
