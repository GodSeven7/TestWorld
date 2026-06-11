// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AGamePawn;

class FCombatObjectTestCommands
{
public:
    static void Initialize();
    static void Cleanup();

    static TArray<TWeakObjectPtr<AGamePawn>> SpawnedTestActors;
    static int32 CurrentTestTypeID;
};
