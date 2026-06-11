#include "RangeWeapon.h"
#include "ProjectileService.h"
#include "CombatManager.h"
#include "GameFramework/Actor.h"

URangeWeapon::URangeWeapon()
{
    WeaponID = TEXT("Weapon_Range");
    SkillID = TEXT("ATK_Range");
}

float URangeWeapon::ExecuteAttack(AActor* Owner, AActor* Target)
{
    if (!Owner || !Target)
    {
        return 0.0f;
    }

    FVector Origin = GetOwnerPosition(Owner);
    FVector TargetPos = GetTargetPosition(Target);

    UWorld* World = Owner->GetWorld();
    UCombatManager* CombatMgr = World ? World->GetSubsystem<UCombatManager>() : nullptr;
    IProjectileService* ProjService = CombatMgr ? CombatMgr->GetProjectileService() : nullptr;

    if (ProjService)
    {
        float Damage = CalculateDamageWithCritical();

        FProjectileLaunchParams Params;
        Params.Origin = Origin;
        Params.TargetLocation = TargetPos;
        Params.Instigator = Owner;
        Params.Config.Speed = ProjectileSpeed;
        Params.Config.Height = Height;
        Params.Config.Damage = Damage;
        Params.Config.CollisionRadius = CollisionRadius;
        Params.Config.MaxLifetime = MaxLifetime;

        ProjService->SpawnProjectile(Params);
    }

    return 0.0f;
}
