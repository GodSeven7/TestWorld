#include "GOAPActionRegistry.h"

void UGOAPActionRegistry::RegisterExecutor(int32 ActionSetID, int32 ActionID, TSharedPtr<FGOAPActionExecutor> Executor)
{
    FWriteScopeLock WriteLock(RegistryLock);
    Executors.FindOrAdd(ActionSetID).Add(ActionID, Executor);
}

void UGOAPActionRegistry::ClearAll()
{
    FWriteScopeLock WriteLock(RegistryLock);
    Executors.Empty();
}

TSharedPtr<FGOAPActionExecutor> UGOAPActionRegistry::GetExecutor(int32 ActionSetID, int32 ActionID) const
{
    FReadScopeLock ReadLock(RegistryLock);
    if (const TMap<int32, TSharedPtr<FGOAPActionExecutor>>* ActionMap = Executors.Find(ActionSetID))
    {
        return ActionMap->FindRef(ActionID);
    }
    return nullptr;
}
