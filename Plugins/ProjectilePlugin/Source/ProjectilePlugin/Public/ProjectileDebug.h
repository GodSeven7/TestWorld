#pragma once

#include "CoreMinimal.h"

class UProjectileManager;

class PROJECTILEPLUGIN_API FProjectileDebug
{
public:
    static void Initialize();
    static void Deinitialize();

    static void RegisterConsoleCommands();
    static void UnregisterConsoleCommands();

private:
    static void FireTestProjectiles(UProjectileManager* Manager, int32 Count, float SpreadAngle);
    static void FireProjectileAtDirection(UProjectileManager* Manager, const FVector& Origin, const FRotator& Rotation);

    static TArray<IConsoleObject*> RegisteredConsoleCommands;
};
