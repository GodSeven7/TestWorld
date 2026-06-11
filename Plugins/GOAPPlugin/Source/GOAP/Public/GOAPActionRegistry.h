#pragma once

#include "CoreMinimal.h"
#include "GOAPActionExecutor.h"
#include "GOAPActionRegistry.generated.h"

UCLASS()
class GOAP_API UGOAPActionRegistry : public UObject
{
    GENERATED_BODY()

public:
    void RegisterExecutor(int32 ActionSetID, int32 ActionID, TSharedPtr<FGOAPActionExecutor> Executor);
    void ClearAll();

    TSharedPtr<FGOAPActionExecutor> GetExecutor(int32 ActionSetID, int32 ActionID) const;

private:
    TMap<int32, TMap<int32, TSharedPtr<FGOAPActionExecutor>>> Executors;
    mutable FRWLock RegistryLock;
};
