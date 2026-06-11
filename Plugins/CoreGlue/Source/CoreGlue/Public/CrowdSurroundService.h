#pragma once

#include "CoreMinimal.h"
#include "CrowdSurroundTypes.h"
#include "UObject/Interface.h"
#include "CrowdSurroundService.generated.h"

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class COREGLUE_API UCrowdSurroundService : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API ICrowdSurroundService
{
    GENERATED_BODY()

public:
    virtual bool RequestSurroundAssignment(const FCrowdSurroundRequest& Request, FCrowdSurroundAssignment& OutAssignment) = 0;
    virtual bool LockSurroundAssignment(int32 ObjectID) = 0;
    virtual void UnlockSurroundAssignment(int32 ObjectID) = 0;
    virtual void Update(float DeltaTime) = 0;

    virtual bool GetSurroundAssignment(int32 ObjectID, FCrowdSurroundAssignment& OutAssignment) const = 0;
    virtual bool IsInSurroundGroup(int32 ObjectID) const = 0;
};
