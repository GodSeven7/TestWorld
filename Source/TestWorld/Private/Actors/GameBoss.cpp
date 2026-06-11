#include "Actors/GameBoss.h"
#include "Components/CombatWithUnifiedDataComponent.h"
#include "Components/SparseGridWithUnifiedDataComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Skills/DashSkill.h"
#include "CharacterSkelMeshComponent.h"

AGameBoss::AGameBoss()
{
    if (CombatComponent)
    {
        // SkillClasses 已移除，技能通过 CharacterID/WeaponID 从配置表加载
        // CombatComponent->SkillClasses.Add(UDashSkill::StaticClass());
    }

    if (SparseGridComponent)
    {
        SparseGridComponent->SetEnableSteering(false);
    }
}
