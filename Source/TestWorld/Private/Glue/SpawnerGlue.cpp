#include "Glue/SpawnerGlue.h"
#include "Subsystems/SpawnerManager.h"
#include "SparseGridManager.h"

void USpawnerGlue::Initialize(UWorld* World)
{
}

void USpawnerGlue::Shutdown()
{
}

void USpawnerGlue::Bind(
    USpawnerManager* InSpawnerManager,
    USparseGridManager* InGridManager,
    UEventBus* InEventBus)
{
    SpawnerManager = InSpawnerManager;
    GridManager = InGridManager;
    EventBus = InEventBus;
}

void USpawnerGlue::Tick(float DeltaTime)
{
    if (bSuspended || !SpawnerManager)
        return;

    SpawnerManager->TickSpawning(DeltaTime);
}
