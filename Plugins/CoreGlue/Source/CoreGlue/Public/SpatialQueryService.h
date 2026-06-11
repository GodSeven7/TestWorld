#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridObjectInfo.h"
#include "SpatialQueryService.generated.h"

USTRUCT(BlueprintType)
struct FSpatialQueryFilter
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TSubclassOf<AActor>> ActorClasses;

    UPROPERTY()
    int32 FactionFilter = -1;

    UPROPERTY()
    float MaxDistance = MAX_FLT;
};

USTRUCT(BlueprintType)
struct FSpatialQueryHandle
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Index = -1;

    UPROPERTY()
    uint32 Version = 0;

    bool IsValid() const { return Index >= 0; }
};

UINTERFACE(MinimalAPI, Blueprintable)
class USpatialQueryService : public UInterface
{
    GENERATED_BODY()
};

class COREGLUE_API ISpatialQueryService
{
    GENERATED_BODY()

public:
    virtual TArray<AActor*> QueryActorsInSphere(
        const FVector& Center,
        float Radius,
        const FSpatialQueryFilter& Filter = FSpatialQueryFilter()) const = 0;

    virtual TArray<AActor*> QueryActorsInBox(
        const FVector& Center,
        const FVector& Extent,
        const FSpatialQueryFilter& Filter = FSpatialQueryFilter()) const = 0;

    virtual AActor* QueryNearestActor(
        const FVector& Origin,
        const FSpatialQueryFilter& Filter = FSpatialQueryFilter()) const = 0;

    virtual TArray<FSpatialQueryHandle> QueryHandlesInSphere(
        const FVector& Center,
        float Radius) const = 0;

    virtual TArray<FSpatialQueryHandle> QueryHandlesInSphereExcludingFactions(
        const FVector& Center,
        float Radius,
        int32 ExcludeFactionID) const = 0;

    virtual TArray<FSpatialQueryHandle> QueryHandlesInBoxExcludingFactions(
        const FVector& Center,
        const FVector& Extent,
        int32 ExcludeFactionID) const = 0;

    virtual IGridObjectInfo* GetObjectFromHandle(const FSpatialQueryHandle& Handle) const = 0;

    // Resolve ObjectID to grid object info (O(1) lookup via internal mapping)
    virtual IGridObjectInfo* GetObjectByID(int32 ObjectID) const = 0;
};
