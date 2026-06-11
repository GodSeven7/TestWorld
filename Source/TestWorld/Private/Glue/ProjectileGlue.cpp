#include "Glue/ProjectileGlue.h"
#include "ProjectileManager.h"
#include "SparseGridManager.h"
#include "BattleDamageManager.h"

static bool bDisableProjectile = false;
FAutoConsoleVariableRef CVarDisableProjectile(
	TEXT("Disable.Projectile"),
	bDisableProjectile,
	TEXT(""));

void UProjectileGlue::Initialize(UWorld* World)
{
}

void UProjectileGlue::Shutdown()
{
}

void UProjectileGlue::Bind(
    UProjectileManager* InProjectileManager,
    USparseGridManager* InGridManager,
    UBattleDamageManager* InDamageManager)
{
    ProjectileManager = InProjectileManager;
    GridManager = InGridManager;
    DamageManager = InDamageManager;

    if (ProjectileManager && GridManager)
    {
        ProjectileManager->SetSpatialQueryService(Cast<ISpatialQueryService>(GridManager));
    }

    if (ProjectileManager && DamageManager)
    {
        ProjectileManager->SetDamageCallback([this](AActor* Target, float Damage, int32 Faction)
        {
            if (DamageManager)
            {
                FDamageInfo Info;
                Info.BaseDamage = Damage;
                Info.DamageType = EDamageType::Physical;
                Info.Target = Target;
                DamageManager->QueueDamage(Target, Info);
            }
        });
    }
}

void UProjectileGlue::Tick(float DeltaTime)
{
    if (bSuspended || !ProjectileManager || bDisableProjectile)
        return;

    ProjectileManager->Tick(DeltaTime);
}
