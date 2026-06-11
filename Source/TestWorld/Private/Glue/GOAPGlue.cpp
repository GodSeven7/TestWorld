#include "Glue/GOAPGlue.h"
#include "GOAPManager.h"
#include "SparseGridManager.h"
#include "CombatManager.h"
#include "CrowdSurroundManager.h"

void UGOAPGlue::Initialize(UWorld* World)
{
}

void UGOAPGlue::Shutdown()
{
}

void UGOAPGlue::Bind(
    UGOAPManager* InGOAPManager,
    USparseGridManager* InGridManager,
    UCombatManager* InCombatManager,
    UCrowdSurroundManager* InCrowdSurroundManager)
{
    GOAPManager = InGOAPManager;
    GridManager = InGridManager;
    CombatManager = InCombatManager;
    CrowdSurroundManager = InCrowdSurroundManager;

    if (GOAPManager && GridManager)
    {
        GOAPManager->SetSpatialQueryService(Cast<ISpatialQueryService>(GridManager));
    }

    if (GOAPManager && CombatManager)
    {
        GOAPManager->SetCombatQueryService(Cast<ICombatQueryService>(CombatManager));
    }

    if (GOAPManager && CrowdSurroundManager)
    {
        GOAPManager->SetCrowdSurroundService(Cast<ICrowdSurroundService>(CrowdSurroundManager));
    }
}

void UGOAPGlue::Tick(float DeltaTime)
{
    if (bSuspended || !GOAPManager)
    {
        return;
    }

    GOAPManager->TickManager(DeltaTime);
}
