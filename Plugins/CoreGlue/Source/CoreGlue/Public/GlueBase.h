#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GlueBase.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class COREGLUE_API UGlueBase : public UObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(UWorld* World) {}
    virtual void Shutdown() {}
    virtual void Tick(float DeltaTime) {}

    UFUNCTION(BlueprintCallable, Category = "Glue")
    virtual void Suspend() { bSuspended = true; }

    UFUNCTION(BlueprintCallable, Category = "Glue")
    virtual void Resume() { bSuspended = false; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Glue")
    bool IsSuspended() const { return bSuspended; }

protected:
    UPROPERTY()
    bool bSuspended = false;
};
