#pragma once

class UGOAPManager;
class UWorld;

class GOAP_API FGOAPDebug
{
public:
    static bool IsActionDebugEnabled();
    static void DrawDebug(UWorld* World, UGOAPManager* Manager);
};
