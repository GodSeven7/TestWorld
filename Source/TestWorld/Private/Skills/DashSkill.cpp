#include "Skills/DashSkill.h"
#include "ActorUnifiedDataComponent.h"
#include "WithUnifiedData.h"
#include "CombatComponent.h"
#include "GameFramework/Actor.h"

UDashSkill::UDashSkill()
{
    SkillID = TEXT("SKILL_Dash");
}

bool UDashSkill::CanExecute(AActor* Owner) const
{
    return Super::CanExecute(Owner);
}

void UDashSkill::OnSkillStarted(AActor* Owner)
{
    if (!Owner)
        return;

    UActorUnifiedDataComponent* UnifiedComp = nullptr;
    if (UCombatComponent* CombatComp = Owner->FindComponentByClass<UCombatComponent>())
    {
        if (IWithUnifiedData* WithData = Cast<IWithUnifiedData>(CombatComp))
        {
            UnifiedComp = WithData->GetUnifiedDataComponent();
        }
    }

    if (UnifiedComp)
    {
        DashStartLocation = UnifiedComp->GetPosition();
        DashDirection = UnifiedComp->GetForwardVector();
    }
    else
    {
        DashStartLocation = Owner->GetActorLocation();
        DashDirection = Owner->GetActorForwardVector();
    }

    DashDirection.Z = 0.0f;
    if (DashDirection.IsNearlyZero())
    {
        DashDirection = FVector::ForwardVector;
    }
    DashDirection.Normalize();

    DashTargetLocation = DashStartLocation + DashDirection * GetConfigDashDistance();
    DashDistanceTraveled = 0.0f;

    if (UnifiedComp)
    {
        UnifiedComp->SetForwardVector(DashDirection);
        UnifiedComp->SetRotation(DashDirection.Rotation());
    }
}

void UDashSkill::ExecuteInternal(AActor* Owner, float DeltaTime)
{
    if (!Owner)
        return;

    float MoveDelta = GetConfigDashSpeed() * DeltaTime;
    DashDistanceTraveled += MoveDelta;

    FVector NewPos;
    if (DashDistanceTraveled >= GetConfigDashDistance())
    {
        NewPos = DashTargetLocation;
    }
    else
    {
        NewPos = DashStartLocation + DashDirection * DashDistanceTraveled;
    }

    UActorUnifiedDataComponent* UnifiedComp = nullptr;
    if (UCombatComponent* CombatComp = Owner->FindComponentByClass<UCombatComponent>())
    {
        if (IWithUnifiedData* WithData = Cast<IWithUnifiedData>(CombatComp))
        {
            UnifiedComp = WithData->GetUnifiedDataComponent();
        }
    }

    if (UnifiedComp)
    {
        UnifiedComp->SetPosition(NewPos);
        UnifiedComp->SetForwardVector(DashDirection);
        UnifiedComp->SetRotation(DashDirection.Rotation());
    }
}

void UDashSkill::OnSkillCompleted(AActor* Owner)
{
    if (!Owner)
        return;

    UActorUnifiedDataComponent* UnifiedComp = nullptr;
    if (UCombatComponent* CombatComp = Owner->FindComponentByClass<UCombatComponent>())
    {
        if (IWithUnifiedData* WithData = Cast<IWithUnifiedData>(CombatComp))
        {
            UnifiedComp = WithData->GetUnifiedDataComponent();
        }
    }

    if (UnifiedComp)
    {
        UnifiedComp->SetForwardVector(DashDirection);
        UnifiedComp->SetRotation(DashDirection.Rotation());
    }
}
