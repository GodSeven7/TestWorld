#include "Components/SparseGridWithUnifiedDataComponent.h"
#include "ActorUnifiedDataComponent.h"
#include "GameFramework/Actor.h"

USparseGridWithUnifiedDataComponent::USparseGridWithUnifiedDataComponent()
{
}

void USparseGridWithUnifiedDataComponent::BeginPlay()
{
	// 先缓存数据源，确保 RegisterToGrid 时 GetFaction() 能返回正确值
	UnifiedDataComponent = GetOwner()->FindComponentByClass<UActorUnifiedDataComponent>();

	Super::BeginPlay();
}

UActorUnifiedDataComponent* USparseGridWithUnifiedDataComponent::GetUnifiedDataComponent() const
{
	return UnifiedDataComponent;
}

FVector USparseGridWithUnifiedDataComponent::GetSparseGridPosition() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetPosition();
	}
	return Super::GetSparseGridPosition();
}

FVector USparseGridWithUnifiedDataComponent::GetSparseGridForwardVector() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetForwardVector();
	}
	return Super::GetSparseGridForwardVector();
}

int32 USparseGridWithUnifiedDataComponent::GetFaction() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetFaction();
	}
	return -1;
}

FVector USparseGridWithUnifiedDataComponent::QueryMovementVelocity() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetMovementVelocity();
	}
	return FVector::ZeroVector;
}

float USparseGridWithUnifiedDataComponent::QueryMoveSpeed() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetMoveSpeed();
	}
	return 0.0f;
}

FVector USparseGridWithUnifiedDataComponent::QueryDesiredPosition() const
{
	if (UnifiedDataComponent)
	{
		// Prefer CorrectedDesiredPosition if available, otherwise fall back to DesiredPosition
		if (UnifiedDataComponent->HasCorrectedDesiredPosition())
		{
			return UnifiedDataComponent->GetCorrectedDesiredPosition();
		}
		if (UnifiedDataComponent->HasDesiredPosition())
		{
			return UnifiedDataComponent->GetDesiredPosition();
		}
		return FVector::ZeroVector;
	}
	return Super::QueryDesiredPosition();
}

bool USparseGridWithUnifiedDataComponent::IsNavigationObstacle() const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->IsNavigationObstacle();
	}
	return Super::IsNavigationObstacle();
}

FVector USparseGridWithUnifiedDataComponent::GetPredictedPosition(float DeltaTime) const
{
	if (UnifiedDataComponent)
	{
		return UnifiedDataComponent->GetPosition()
			+ UnifiedDataComponent->GetMovementVelocity() * DeltaTime;
	}
	return GetSparseGridPosition();
}
