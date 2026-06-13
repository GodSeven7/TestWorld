#include "Components/GOAPAgentWithUnifiedDataComponent.h"
#include "ActorUnifiedDataComponent.h"
#include "Core/UnifiedDataExtensionComponent.h"
#include "GOAPGameTypes.h"
#include "GOAPAgentConfig.h"
#include "GOAPManager.h"
#include "CrowdSurroundService.h"
#include "GameFramework/Actor.h"
#include "Actors/GameCharacter.h"
#include "CombatComponent.h"


UGOAPAgentWithUnifiedDataComponent::UGOAPAgentWithUnifiedDataComponent()
{
}

void UGOAPAgentWithUnifiedDataComponent::BeginPlay()
{
	UnifiedDataComponent = GetOwner()->FindComponentByClass<UActorUnifiedDataComponent>();
	ExtensionComponent = GetOwner()->FindComponentByClass<UUnifiedDataExtensionComponent>();

	Super::BeginPlay();
}

UActorUnifiedDataComponent* UGOAPAgentWithUnifiedDataComponent::GetUnifiedDataComponent() const
{
	return UnifiedDataComponent;
}

int32 UGOAPAgentWithUnifiedDataComponent::GetObjectID() const
{
	return UnifiedDataComponent ? UnifiedDataComponent->GetObjectID() : INDEX_NONE;
}

FVector UGOAPAgentWithUnifiedDataComponent::QueryWorldPosition() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetPosition();
	}
	return Super::QueryWorldPosition();
}

void UGOAPAgentWithUnifiedDataComponent::ApplyWorldPosition(const FVector& Position)
{
	if (UnifiedDataComponent)
	{
		UnifiedDataComponent->SetPosition(Position);
	}
	else
	{
		Super::ApplyWorldPosition(Position);
	}
}

void UGOAPAgentWithUnifiedDataComponent::ApplyWorldRotation(const FRotator& Rotation)
{
	if (UnifiedDataComponent)
	{
		UnifiedDataComponent->SetRotation(Rotation);
	}
	else
	{
		Super::ApplyWorldRotation(Rotation);
	}
}

void UGOAPAgentWithUnifiedDataComponent::ApplyForwardVector(const FVector& Forward)
{
	if (UnifiedDataComponent)
	{
		UnifiedDataComponent->SetForwardVector(Forward);
	}
	else
	{
		Super::ApplyForwardVector(Forward);
	}
}

void UGOAPAgentWithUnifiedDataComponent::ApplyContext(const FGOAPAgentContext& Context, float DeltaTime)
{
	if (UnifiedDataComponent)
	{
		// GOAP outputs intent only; FlowField and Steering turn it into movement.
		UnifiedDataComponent->SetMoveSpeed(Context.MoveSpeed);

		if (Context.bHasDesiredPosition)
		{
			UnifiedDataComponent->SetDesiredPosition(Context.DesiredPosition);
		}
		else
		{
			UnifiedDataComponent->SetDesiredPosition(FVector::ZeroVector);
		}

		// Combat crowd agents stay pushable. Stable standing is handled by PBD contact constraints,
		// not by turning attackers into hard navigation obstacles.
		UnifiedDataComponent->SetNavigationObstacle(false);
	}

	if (!Context.LookDirection.IsNearlyZero())
	{
		ApplyWorldRotation(Context.LookDirection.Rotation());
		ApplyForwardVector(Context.LookDirection.GetSafeNormal());
	}
}

int32 UGOAPAgentWithUnifiedDataComponent::QueryFaction() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetFaction();
	}
	return Super::QueryFaction();
}

bool UGOAPAgentWithUnifiedDataComponent::QueryIsAlive() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->IsAlive();
	}
	return Super::QueryIsAlive();
}

bool UGOAPAgentWithUnifiedDataComponent::QueryHasTarget() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->HasTarget();
	}
	return Super::QueryHasTarget();
}

FVector UGOAPAgentWithUnifiedDataComponent::QueryTargetPosition() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetTargetPosition();
	}
	return Super::QueryTargetPosition();
}

UObject* UGOAPAgentWithUnifiedDataComponent::QueryTargetObject() const
{
	return CachedTargetActor.Get();
}

float UGOAPAgentWithUnifiedDataComponent::QueryMoveSpeed() const
{
	if (AActor* Owner = GetOwner())
	{
		if (AGameCharacter* Character = Cast<AGameCharacter>(Owner))
		{
			return Character->MoveSpeed;
		}
	}
	return Super::QueryMoveSpeed();
}

float UGOAPAgentWithUnifiedDataComponent::QueryAttackRange() const
{
	if (AActor* Owner = GetOwner())
	{
		if (UCombatComponent* CombatComp = Owner->FindComponentByClass<UCombatComponent>())
		{
			float WeaponRange = CombatComp->GetWeaponRange();
			if (WeaponRange > 0.0f)
			{
				return WeaponRange;
			}
		}
	}
	return Super::QueryAttackRange();
}

FGOAPWorldState UGOAPAgentWithUnifiedDataComponent::GetGoalState() const
{
	if (ExtensionComponent)
	{
		return ExtensionComponent->GetGoalState();
	}
	return Super::GetGoalState();
}

void UGOAPAgentWithUnifiedDataComponent::OnTargetFound(UActorUnifiedDataComponent* TargetData)
{
    AActor* NewTarget = TargetData ? TargetData->GetOwner() : nullptr;

    // 目标切换：清理旧目标的 CrowdSurround 状态
    if (CachedTargetActor.IsValid() && CachedTargetActor.Get() != NewTarget)
    {
        if (UGOAPManager* Manager = GetWorld()->GetSubsystem<UGOAPManager>())
        {
            ICrowdSurroundService* SurroundService = Manager->GetCrowdSurroundService();
            if (SurroundService)
            {
                SurroundService->ClearSurroundState(GetObjectID());
            }
        }
    }

    if (UnifiedDataComponent && TargetData)
    {
        FVector TargetPos = TargetData->GetPosition();
        float Distance = FVector::Dist2D(QueryWorldPosition(), TargetPos);
        int32 TargetID = static_cast<int32>(TargetData->GetUniqueID());
        UnifiedDataComponent->SetTarget(TargetID, TargetPos, Distance);
    }

    CachedTargetActor = NewTarget;
}

void UGOAPAgentWithUnifiedDataComponent::OnTargetLost()
{
    if (UnifiedDataComponent)
    {
        UnifiedDataComponent->ClearTarget();
    }

    CachedTargetActor = nullptr;

    Super::OnTargetLost();
}

void UGOAPAgentWithUnifiedDataComponent::SetCombatGoalWithTarget(AActor* Target)
{
	SetCombatGoal();

	if (Target)
	{
		if (UActorUnifiedDataComponent* TargetData = Target->FindComponentByClass<UActorUnifiedDataComponent>())
		{
			OnTargetFound(TargetData);
		}
	}
}
